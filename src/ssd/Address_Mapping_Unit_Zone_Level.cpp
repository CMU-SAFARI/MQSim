#include <cmath>
#include <assert.h>
#include <stdexcept>

#include "Address_Mapping_Unit_Page_Level.h"
#include "Address_Mapping_Unit_Zone_Level.h"
#include "Stats.h"
#include "../utils/Logical_Address_Partitioning_Unit.h"

namespace SSD_Components
{
	Address_Mapping_Unit_Zone_Level::Address_Mapping_Unit_Zone_Level(const sim_object_id_type& id, FTL* ftl, NVM_PHY_ONFI* flash_controller, Flash_Block_Manager_Base* block_manager, Flash_Zone_Manager_Base* zone_manager,
		bool ideal_mapping_table, unsigned int cmt_capacity_in_byte, 
		Flash_Plane_Allocation_Scheme_Type PlaneAllocationScheme,
		Zone_Allocation_Scheme_Type ZoneAllocationScheme,
		SubZone_Allocation_Scheme_Type SubZoneAllocationScheme,
		unsigned int concurrent_stream_no,
		unsigned int channel_count, unsigned int chip_no_per_channel, unsigned int die_no_per_chip, unsigned int plane_no_per_die,
		unsigned int ChannelNoPerZone, unsigned int ChipNoPerZone,
		unsigned int DieNoPerZone, unsigned int PlaneNoPerZone,
		std::vector<std::vector<flash_channel_ID_type>> stream_channel_ids, std::vector<std::vector<flash_chip_ID_type>> stream_chip_ids,
		std::vector<std::vector<flash_die_ID_type>> stream_die_ids, std::vector<std::vector<flash_plane_ID_type>> stream_plane_ids,
		unsigned int Block_no_per_plane, unsigned int Page_no_per_block, unsigned int SectorsPerPage, unsigned int PageSizeInByte,
		double Overprovisioning_ratio, CMT_Sharing_Mode sharing_mode, bool fold_large_addresses, bool Support_Zone)
		: Address_Mapping_Unit_Page_Level(id, ftl, flash_controller, block_manager, ideal_mapping_table, cmt_capacity_in_byte, PlaneAllocationScheme, concurrent_stream_no, channel_count, chip_no_per_channel, die_no_per_chip, plane_no_per_die, stream_channel_ids, stream_chip_ids, stream_die_ids, stream_plane_ids, Block_no_per_plane, Page_no_per_block, SectorsPerPage, PageSizeInByte, Overprovisioning_ratio, sharing_mode, fold_large_addresses)
	{
		fzm = zone_manager;
		zones = fzm->zones;
		domains = new AddressMappingDomain*[no_of_input_streams];

		for (unsigned int domainID = 0; domainID < no_of_input_streams; domainID++)
		{
			domains[domainID]->ZoneAllocationeScheme = ZoneAllocationScheme;
			domains[domainID]->SubZoneAllocationScheme = SubZoneAllocationScheme;
			domains[domainID]->Channel_No_Per_Zone = ChannelNoPerZone;
			domains[domainID]->Chip_No_Per_Zone = ChipNoPerZone;
			domains[domainID]->Die_No_Per_Zone = DieNoPerZone;
			domains[domainID]->Plane_No_Per_Zone = PlaneNoPerZone;
		}
		
	}

	Address_Mapping_Unit_Zone_Level::~Address_Mapping_Unit_Zone_Level()
	{
	}

	void Address_Mapping_Unit_Zone_Level::allocate_plane_for_preconditioning(stream_id_type stream_id, LPA_type lpn, NVM::FlashMemory::Physical_Page_Address& targetAddress)
	{
		
	}

	void Address_Mapping_Unit_Zone_Level::Translate_lpa_to_ppa_and_dispatch(const std::list<NVM_Transaction*>& transactionList)
	{
		for (std::list<NVM_Transaction*>::const_iterator it = transactionList.begin(); it != transactionList.end(); ) {
			if (is_lpa_locked_for_gc((*it)->Stream_id, ((NVM_Transaction_Flash*)(*it))->LPA)) {
				// iterator should be post-incremented since the iterator may be deleted from list
				manage_user_transaction_facing_barrier((NVM_Transaction_Flash*)*(it++));
			} else {
				query_cmt((NVM_Transaction_Flash*)(*it++));
			}
		}

		if (transactionList.size() > 0)  {
			ftl->TSU->Prepare_for_transaction_submit();
			for(std::list<NVM_Transaction*>::const_iterator it = transactionList.begin(); it != transactionList.end(); it++) {
				if (((NVM_Transaction_Flash*)(*it))->Physical_address_determined) {
					ftl->TSU->Submit_transaction(static_cast<NVM_Transaction_Flash*>(*it));
					if (((NVM_Transaction_Flash*)(*it))->Type == Transaction_Type::WRITE) {
						if (((NVM_Transaction_Flash_WR*)(*it))->RelatedRead != NULL) {
							ftl->TSU->Submit_transaction(((NVM_Transaction_Flash_WR*)(*it))->RelatedRead);
						}
					}
				}
			}
			ftl->TSU->Schedule();
		}
	}
	bool Address_Mapping_Unit_Zone_Level::query_cmt(NVM_Transaction_Flash* transaction)
	{
		stream_id_type stream_id = transaction->Stream_id;
		Stats::total_CMT_queries++;
		Stats::total_CMT_queries_per_stream[stream_id]++;

		if (domains[stream_id]->Mapping_entry_accessible(ideal_mapping_table, stream_id, transaction->LPA))//Either limited or unlimited CMT
		{
			Stats::CMT_hits_per_stream[stream_id]++;
			Stats::CMT_hits++;
			if (transaction->Type == Transaction_Type::READ) {
				Stats::total_readTR_CMT_queries_per_stream[stream_id]++;
				Stats::total_readTR_CMT_queries++;
				Stats::readTR_CMT_hits_per_stream[stream_id]++;
				Stats::readTR_CMT_hits++;
			} else {
				//This is a write transaction
				Stats::total_writeTR_CMT_queries++;
				Stats::total_writeTR_CMT_queries_per_stream[stream_id]++;
				Stats::writeTR_CMT_hits++;
				Stats::writeTR_CMT_hits_per_stream[stream_id]++;
			}

			if (translate_lpa_to_ppa(stream_id, transaction)) {
				return true;
			} else {
				mange_unsuccessful_translation(transaction);
				return false;
			}
		} else {//Limited CMT
			//Maybe we can catch mapping data from an on-the-fly write back request
			if (request_mapping_entry(stream_id, transaction->LPA)) {
				Stats::CMT_miss++;
				Stats::CMT_miss_per_stream[stream_id]++;
				if (transaction->Type == Transaction_Type::READ) {
					Stats::total_readTR_CMT_queries++;
					Stats::total_readTR_CMT_queries_per_stream[stream_id]++;
					Stats::readTR_CMT_miss++;
					Stats::readTR_CMT_miss_per_stream[stream_id]++;
				} else { //This is a write transaction
					Stats::total_writeTR_CMT_queries++;
					Stats::total_writeTR_CMT_queries_per_stream[stream_id]++;
					Stats::writeTR_CMT_miss++;
					Stats::writeTR_CMT_miss_per_stream[stream_id]++;
				}
				if (translate_lpa_to_ppa(stream_id, transaction)) {
					return true;
				} else {
					mange_unsuccessful_translation(transaction);
					return false;
				}
			} else {
				if (transaction->Type == Transaction_Type::READ) {
					Stats::total_readTR_CMT_queries++;
					Stats::total_readTR_CMT_queries_per_stream[stream_id]++;
					Stats::readTR_CMT_miss++;
					Stats::readTR_CMT_miss_per_stream[stream_id]++;
					domains[stream_id]->Waiting_unmapped_read_transactions.insert(std::pair<LPA_type, NVM_Transaction_Flash*>(transaction->LPA, transaction));
				} else {//This is a write transaction
					Stats::total_writeTR_CMT_queries++;
					Stats::total_writeTR_CMT_queries_per_stream[stream_id]++;
					Stats::writeTR_CMT_miss++;
					Stats::writeTR_CMT_miss_per_stream[stream_id]++;
					domains[stream_id]->Waiting_unmapped_program_transactions.insert(std::pair<LPA_type, NVM_Transaction_Flash*>(transaction->LPA, transaction));
				}
			}

			return false;
		}
	}

	bool Address_Mapping_Unit_Zone_Level::translate_lpa_to_ppa(stream_id_type streamID, NVM_Transaction_Flash* transaction)  
	{
		PPA_type ppa = domains[streamID]->Get_ppa(ideal_mapping_table, streamID, transaction->LPA);

		if (transaction->Type == Transaction_Type::READ) {
			if (ppa == NO_PPA) {
				//ppa = online_create_entry_for_reads(transaction->LPA, streamID, transaction->Address, ((NVM_Transaction_Flash_RD*)transaction)->read_sectors_bitmap);
				// I don't know for now, but I think this situation will not happen, ZNS
			}
			transaction->PPA = ppa;
			Convert_ppa_to_address(transaction->PPA, transaction->Address);
			block_manager->Read_transaction_issued(transaction->Address);
			transaction->Physical_address_determined = true;

			return true;
			
		} else { // This is a write transaction
			allocate_plane_for_user_write((NVM_Transaction_Flash_WR*)transaction);
			allocate_page_in_plane_for_user_write((NVM_Transaction_Flash_WR*)transaction, false);
			transaction->Physical_address_determined = true;

			return true;
		}
	}

	void Address_Mapping_Unit_Zone_Level::allocate_plane_for_user_write(NVM_Transaction_Flash_WR* transaction)
	{
		LPA_type lpn = transaction->LPA;
		NVM::FlashMemory::Physical_Page_Address& targetAddress = transaction->Address;
		AddressMappingDomain* domain = domains[transaction->Stream_id];

		unsigned int zone_size_in_byte = fzm->zone_size * 1024 * 1024;
		unsigned int block_size_in_byte = pages_no_per_block * page_size_in_byte;
		unsigned int zone_p_level = domain->Channel_No_Per_Zone * domain->Chip_No_Per_Zone * domain->Die_No_Per_Zone * domain->Plane_No_Per_Zone;
		unsigned int total_level = domain->Channel_no * domain->Chip_no * domain->Die_no * domain->Plane_no;

		Zone_ID_type zoneID = lpn / zone_size_in_byte;
		unsigned int zoneOffset = lpn % zone_size_in_byte;
		unsigned int subzoneID = zoneOffset / (zone_size_in_byte / zone_p_level);
		unsigned int subzoneOffset = zoneOffset % (zone_size_in_byte / zone_p_level);
		unsigned int blockID = subzoneOffset / block_size_in_byte;
		unsigned int blockOffset = subzoneOffset % block_size_in_byte;
		unsigned int pageID = blockOffset / page_size_in_byte;

		unsigned int subzone_no_per_zone = zone_size_in_byte / zone_p_level / block_size_in_byte;

		//if (zoneOffset < zones[zoneID]->write_point)
		//	PRINT_ERROR("write off set is smaller than write_point in a zone. Something wrong!");
		
		unsigned int index;
        switch (domain->ZoneAllocationeScheme) {
			case Zone_Allocation_Scheme_Type::CDPW:
				if (domain->Channel_No_Per_Zone > 1) { // one zone is spread to at least two channels
					if(domain->Channel_No_Per_Zone == channel_count && 
					   domain->Chip_No_Per_Zone == chip_no_per_channel && 
					   domain->Die_No_Per_Zone == die_no_per_chip && 
					   domain->Plane_No_Per_Zone == plane_no_per_die) { // this is the maximum parallelism case
						index = subzoneID;
						targetAddress.BlockID = subzone_no_per_zone * zoneID + blockID;
						targetAddress.PageID = pageID;
					}
					else {

					}				
				}
				else if (domain->Chip_No_Per_Zone > 1) {
				
				}
				else if (domain->Die_No_Per_Zone > 1 ) {

				}
				else if (domain->Plane_No_Per_Zone > 1){

				} 
				else {// 1*1*1*1 = minimum parallelism in one zone 
					index = zoneID;
					targetAddress.BlockID = subzone_no_per_zone * (zoneID/total_level) + blockID;
					targetAddress.PageID = pageID;
				}

				targetAddress.ChannelID = domain->Channel_ids[index % channel_count];
				targetAddress.ChipID = domain->Chip_ids[(index / (channel_count * die_no_per_chip * plane_no_per_die)) % chip_no_per_channel];
				targetAddress.DieID = domain->Die_ids[index / channel_count % die_no_per_chip];
				targetAddress.PlaneID = domain->Plane_ids[index / (channel_count*die_no_per_chip) % plane_no_per_die];
				break;
			default:
				PRINT_ERROR("Unknown zone allocation scheme type!")
		}		
	}

	void Address_Mapping_Unit_Zone_Level::allocate_page_in_plane_for_user_write(NVM_Transaction_Flash_WR *transaction, bool is_for_gc)
	{
		AddressMappingDomain* domain = domains[transaction->Stream_id];
		PPA_type old_ppa = domain->Get_ppa(ideal_mapping_table, transaction->Stream_id, transaction->LPA);

		if (old_ppa = NO_PPA){ /* This is the first time to the logical page */
			if (is_for_gc){
				PRINT_ERROR("Unexpected mapping table status in allocate_page_in_plane_for_user_Write function in Address_Mapping_Unit_Zone_Level for a GC/WL write");
			}
		} else {	// we have aready LPA
			return;
		}
		if (is_for_gc) {
			return; // ZNS may no need this part
		} else {
			block_manager->Allocate_block_and_page_in_plane_for_user_write_in_Zone(transaction->Stream_id, transaction->Address);
		}

		transaction->PPA = Convert_address_to_ppa(transaction->Address);
		domain->Update_mapping_info(ideal_mapping_table, transaction->Stream_id, transaction->LPA, transaction->PPA, 
			((NVM_Transaction_Flash_WR*)transaction)->write_sectors_bitmap | domain->Get_page_status(ideal_mapping_table, transaction->Stream_id, 
			transaction->LPA));
		
	}

	Zone_ID_type Address_Mapping_Unit_Zone_Level::translate_lpa_to_zoneID_for_gc(std::list<NVM_Transaction*> transaction_list)
	{
		if (transaction_list.size() == 0) {
			PRINT_ERROR("No transactions in NVM_Transaction_Flash_ER!!");
		}

		NVM_Transaction_Flash_ER* tr = static_cast<NVM_Transaction_Flash_ER*>(transaction_list.front());
		LPA_type lpn = tr->LPA;

		return lpn / (fzm->zone_size * 1024 * 1024);
	}

	Zone_ID_type Address_Mapping_Unit_Zone_Level::get_zone_block_list(std::list<NVM_Transaction*> transaction_list, std::list<NVM::FlashMemory::Physical_Page_Address*> &list)
	{
		Zone_ID_type zoneID = translate_lpa_to_zoneID_for_gc(transaction_list);
		NVM_Transaction_Flash_ER* tr = static_cast<NVM_Transaction_Flash_ER*>(transaction_list.front());
		AddressMappingDomain* domain = domains[tr->Stream_id];

		switch (domain->ZoneAllocationeScheme) {
			case Zone_Allocation_Scheme_Type::CDPW:
				for (int i = 0; i < fzm->zone_count; i++) {
					for (int j = 4 * i; j < 4 * (i + 1); j++) {
						NVM::FlashMemory::Physical_Page_Address* address = new NVM::FlashMemory::Physical_Page_Address;
						address->ChannelID = domain->Channel_ids[i % channel_count];
						address->ChipID = domain->Chip_ids[(i / (channel_count * die_no_per_chip * plane_no_per_die)) % chip_no_per_channel];
						address->DieID = domain->Die_ids[i / channel_count % die_no_per_chip];
						address->PlaneID = domain->Plane_ids[i / (channel_count*die_no_per_chip) % plane_no_per_die];
						address->BlockID = j;
						list.push_back(address);
					}
				}
				break;
			default:
				PRINT_MESSAGE("no zone allocateion scheme?");
			
		}
		return zoneID;	
	}

}
