#include <cmath>
#include <assert.h>
#include <stdexcept>

#include "Address_Mapping_Unit_Page_Level.h"
#include "Address_Mapping_Unit_Zone_Level.h"
#include "Stats.h"
#include "../utils/Logical_Address_Partitioning_Unit.h"

namespace SSD_Components
{
	Address_Mapping_Unit_Zone_Level* Address_Mapping_Unit_Zone_Level::_my_instance = NULL;
	Address_Mapping_Unit_Zone_Level::Address_Mapping_Unit_Zone_Level(const sim_object_id_type& id, FTL* ftl, NVM_PHY_ONFI* flash_controller, Flash_Block_Manager_Base* block_manager, Flash_Zone_Manager_Base* zone_manager, bool Support_Zone,
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
		double Overprovisioning_ratio, CMT_Sharing_Mode sharing_mode, bool fold_large_addresses)
		: Address_Mapping_Unit_Base(id, ftl, flash_controller, block_manager, ideal_mapping_table, concurrent_stream_no, channel_count, chip_no_per_channel, die_no_per_chip, plane_no_per_die,
		Block_no_per_plane, Page_no_per_block, SectorsPerPage, PageSizeInByte, Overprovisioning_ratio, sharing_mode, fold_large_addresses)
	{
		_my_instance = this;
		domains = new AddressMappingDomain*[no_of_input_streams];

		Write_transactions_for_overfull_planes = new std::set<NVM_Transaction_Flash_WR*>***[channel_count];
		for (unsigned int channel_id = 0; channel_id < channel_count; channel_id++) {
			Write_transactions_for_overfull_planes[channel_id] = new std::set<NVM_Transaction_Flash_WR*>**[chip_no_per_channel];
			for (unsigned int chip_id = 0; chip_id < chip_no_per_channel; chip_id++) {
				Write_transactions_for_overfull_planes[channel_id][chip_id] = new std::set<NVM_Transaction_Flash_WR*>*[die_no_per_chip];
				for (unsigned int die_id = 0; die_id < die_no_per_chip; die_id++) {
					Write_transactions_for_overfull_planes[channel_id][chip_id][die_id] = new std::set<NVM_Transaction_Flash_WR*>[plane_no_per_die];
				}
			}
		}

		flash_channel_ID_type* channel_ids = NULL;
		flash_channel_ID_type* chip_ids = NULL;
		flash_channel_ID_type* die_ids = NULL;
		flash_channel_ID_type* plane_ids = NULL;
		for (unsigned int domainID = 0; domainID < no_of_input_streams; domainID++) {
			/* Since we want to have the same mapping table entry size for all streams, the entry size
			*  is calculated at this level and then pass it to the constructors of mapping domains
			* entry size = sizeOf(lpa) + sizeOf(ppn) + sizeOf(bit vector that shows written sectors of a page)
			*/
			CMT_entry_size = (unsigned int)std::ceil(((2 * std::log2(total_physical_pages_no)) + sector_no_per_page) / 8);
			//In GTD we do not need to store lpa
			GTD_entry_size = (unsigned int)std::ceil((std::log2(total_physical_pages_no) + sector_no_per_page) / 8);
			no_of_translation_entries_per_page = (SectorsPerPage * SECTOR_SIZE_IN_BYTE) / GTD_entry_size;

			Cached_Mapping_Table* sharedCMT = NULL;
			unsigned int per_stream_cmt_capacity = 0;
			cmt_capacity = cmt_capacity_in_byte / CMT_entry_size;
			switch (sharing_mode) {
				case CMT_Sharing_Mode::SHARED:
					per_stream_cmt_capacity = cmt_capacity;
					sharedCMT = new Cached_Mapping_Table(cmt_capacity);
					break;
				case CMT_Sharing_Mode::EQUAL_SIZE_PARTITIONING:
					per_stream_cmt_capacity = cmt_capacity / no_of_input_streams;
					break;
			}


			channel_ids = new flash_channel_ID_type[stream_channel_ids[domainID].size()];
			for (unsigned int i = 0; i < stream_channel_ids[domainID].size(); i++) {
				if (stream_channel_ids[domainID][i] < channel_count) {
					channel_ids[i] = stream_channel_ids[domainID][i];
				} else {
					PRINT_ERROR("Invalid channel ID specified for I/O flow " << domainID);
				}
			}

			chip_ids = new flash_channel_ID_type[stream_chip_ids[domainID].size()];
			for (unsigned int i = 0; i < stream_chip_ids[domainID].size(); i++) {
				if (stream_chip_ids[domainID][i] < chip_no_per_channel) {
					chip_ids[i] = stream_chip_ids[domainID][i];
				} else {
					PRINT_ERROR("Invalid chip ID specified for I/O flow " << domainID);
				}
			}

			die_ids = new flash_channel_ID_type[stream_die_ids[domainID].size()];
			for (unsigned int i = 0; i < stream_die_ids[domainID].size(); i++) {
				if (stream_die_ids[domainID][i] < die_no_per_chip) {
					die_ids[i] = stream_die_ids[domainID][i];
				} else {
					PRINT_ERROR("Invalid die ID specified for I/O flow " << domainID);
				}
			}

			plane_ids = new flash_channel_ID_type[stream_plane_ids[domainID].size()];
			for (unsigned int i = 0; i < stream_plane_ids[domainID].size(); i++) {
				if (stream_plane_ids[domainID][i] < plane_no_per_die) {
					plane_ids[i] = stream_plane_ids[domainID][i];
				} else {
					PRINT_ERROR("Invalid plane ID specified for I/O flow " << domainID);
				}
			}

			domains[domainID] = new AddressMappingDomain(per_stream_cmt_capacity, CMT_entry_size, no_of_translation_entries_per_page,
				sharedCMT,
				PlaneAllocationScheme,
				channel_ids, (unsigned int)(stream_channel_ids[domainID].size()), chip_ids, (unsigned int)(stream_chip_ids[domainID].size()), die_ids, 
				(unsigned int)(stream_die_ids[domainID].size()), plane_ids, (unsigned int)(stream_plane_ids[domainID].size()),
				Utils::Logical_Address_Partitioning_Unit::PDA_count_allocate_to_flow(domainID), Utils::Logical_Address_Partitioning_Unit::LHA_count_allocate_to_flow_from_device_view(domainID),
				sector_no_per_page);
			delete[] channel_ids;
			delete[] chip_ids;
			delete[] die_ids;
			delete[] plane_ids;
		}

		// for ZNS
		std::cout << "nysong - we are here, in Zone_Level" << std::endl;
		fzm = zone_manager;
		zones = fzm->zones;
		for (unsigned int domainID = 0; domainID < no_of_input_streams; domainID++)
		{
			domains[domainID]->ZoneAllocationScheme = ZoneAllocationScheme;
			domains[domainID]->SubZoneAllocationScheme = SubZoneAllocationScheme;
			domains[domainID]->Channel_No_Per_Zone = ChannelNoPerZone;
			domains[domainID]->Chip_No_Per_Zone = ChipNoPerZone;
			domains[domainID]->Die_No_Per_Zone = DieNoPerZone;
			domains[domainID]->Plane_No_Per_Zone = PlaneNoPerZone;
			std::cout << "CWPD=" << ChannelNoPerZone << ChipNoPerZone << DieNoPerZone << PlaneNoPerZone << std::endl;
			std::cout << "'zone_size = " << fzm->zone_size << std::endl;
		}
		
	}

	Address_Mapping_Unit_Zone_Level::~Address_Mapping_Unit_Zone_Level()
	{
		for (unsigned int i = 0; i < no_of_input_streams; i++) {
			delete domains[i];
		}
		delete[] domains;
	}

	void Address_Mapping_Unit_Zone_Level::Setup_triggers()
	{
		Sim_Object::Setup_triggers();
		flash_controller->ConnectToTransactionServicedSignal(handle_transaction_serviced_signal_from_PHY);
	}

	void Address_Mapping_Unit_Zone_Level::Start_simulation()
	{
		Store_mapping_table_on_flash_at_start();
	}

	void Address_Mapping_Unit_Zone_Level::Validate_simulation_config()
	{
	}

	void Address_Mapping_Unit_Zone_Level::Execute_simulator_event(MQSimEngine::Sim_Event* event)
	{
	}

	void Address_Mapping_Unit_Zone_Level::Store_mapping_table_on_flash_at_start()
	{
		if (mapping_table_stored_on_flash) {
			return;
		}
		//Since address translation functions work on flash transactions
		NVM_Transaction_Flash_WR* dummy_tr = new NVM_Transaction_Flash_WR(Transaction_Source_Type::MAPPING, 0, 0,
			NO_LPA, 0, NULL, 0, NULL, 0, 0);

		for (unsigned int stream_id = 0; stream_id < no_of_input_streams; stream_id++) {
			dummy_tr->Stream_id = stream_id;
			for (MVPN_type translation_page_id = 0; translation_page_id < domains[stream_id]->Total_translation_pages_no; translation_page_id++) {
				dummy_tr->LPA = (LPA_type)translation_page_id;
				allocate_plane_for_translation_write(dummy_tr);
				allocate_page_in_plane_for_translation_write(dummy_tr, (MVPN_type)dummy_tr->LPA, false);
				flash_controller->Change_flash_page_status_for_preconditioning(dummy_tr->Address, dummy_tr->LPA);
			}
		}
		mapping_table_stored_on_flash = true;
	}

	int Address_Mapping_Unit_Zone_Level::Bring_to_CMT_for_preconditioning(stream_id_type stream_id, LPA_type lpa)
	{
		if (domains[stream_id]->GlobalMappingTable[lpa].PPA == NO_PPA) {
			PRINT_ERROR("Touching an unallocated logical address in preconditioning!")
		}

		if (domains[stream_id]->CMT->Exists(stream_id, lpa)) {
			return domains[stream_id]->No_of_inserted_entries_in_preconditioning;
		}

		if (domains[stream_id]->CMT->Check_free_slot_availability()) {
			domains[stream_id]->CMT->Reserve_slot_for_lpn(stream_id, lpa);
			domains[stream_id]->CMT->Insert_new_mapping_info(stream_id, lpa,
				domains[stream_id]->GlobalMappingTable[lpa].PPA, domains[stream_id]->GlobalMappingTable[lpa].WrittenStateBitmap);
		} else {
			LPA_type evicted_lpa;
			domains[stream_id]->CMT->Evict_one_slot(evicted_lpa);
			domains[stream_id]->CMT->Reserve_slot_for_lpn(stream_id, lpa);
			domains[stream_id]->CMT->Insert_new_mapping_info(stream_id, lpa,
				domains[stream_id]->GlobalMappingTable[lpa].PPA, domains[stream_id]->GlobalMappingTable[lpa].WrittenStateBitmap);
		}
		domains[stream_id]->No_of_inserted_entries_in_preconditioning++;
		
		return domains[stream_id]->No_of_inserted_entries_in_preconditioning;
	}

	unsigned int Address_Mapping_Unit_Zone_Level::Get_cmt_capacity()
	{
		return cmt_capacity;
	}

	unsigned int Address_Mapping_Unit_Zone_Level::Get_current_cmt_occupancy_for_stream(stream_id_type stream_id)
	{
		return domains[stream_id]->No_of_inserted_entries_in_preconditioning;
	}

	void Address_Mapping_Unit_Zone_Level::Translate_lpa_to_ppa_and_dispatch(const std::list<NVM_Transaction*>& transactionList)
	{
		for (std::list<NVM_Transaction*>::const_iterator it = transactionList.begin();
			it != transactionList.end(); ) {
			if (is_lpa_locked_for_gc((*it)->Stream_id, ((NVM_Transaction_Flash*)(*it))->LPA)) {
				//iterator should be post-incremented since the iterator may be deleted from list
				manage_user_transaction_facing_barrier((NVM_Transaction_Flash*)*(it++));
			} else {
				query_cmt((NVM_Transaction_Flash*)(*it++));
			}
		}

		if (transactionList.size() > 0) {
			ftl->TSU->Prepare_for_transaction_submit();
			for (std::list<NVM_Transaction*>::const_iterator it = transactionList.begin();
				it != transactionList.end(); it++) {
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

	/*This function should be invoked only if the address translation entry exists in CMT.
	* Otherwise, the call to the CMT->Rerieve_ppa, within this function, will throw an exception.
	*/
	bool Address_Mapping_Unit_Zone_Level::translate_lpa_to_ppa(stream_id_type streamID, NVM_Transaction_Flash* transaction)
	{
		PPA_type ppa = domains[streamID]->Get_ppa(ideal_mapping_table, streamID, transaction->LPA);

		if (transaction->Type == Transaction_Type::READ) {
			if (ppa == NO_PPA) {
				ppa = online_create_entry_for_reads(transaction->LPA, streamID, transaction->Address, ((NVM_Transaction_Flash_RD*)transaction)->read_sectors_bitmap);
			}
			transaction->PPA = ppa;
			Convert_ppa_to_address(transaction->PPA, transaction->Address);
			block_manager->Read_transaction_issued(transaction->Address);
			transaction->Physical_address_determined = true;
			
			return true;
		} else {//This is a write transaction
			allocate_plane_for_user_write((NVM_Transaction_Flash_WR*)transaction);
			//there are too few free pages remaining only for GC
			if (ftl->GC_and_WL_Unit->Stop_servicing_writes(transaction->Address)){
				return false;
			}
			allocate_page_in_plane_for_user_write((NVM_Transaction_Flash_WR*)transaction, false);
			transaction->Physical_address_determined = true;
			
			return true;
		}
	}
	
	void Address_Mapping_Unit_Zone_Level::Allocate_address_for_preconditioning(const stream_id_type stream_id, std::map<LPA_type, page_status_type>& lpa_list, std::vector<double>& steady_state_distribution)
	{
		int idx = 0;
		std::vector<LPA_type>**** assigned_lpas = new std::vector<LPA_type>***[channel_count];
		for (unsigned int channel_cntr = 0; channel_cntr < channel_count; channel_cntr++) {
			assigned_lpas[channel_cntr] = new std::vector<LPA_type>**[chip_no_per_channel];
			for (unsigned int chip_cntr = 0; chip_cntr < chip_no_per_channel; chip_cntr++) {
				assigned_lpas[channel_cntr][chip_cntr] = new std::vector<LPA_type>*[die_no_per_chip];
				for (unsigned int die_cntr = 0; die_cntr < die_no_per_chip; die_cntr++) {
					assigned_lpas[channel_cntr][chip_cntr][die_cntr] = new std::vector<LPA_type>[plane_no_per_die];
				}
			}
		}

		//First: distribute LPAs to planes
		NVM::FlashMemory::Physical_Page_Address plane_address;
		for (auto lpa = lpa_list.begin(); lpa != lpa_list.end();) {
			if ((*lpa).first >= domains[stream_id]->Total_logical_pages_no) {
				PRINT_ERROR("Out of range LPA specified for preconditioning! LPA shoud be smaller than " << domains[stream_id]->Total_logical_pages_no << ", but it is " << (*lpa).first)
			}
			PPA_type ppa = domains[stream_id]->Get_ppa_for_preconditioning(stream_id, (*lpa).first);
			if (ppa != NO_LPA) {
				PRINT_ERROR("Calling address allocation for a previously allocated LPA during preconditioning!")
			}
			allocate_plane_for_preconditioning(stream_id, (*lpa).first, plane_address);
			if (LPA_type(Utils::Logical_Address_Partitioning_Unit::Get_share_of_physcial_pages_in_plane(plane_address.ChannelID, plane_address.ChipID, plane_address.DieID, plane_address.PlaneID) * page_no_per_plane)
				> assigned_lpas[plane_address.ChannelID][plane_address.ChipID][plane_address.DieID][plane_address.PlaneID].size()) {
				assigned_lpas[plane_address.ChannelID][plane_address.ChipID][plane_address.DieID][plane_address.PlaneID].push_back((*lpa).first);
				lpa++;
			} else {
				lpa_list.erase(lpa++);
			}
		}

		//Second: distribute LPAs within planes based on the steady-state status of blocks
		//unsigned int safe_guard_band = ftl->GC_and_WL_Unit->Get_minimum_number_of_free_pages_before_GC();
		for (unsigned int channel_cntr = 0; channel_cntr < domains[stream_id]->Channel_no; channel_cntr++) {
			for (unsigned int chip_cntr = 0; chip_cntr < domains[stream_id]->Chip_no; chip_cntr++) {
				for (unsigned int die_cntr = 0; die_cntr < domains[stream_id]->Die_no; die_cntr++) {
					for (unsigned int plane_cntr = 0; plane_cntr < domains[stream_id]->Plane_no; plane_cntr++) {
						plane_address.ChannelID = domains[stream_id]->Channel_ids[channel_cntr];
						plane_address.ChipID = domains[stream_id]->Chip_ids[chip_cntr];
						plane_address.DieID = domains[stream_id]->Die_ids[die_cntr];
						plane_address.PlaneID = domains[stream_id]->Plane_ids[plane_cntr];

						unsigned int physical_block_consumption_goal = (unsigned int)(double(block_no_per_plane - ftl->GC_and_WL_Unit->Get_minimum_number_of_free_pages_before_GC() / 2)
							* Utils::Logical_Address_Partitioning_Unit::Get_share_of_physcial_pages_in_plane(plane_address.ChannelID, plane_address.ChipID, plane_address.DieID, plane_address.PlaneID));

						//Adjust the average
						double model_average = 0;
						std::vector<double> adjusted_steady_state_distribution;
						//Check if probability distribution is correct 
						for (unsigned int i = 0; i <= pages_no_per_block; i++) {
							model_average += steady_state_distribution[i] * double(i) / double(pages_no_per_block);
							adjusted_steady_state_distribution.push_back(steady_state_distribution[i]);
						}
						double real_average = double(assigned_lpas[plane_address.ChannelID][plane_address.ChipID][plane_address.DieID][plane_address.PlaneID].size()) / (physical_block_consumption_goal * pages_no_per_block);
						if (std::abs(model_average - real_average) * pages_no_per_block > 0.9999) {
							int displacement_index = int((real_average - model_average) * pages_no_per_block);
							if (displacement_index > 0) {
								for (int i = 0; i < displacement_index; i++) {
									adjusted_steady_state_distribution[i] = 0;
								}
								for (int i = displacement_index; i < int(pages_no_per_block); i++) {
									adjusted_steady_state_distribution[i] = steady_state_distribution[i - displacement_index];
								}
							} else {
								displacement_index *= -1;
								for (int i = 0; i < int(pages_no_per_block) - displacement_index; i++) {
									adjusted_steady_state_distribution[i] = steady_state_distribution[i + displacement_index];
								}
								for (int i = int(pages_no_per_block) - displacement_index; i < int(pages_no_per_block); i++) {
									adjusted_steady_state_distribution[i] = 0;
								}
							}
						}

						//Check if it is possible to find a PPA for each LPA with current proability assignments 
						unsigned int total_valid_pages = 0;
						for (int valid_pages_in_block = pages_no_per_block; valid_pages_in_block >= 0; valid_pages_in_block--) {
							total_valid_pages += valid_pages_in_block * (unsigned int)(adjusted_steady_state_distribution[valid_pages_in_block] * physical_block_consumption_goal);
						}
						unsigned int pages_need_PPA = 0;//The number of LPAs that remain unassigned due to imperfect probability assignments
						if (total_valid_pages < assigned_lpas[plane_address.ChannelID][plane_address.ChipID][plane_address.DieID][plane_address.PlaneID].size()) {
							pages_need_PPA = (unsigned int)(assigned_lpas[plane_address.ChannelID][plane_address.ChipID][plane_address.DieID][plane_address.PlaneID].size()) - total_valid_pages;
						}
						
						unsigned int remaining_blocks_to_consume = physical_block_consumption_goal;
						for (int valid_pages_in_block = pages_no_per_block; valid_pages_in_block >= 0; valid_pages_in_block--) {
							unsigned int block_no_with_x_valid_page = (unsigned int)(adjusted_steady_state_distribution[valid_pages_in_block] * physical_block_consumption_goal);
							if (block_no_with_x_valid_page > 0 && pages_need_PPA > 0) {
								block_no_with_x_valid_page += (pages_need_PPA / valid_pages_in_block) + (pages_need_PPA % valid_pages_in_block == 0 ? 0 : 1);
								pages_need_PPA = 0;
							}

							if (block_no_with_x_valid_page <= remaining_blocks_to_consume) {
								remaining_blocks_to_consume -= block_no_with_x_valid_page;
							} else {
								block_no_with_x_valid_page = remaining_blocks_to_consume;
								remaining_blocks_to_consume = 0;
							}

							for (unsigned int block_cntr = 0; block_cntr < block_no_with_x_valid_page; block_cntr++) {
								//Assign physical addresses
								std::vector<NVM::FlashMemory::Physical_Page_Address> addresses;
								if (assigned_lpas[plane_address.ChannelID][plane_address.ChipID][plane_address.DieID][plane_address.PlaneID].size() < valid_pages_in_block) {
									valid_pages_in_block = int(assigned_lpas[plane_address.ChannelID][plane_address.ChipID][plane_address.DieID][plane_address.PlaneID].size());
								}
								for (int page_cntr = 0; page_cntr < valid_pages_in_block; page_cntr++) {
									NVM::FlashMemory::Physical_Page_Address addr(plane_address.ChannelID, plane_address.ChipID, plane_address.DieID, plane_address.PlaneID, 0, 0);
									addresses.push_back(addr);
								}
								block_manager->Allocate_Pages_in_block_and_invalidate_remaining_for_preconditioning(stream_id, plane_address, addresses);

								//Update mapping table
								for (auto const &address : addresses) {
									LPA_type lpa = assigned_lpas[plane_address.ChannelID][plane_address.ChipID][plane_address.DieID][plane_address.PlaneID].back();
									assigned_lpas[plane_address.ChannelID][plane_address.ChipID][plane_address.DieID][plane_address.PlaneID].pop_back();
									PPA_type ppa = Convert_address_to_ppa(address);
									flash_controller->Change_memory_status_preconditioning(&address, &lpa);
									domains[stream_id]->GlobalMappingTable[lpa].PPA = ppa;
									domains[stream_id]->GlobalMappingTable[lpa].WrittenStateBitmap = (*lpa_list.find(lpa)).second;
									domains[stream_id]->GlobalMappingTable[lpa].TimeStamp = 0;
								}
							}
						}
						if (assigned_lpas[plane_address.ChannelID][plane_address.ChipID][plane_address.DieID][plane_address.PlaneID].size() > 0) {
							PRINT_ERROR("It is not possible to assign PPA to all LPAs in Allocate_address_for_preconditioning! It is not safe to continue preconditioning." << assigned_lpas[plane_address.ChannelID][plane_address.ChipID][plane_address.DieID][plane_address.PlaneID].size())
						}
					}
				}
			}
		}

		for (unsigned int channel_cntr = 0; channel_cntr < channel_count; channel_cntr++) {
			for (unsigned int chip_cntr = 0; chip_cntr < chip_no_per_channel; chip_cntr++) {
				for (unsigned int die_cntr = 0; die_cntr < die_no_per_chip; die_cntr++) {
					delete[] assigned_lpas[channel_cntr][chip_cntr][die_cntr];
				}
				delete[] assigned_lpas[channel_cntr][chip_cntr];
			}
			delete[] assigned_lpas[channel_cntr];
		}
		delete[] assigned_lpas;
	}

	void Address_Mapping_Unit_Zone_Level::Allocate_new_page_for_gc(NVM_Transaction_Flash_WR* transaction, bool is_translation_page)
	{
		if (is_translation_page) {
			MPPN_type mppn = domains[transaction->Stream_id]->GlobalTranslationDirectory[transaction->LPA].MPPN;
			if (mppn == NO_MPPN) {
				PRINT_ERROR("Unexpected situation occured for gc write in Allocate_new_page_for_gc function!")
			}

			allocate_page_in_plane_for_translation_write(transaction, (MVPN_type)transaction->LPA, true);
			transaction->Physical_address_determined = true;
		} else {
			if (!domains[transaction->Stream_id]->Mapping_entry_accessible(ideal_mapping_table, transaction->Stream_id, transaction->LPA)) {
				if (!domains[transaction->Stream_id]->CMT->Check_free_slot_availability()) {
					LPA_type evicted_lpa;
					CMTSlotType evictedItem = domains[transaction->Stream_id]->CMT->Evict_one_slot(evicted_lpa);
					if (evictedItem.Dirty) {
						/* In order to eliminate possible race conditions for the requests that
						* will access the evicted lpa in the near future (before the translation
						* write finishes), MQSim updates GMT (the on flash mapping table) right
						* after eviction happens.*/
						domains[transaction->Stream_id]->GlobalMappingTable[evicted_lpa].PPA = evictedItem.PPA;
						domains[transaction->Stream_id]->GlobalMappingTable[evicted_lpa].WrittenStateBitmap = evictedItem.WrittenStateBitmap;
						if (domains[transaction->Stream_id]->GlobalMappingTable[evicted_lpa].TimeStamp > CurrentTimeStamp) {
							throw std::logic_error("Unexpected situation occured in handling GMT!");
						}
						domains[transaction->Stream_id]->GlobalMappingTable[evicted_lpa].TimeStamp = CurrentTimeStamp;
						generate_flash_writeback_request_for_mapping_data(transaction->Stream_id, evicted_lpa);
					}
				}
				domains[transaction->Stream_id]->CMT->Reserve_slot_for_lpn(transaction->Stream_id, transaction->LPA);
				domains[transaction->Stream_id]->CMT->Insert_new_mapping_info(transaction->Stream_id, transaction->LPA, Convert_address_to_ppa(transaction->Address), transaction->write_sectors_bitmap);
			}

			allocate_page_in_plane_for_user_write(transaction, true);
			transaction->Physical_address_determined = true;

			//the mapping entry should be updated
			stream_id_type stream_id = transaction->Stream_id;
			Stats::total_CMT_queries++;
			Stats::total_CMT_queries_per_stream[stream_id]++;

			//either limited or unlimited mapping
			if (domains[stream_id]->Mapping_entry_accessible(ideal_mapping_table, stream_id, transaction->LPA)) {
				Stats::CMT_hits++;
				Stats::CMT_hits_per_stream[stream_id]++;
				Stats::total_writeTR_CMT_queries++;
				Stats::total_writeTR_CMT_queries_per_stream[stream_id]++;
				Stats::writeTR_CMT_hits++;
				Stats::writeTR_CMT_hits_per_stream[stream_id]++;
				domains[stream_id]->Update_mapping_info(ideal_mapping_table, stream_id, transaction->LPA, transaction->PPA, transaction->write_sectors_bitmap);
			} else { //the else block only executed for non-ideal mapping table in which CMT has a limited capacity and mapping data is read/written from/to the flash storage
				if (!domains[stream_id]->CMT->Check_free_slot_availability()) {
					LPA_type evicted_lpa;
					CMTSlotType evictedItem = domains[stream_id]->CMT->Evict_one_slot(evicted_lpa);
					if (evictedItem.Dirty) {
						/* In order to eliminate possible race conditions for the requests that
						* will access the evicted lpa in the near future (before the translation
						* write finishes), MQSim updates GMT (the on flash mapping table) right
						* after eviction happens.*/
						domains[stream_id]->GlobalMappingTable[evicted_lpa].PPA = evictedItem.PPA;
						domains[stream_id]->GlobalMappingTable[evicted_lpa].WrittenStateBitmap = evictedItem.WrittenStateBitmap;
						if (domains[stream_id]->GlobalMappingTable[evicted_lpa].TimeStamp > CurrentTimeStamp)
							throw std::logic_error("Unexpected situation occured in handling GMT!");
						domains[stream_id]->GlobalMappingTable[evicted_lpa].TimeStamp = CurrentTimeStamp;
						generate_flash_writeback_request_for_mapping_data(stream_id, evicted_lpa);
					}
				}
				domains[stream_id]->CMT->Reserve_slot_for_lpn(stream_id, transaction->LPA);
				domains[stream_id]->CMT->Insert_new_mapping_info(stream_id, transaction->LPA, transaction->PPA, transaction->write_sectors_bitmap);
			}
		}
	}

	void Address_Mapping_Unit_Zone_Level::allocate_plane_for_preconditioning(stream_id_type stream_id, LPA_type lpn, NVM::FlashMemory::Physical_Page_Address& targetAddress)
	{
		AddressMappingDomain* domain = domains[stream_id];

		switch (domain->PlaneAllocationScheme) {
			case Flash_Plane_Allocation_Scheme_Type::CWDP:
				targetAddress.ChannelID = domain->Channel_ids[(unsigned int)(lpn % domain->Channel_no)];
				targetAddress.ChipID = domain->Chip_ids[(unsigned int)((lpn / domain->Channel_no) % domain->Chip_no)];
				targetAddress.DieID = domain->Die_ids[(unsigned int)((lpn / (domain->Chip_no * domain->Channel_no)) % domain->Die_no)];
				targetAddress.PlaneID = domain->Plane_ids[(unsigned int)((lpn / (domain->Die_no * domain->Chip_no * domain->Channel_no)) % domain->Plane_no)];
				break;
			case Flash_Plane_Allocation_Scheme_Type::CWPD:
				targetAddress.ChannelID = domain->Channel_ids[(unsigned int)(lpn % domain->Channel_no)];
				targetAddress.ChipID = domain->Chip_ids[(unsigned int)((lpn / domain->Channel_no) % domain->Chip_no)];
				targetAddress.DieID = domain->Die_ids[(unsigned int)((lpn / (domain->Channel_no * domain->Chip_no * domain->Plane_no)) % domain->Die_no)];
				targetAddress.PlaneID = domain->Plane_ids[(unsigned int)((lpn / (domain->Channel_no * domain->Chip_no)) % domain->Plane_no)];
				break;
			case Flash_Plane_Allocation_Scheme_Type::CDWP:
				targetAddress.ChannelID = domain->Channel_ids[(unsigned int)(lpn % domain->Channel_no)];
				targetAddress.ChipID = domain->Chip_ids[(unsigned int)((lpn / (domain->Die_no * domain->Channel_no)) % domain->Chip_no)];
				targetAddress.DieID = domain->Die_ids[(unsigned int)((lpn / domain->Channel_no) % domain->Die_no)];
				targetAddress.PlaneID = domain->Plane_ids[(unsigned int)((lpn / (domain->Die_no * domain->Chip_no * domain->Channel_no)) % domain->Plane_no)];
				break;
			case Flash_Plane_Allocation_Scheme_Type::CDPW:
				targetAddress.ChannelID = domain->Channel_ids[(unsigned int)(lpn % domain->Channel_no)];
				targetAddress.ChipID = domain->Chip_ids[(unsigned int)((lpn / (domain->Plane_no * domain->Die_no * domain->Channel_no)) % domain->Chip_no)];
				targetAddress.DieID = domain->Die_ids[(unsigned int)((lpn / domain->Channel_no) % domain->Die_no)];
				targetAddress.PlaneID = domain->Plane_ids[(unsigned int)((lpn / (domain->Die_no * domain->Channel_no)) % domain->Plane_no)];
				break;
			case Flash_Plane_Allocation_Scheme_Type::CPWD:
				targetAddress.ChannelID = domain->Channel_ids[(unsigned int)(lpn % domain->Channel_no)];
				targetAddress.ChipID = domain->Chip_ids[(unsigned int)((lpn / (domain->Plane_no * domain->Channel_no)) % domain->Chip_no)];
				targetAddress.DieID = domain->Die_ids[(unsigned int)((lpn / (domain->Plane_no * domain->Chip_no * domain->Channel_no)) % domain->Die_no)];
				targetAddress.PlaneID = domain->Plane_ids[(unsigned int)((lpn / domain->Channel_no) % domain->Plane_no)];
				break;
			case Flash_Plane_Allocation_Scheme_Type::CPDW:
				targetAddress.ChannelID = domain->Channel_ids[(unsigned int)(lpn % domain->Channel_no)];
				targetAddress.ChipID = domain->Chip_ids[(unsigned int)((lpn / (domain->Plane_no * domain->Die_no * domain->Channel_no)) % domain->Chip_no)];
				targetAddress.DieID = domain->Die_ids[(unsigned int)((lpn / (domain->Plane_no * domain->Channel_no)) % domain->Die_no)];
				targetAddress.PlaneID = domain->Plane_ids[(unsigned int)((lpn / domain->Channel_no) % domain->Plane_no)];
				break;
				//Static: Way first
			case Flash_Plane_Allocation_Scheme_Type::WCDP:
				targetAddress.ChannelID = domain->Channel_ids[(unsigned int)((lpn / domain->Chip_no) % domain->Channel_no)];
				targetAddress.ChipID = domain->Chip_ids[(unsigned int)(lpn % domain->Chip_no)];
				targetAddress.DieID = domain->Die_ids[(unsigned int)((lpn / (domain->Chip_no * domain->Channel_no)) % domain->Die_no)];
				targetAddress.PlaneID = domain->Plane_ids[(unsigned int)((lpn / (domain->Chip_no * domain->Channel_no * domain->Die_no)) % domain->Plane_no)];
				break;
			case Flash_Plane_Allocation_Scheme_Type::WCPD:
				targetAddress.ChannelID = domain->Channel_ids[(unsigned int)((lpn / domain->Chip_no) % domain->Channel_no)];
				targetAddress.ChipID = domain->Chip_ids[(unsigned int)(lpn % domain->Chip_no)];
				targetAddress.DieID = domain->Die_ids[(unsigned int)((lpn / (domain->Chip_no * domain->Channel_no * domain->Plane_no)) % domain->Die_no)];
				targetAddress.PlaneID = domain->Plane_ids[(unsigned int)((lpn / (domain->Chip_no * domain->Channel_no)) % domain->Plane_no)];
				break;
			case Flash_Plane_Allocation_Scheme_Type::WDCP:
				targetAddress.ChannelID = domain->Channel_ids[(unsigned int)((lpn / (domain->Chip_no * domain->Die_no)) % domain->Channel_no)];
				targetAddress.ChipID = domain->Chip_ids[(unsigned int)(lpn % domain->Chip_no)];
				targetAddress.DieID = domain->Die_ids[(unsigned int)((lpn / domain->Chip_no) % domain->Die_no)];
				targetAddress.PlaneID = domain->Plane_ids[(unsigned int)((lpn / (domain->Chip_no * domain->Die_no * domain->Channel_no)) % domain->Plane_no)];
				break;
			case Flash_Plane_Allocation_Scheme_Type::WDPC:
				targetAddress.ChannelID = domain->Channel_ids[(unsigned int)((lpn / (domain->Chip_no * domain->Die_no * domain->Plane_no)) % domain->Channel_no)];
				targetAddress.ChipID = domain->Chip_ids[(unsigned int)(lpn % domain->Chip_no)];
				targetAddress.DieID = domain->Die_ids[(unsigned int)((lpn / domain->Chip_no) % domain->Die_no)];
				targetAddress.PlaneID = domain->Plane_ids[(unsigned int)((lpn / (domain->Chip_no * domain->Die_no)) % domain->Plane_no)];
				break;
			case Flash_Plane_Allocation_Scheme_Type::WPCD:
				targetAddress.ChannelID = domain->Channel_ids[(unsigned int)((lpn / (domain->Chip_no * domain->Plane_no)) % domain->Channel_no)];
				targetAddress.ChipID = domain->Chip_ids[(unsigned int)(lpn % domain->Chip_no)];
				targetAddress.DieID = domain->Die_ids[(unsigned int)((lpn / (domain->Chip_no * domain->Plane_no * domain->Channel_no)) % domain->Die_no)];
				targetAddress.PlaneID = domain->Plane_ids[(unsigned int)((lpn / domain->Chip_no) % domain->Plane_no)];
				break;
			case Flash_Plane_Allocation_Scheme_Type::WPDC:
				targetAddress.ChannelID = domain->Channel_ids[(unsigned int)((lpn / (domain->Chip_no * domain->Plane_no * domain->Die_no)) % domain->Channel_no)];
				targetAddress.ChipID = domain->Chip_ids[(unsigned int)(lpn % domain->Chip_no)];
				targetAddress.DieID = domain->Die_ids[(unsigned int)((lpn / (domain->Chip_no * domain->Plane_no)) % domain->Die_no)];
				targetAddress.PlaneID = domain->Plane_ids[(unsigned int)((lpn / domain->Chip_no) % domain->Plane_no)];
				break;
				//Static: Die first
			case Flash_Plane_Allocation_Scheme_Type::DCWP:
				targetAddress.ChannelID = domain->Channel_ids[(unsigned int)((lpn / domain->Die_no) % domain->Channel_no)];
				targetAddress.ChipID = domain->Chip_ids[(unsigned int)((lpn / (domain->Die_no * domain->Channel_no)) % domain->Chip_no)];
				targetAddress.DieID = domain->Die_ids[(unsigned int)(lpn % domain->Die_no)];
				targetAddress.PlaneID = domain->Plane_ids[(unsigned int)((lpn / (domain->Die_no * domain->Channel_no * domain->Chip_no)) % domain->Plane_no)];
				break;
			case Flash_Plane_Allocation_Scheme_Type::DCPW:
				targetAddress.ChannelID = domain->Channel_ids[(unsigned int)((lpn / domain->Die_no) % domain->Channel_no)];
				targetAddress.ChipID = domain->Chip_ids[(unsigned int)((lpn / (domain->Die_no * domain->Channel_no * domain->Plane_no)) % domain->Chip_no)];
				targetAddress.DieID = domain->Die_ids[(unsigned int)(lpn % domain->Die_no)];
				targetAddress.PlaneID = domain->Plane_ids[(unsigned int)((lpn / (domain->Die_no * domain->Channel_no)) % domain->Plane_no)];
				break;
			case Flash_Plane_Allocation_Scheme_Type::DWCP:
				targetAddress.ChannelID = domain->Channel_ids[(unsigned int)((lpn / (domain->Die_no * domain->Chip_no)) % domain->Channel_no)];
				targetAddress.ChipID = domain->Chip_ids[(unsigned int)((lpn / domain->Die_no) % domain->Chip_no)];
				targetAddress.DieID = domain->Die_ids[(unsigned int)(lpn % domain->Die_no)];
				targetAddress.PlaneID = domain->Plane_ids[(unsigned int)((lpn / (domain->Die_no * domain->Chip_no * domain->Channel_no)) % domain->Plane_no)];
				break;
			case Flash_Plane_Allocation_Scheme_Type::DWPC:
				targetAddress.ChannelID = domain->Channel_ids[(unsigned int)((lpn / (domain->Die_no * domain->Chip_no * domain->Plane_no)) % domain->Channel_no)];
				targetAddress.ChipID = domain->Chip_ids[(unsigned int)((lpn / domain->Die_no) % domain->Chip_no)];
				targetAddress.DieID = domain->Die_ids[(unsigned int)(lpn % domain->Die_no)];
				targetAddress.PlaneID = domain->Plane_ids[(unsigned int)((lpn / (domain->Die_no * domain->Chip_no)) % domain->Plane_no)];
				break;
			case Flash_Plane_Allocation_Scheme_Type::DPCW:
				targetAddress.ChannelID = domain->Channel_ids[(unsigned int)((lpn / (domain->Die_no * domain->Plane_no)) % domain->Channel_no)];
				targetAddress.ChipID = domain->Chip_ids[(unsigned int)((lpn / (domain->Die_no * domain->Plane_no * domain->Channel_no)) % domain->Chip_no)];
				targetAddress.DieID = domain->Die_ids[(unsigned int)(lpn % domain->Die_no)];
				targetAddress.PlaneID = domain->Plane_ids[(unsigned int)((lpn / domain->Die_no) % domain->Plane_no)];
				break;
			case Flash_Plane_Allocation_Scheme_Type::DPWC:
				targetAddress.ChannelID = domain->Channel_ids[(unsigned int)((lpn / (domain->Die_no * domain->Plane_no * domain->Chip_no)) % domain->Channel_no)];
				targetAddress.ChipID = domain->Chip_ids[(unsigned int)((lpn / (domain->Die_no * domain->Plane_no)) % domain->Chip_no)];
				targetAddress.DieID = domain->Die_ids[(unsigned int)(lpn % domain->Die_no)];
				targetAddress.PlaneID = domain->Plane_ids[(unsigned int)((lpn / domain->Die_no) % domain->Plane_no)];
				break;
				//Static: Plane first
			case Flash_Plane_Allocation_Scheme_Type::PCWD:
				targetAddress.ChannelID = domain->Channel_ids[(unsigned int)((lpn / domain->Plane_no) % domain->Channel_no)];
				targetAddress.ChipID = domain->Chip_ids[(unsigned int)((lpn / (domain->Plane_no * domain->Channel_no)) % domain->Chip_no)];
				targetAddress.DieID = domain->Die_ids[(unsigned int)((lpn / (domain->Plane_no * domain->Channel_no * domain->Chip_no)) % domain->Die_no)];
				targetAddress.PlaneID = domain->Plane_ids[(unsigned int)(lpn % domain->Plane_no)];
				break;
			case Flash_Plane_Allocation_Scheme_Type::PCDW:
				targetAddress.ChannelID = domain->Channel_ids[(unsigned int)((lpn / domain->Plane_no) % domain->Channel_no)];
				targetAddress.ChipID = domain->Chip_ids[(unsigned int)((lpn / (domain->Plane_no * domain->Channel_no * domain->Die_no)) % domain->Chip_no)];
				targetAddress.DieID = domain->Die_ids[(unsigned int)((lpn / (domain->Plane_no * domain->Channel_no)) % domain->Die_no)];
				targetAddress.PlaneID = domain->Plane_ids[(unsigned int)(lpn % domain->Plane_no)];
				break;
			case Flash_Plane_Allocation_Scheme_Type::PWCD:
				targetAddress.ChannelID = domain->Channel_ids[(unsigned int)((lpn / (domain->Plane_no * domain->Chip_no)) % domain->Channel_no)];
				targetAddress.ChipID = domain->Chip_ids[(unsigned int)((lpn / domain->Plane_no) % domain->Chip_no)];
				targetAddress.DieID = domain->Die_ids[(unsigned int)((lpn / (domain->Plane_no * domain->Chip_no * domain->Channel_no)) % domain->Die_no)];
				targetAddress.PlaneID = domain->Plane_ids[(unsigned int)(lpn % domain->Plane_no)];
				break;
			case Flash_Plane_Allocation_Scheme_Type::PWDC:
				targetAddress.ChannelID = domain->Channel_ids[(unsigned int)((lpn / (domain->Plane_no * domain->Chip_no * domain->Die_no)) % domain->Channel_no)];
				targetAddress.ChipID = domain->Chip_ids[(unsigned int)((lpn / domain->Plane_no) % domain->Chip_no)];
				targetAddress.DieID = domain->Die_ids[(unsigned int)((lpn / (domain->Plane_no * domain->Chip_no)) % domain->Die_no)];
				targetAddress.PlaneID = domain->Plane_ids[(unsigned int)(lpn % domain->Plane_no)];
				break;
			case Flash_Plane_Allocation_Scheme_Type::PDCW:
				targetAddress.ChannelID = domain->Channel_ids[(unsigned int)((lpn / (domain->Plane_no * domain->Die_no)) % domain->Channel_no)];
				targetAddress.ChipID = domain->Chip_ids[(unsigned int)((lpn / (domain->Plane_no * domain->Die_no * domain->Channel_no)) % domain->Chip_no)];
				targetAddress.DieID = domain->Die_ids[(unsigned int)((lpn / domain->Plane_no) % domain->Die_no)];
				targetAddress.PlaneID = domain->Plane_ids[(unsigned int)(lpn % domain->Plane_no)];
				break;
			case Flash_Plane_Allocation_Scheme_Type::PDWC:
				targetAddress.ChannelID = domain->Channel_ids[(unsigned int)((lpn / (domain->Plane_no * domain->Die_no * domain->Chip_no)) % domain->Channel_no)];
				targetAddress.ChipID = domain->Chip_ids[(unsigned int)((lpn / (domain->Plane_no * domain->Die_no)) % domain->Chip_no)];
				targetAddress.DieID = domain->Die_ids[(unsigned int)((lpn / domain->Plane_no) % domain->Die_no)];
				targetAddress.PlaneID = domain->Plane_ids[(unsigned int)(lpn % domain->Plane_no)];
				break;
			default:
				PRINT_ERROR("Unknown plane allocation scheme type!")
		}
	}

	void Address_Mapping_Unit_Zone_Level::allocate_plane_for_user_write(NVM_Transaction_Flash_WR* transaction)
	{
		LPA_type lpn = transaction->LPA;
		NVM::FlashMemory::Physical_Page_Address& targetAddress = transaction->Address;
		AddressMappingDomain* domain = domains[transaction->Stream_id];

		unsigned int zone_size_in_byte, block_size_in_byte, zone_p_level, total_level;
		unsigned int zoneOffset, subzoneID, subzoneOffset, blockID, blockOffset, pageID;

		zone_size_in_byte = fzm->zone_size * 1024 * 1024;
		block_size_in_byte = pages_no_per_block * page_size_in_byte;
		zone_p_level = domain->Channel_No_Per_Zone * domain->Chip_No_Per_Zone * domain->Die_No_Per_Zone * domain->Plane_No_Per_Zone;
		// This zone_p_level is the same as the number of subzones per zone
		total_level = domain->Channel_no * domain->Chip_no * domain->Die_no * domain->Plane_no;


		//unsigned int subzone_no_per_zone 
		unsigned int block_no_per_subzone = zone_size_in_byte / zone_p_level / block_size_in_byte;
		unsigned int page_no_per_zone = zone_size_in_byte / page_size_in_byte;
		
		Zone_ID_type zoneID = lpn / page_no_per_zone; 
		zoneOffset = lpn % page_no_per_zone;

		unsigned int index;
		switch (domain->ZoneAllocationScheme) {
			case Zone_Allocation_Scheme_Type::CDPW:
				if (domain->Channel_No_Per_Zone > 1) {	// one zone is spread to two channels at least
					if (domain->Channel_No_Per_Zone == channel_count && 
						domain->Chip_No_Per_Zone == chip_no_per_channel && 
						domain->Die_No_Per_Zone == die_no_per_chip && 
						domain->Plane_No_Per_Zone == plane_no_per_die) { // maximum parallelism
							//std::cout << "nysong - this is the maximum parallelism in one zone" << std::endl;
							//blockID = zoneOffset / block_size_in_byte;
							pageID = zoneOffset ;
							
							index = pageID;
							targetAddress.ChannelID = domain->Channel_ids[(unsigned int)(index % channel_count)];
							targetAddress.ChipID = domain->Chip_ids[(unsigned int)((index / (channel_count * die_no_per_chip * plane_no_per_die)) % chip_no_per_channel)];
							targetAddress.DieID = domain->Die_ids[(unsigned int)(index / channel_count % die_no_per_chip)];
							targetAddress.PlaneID = domain->Plane_ids[(unsigned int)(index / (channel_count*die_no_per_chip) % plane_no_per_die)];
							// targetAddress.BlockID = (unsigned int)((zoneID * block_no_per_subzone) + ((pageID / total_level) / pages_no_per_block));
							// targetAddress.PageID = (unsigned int)((pageID / total_level) % pages_no_per_block);
							targetAddress.BlockID = (unsigned int)((lpn / total_level) / pages_no_per_block); //TODO:can I use lpn here?
							targetAddress.PageID = (unsigned int)((lpn / total_level) % pages_no_per_block);
						}
				}
				else if (domain->Chip_No_Per_Zone > 1) {

				} 
				else if (domain->Die_No_Per_Zone > 1) {

				}
				else if (domain->Plane_No_Per_Zone > 1) {

				}
				else {	// 1*1*1*1 = minimum parallelism in one zone, zone_p_level is 1, we will use only one channel, one chip, one die, one plane. That means, one zone's subzone and blocks are contiguous in one plane. 
					//std::cout << "nysong - This it the minimum parallelism in one zone" << std::endl;
					
					blockID = zoneOffset / pages_no_per_block;
					pageID = zoneOffset % pages_no_per_block;

					index = zoneID;
					targetAddress.ChannelID = domain->Channel_ids[(unsigned int)(index % channel_count)];
					targetAddress.ChipID = domain->Chip_ids[(unsigned int)((index / (channel_count * die_no_per_chip * plane_no_per_die)) % chip_no_per_channel)];
					targetAddress.DieID = domain->Die_ids[(unsigned int)(index / channel_count % die_no_per_chip)];
					targetAddress.PlaneID = domain->Plane_ids[(unsigned int)(index / (channel_count*die_no_per_chip) % plane_no_per_die)]; 
					//targetAddress.BlockID = (unsigned int)((zoneID / total_level) + blockID);
					targetAddress.BlockID = (unsigned int) blockID;
					targetAddress.PageID = pageID;
				}
				break;
			default:
				PRINT_ERROR("Unknown zone allocation scheme type!")
		}
	}

	void Address_Mapping_Unit_Zone_Level::allocate_page_in_plane_for_user_write(NVM_Transaction_Flash_WR* transaction, bool is_for_gc)
	{
		AddressMappingDomain* domain = domains[transaction->Stream_id];
		PPA_type old_ppa = domain->Get_ppa(ideal_mapping_table, transaction->Stream_id, transaction->LPA);

		if (old_ppa == NO_PPA)  /*this is the first access to the logical page*/
		{
			if (is_for_gc) {
				PRINT_ERROR("Unexpected mapping table status in allocate_page_in_plane_for_user_write function for a GC/WL write!")
			}
		} else {
			if (is_for_gc) {
				NVM::FlashMemory::Physical_Page_Address addr;
				Convert_ppa_to_address(old_ppa, addr);
				block_manager->Invalidate_page_in_block(transaction->Stream_id, addr);
				page_status_type page_status_in_cmt = domain->Get_page_status(ideal_mapping_table, transaction->Stream_id, transaction->LPA);
				if (page_status_in_cmt != transaction->write_sectors_bitmap)
					PRINT_ERROR("Unexpected mapping table status in allocate_page_in_plane_for_user_write for a GC/WL write!")
			} else {
				page_status_type prev_page_status = domain->Get_page_status(ideal_mapping_table, transaction->Stream_id, transaction->LPA);
				page_status_type status_intersection = transaction->write_sectors_bitmap & prev_page_status;
				//check if an update read is required
				if (status_intersection == prev_page_status) {
					NVM::FlashMemory::Physical_Page_Address addr;
					Convert_ppa_to_address(old_ppa, addr);
					block_manager->Invalidate_page_in_block(transaction->Stream_id, addr);
				} else {
					page_status_type read_pages_bitmap = status_intersection ^ prev_page_status;
					NVM_Transaction_Flash_RD *update_read_tr = new NVM_Transaction_Flash_RD(transaction->Source, transaction->Stream_id,
						count_sector_no_from_status_bitmap(read_pages_bitmap) * SECTOR_SIZE_IN_BYTE, transaction->LPA, old_ppa, transaction->UserIORequest,
						transaction->Content, transaction, read_pages_bitmap, domain->GlobalMappingTable[transaction->LPA].TimeStamp);
					Convert_ppa_to_address(old_ppa, update_read_tr->Address);
					block_manager->Read_transaction_issued(update_read_tr->Address);//Inform block manager about a new transaction as soon as the transaction's target address is determined
					block_manager->Invalidate_page_in_block(transaction->Stream_id, update_read_tr->Address);
					transaction->RelatedRead = update_read_tr;
				}
			}
		}

		/*The following lines should not be ordered with respect to the block_manager->Invalidate_page_in_block
		* function call in the above code blocks. Otherwise, GC may be invoked (due to the call to Allocate_block_....) and
		* may decide to move a page that is just invalidated.*/
		if (is_for_gc) {
			block_manager->Allocate_block_and_page_in_plane_for_gc_write(transaction->Stream_id, transaction->Address);
		} else {
			//block_manager->Allocate_block_and_page_in_plane_for_user_write(transaction->Stream_id, transaction->Address);
			block_manager->Allocate_block_and_page_in_plane_for_user_write_in_Zone(transaction->Stream_id, transaction->Address);
		}
		transaction->PPA = Convert_address_to_ppa(transaction->Address);
		domain->Update_mapping_info(ideal_mapping_table, transaction->Stream_id, transaction->LPA, transaction->PPA,
			((NVM_Transaction_Flash_WR*)transaction)->write_sectors_bitmap | domain->Get_page_status(ideal_mapping_table, transaction->Stream_id, transaction->LPA));
	}

	void Address_Mapping_Unit_Zone_Level::allocate_plane_for_translation_write(NVM_Transaction_Flash* transaction)
	{
		allocate_plane_for_user_write((NVM_Transaction_Flash_WR*)transaction);
	}

	void Address_Mapping_Unit_Zone_Level::allocate_page_in_plane_for_translation_write(NVM_Transaction_Flash* transaction, MVPN_type mvpn, bool is_for_gc)
	{
		AddressMappingDomain* domain = domains[transaction->Stream_id];

		MPPN_type old_MPPN = domain->GlobalTranslationDirectory[mvpn].MPPN;
		/*this is the first access to the mvpn*/
		if (old_MPPN == NO_MPPN) {
			if (is_for_gc) {
				PRINT_ERROR("Unexpected mapping table status in allocate_page_in_plane_for_translation_write for a GC/WL write!")
			}
		} else {
			NVM::FlashMemory::Physical_Page_Address prevAddr;
			Convert_ppa_to_address(old_MPPN, prevAddr);
			block_manager->Invalidate_page_in_block(transaction->Stream_id, prevAddr);
		}

		block_manager->Allocate_block_and_page_in_plane_for_translation_write(transaction->Stream_id, transaction->Address, false);
		transaction->PPA = Convert_address_to_ppa(transaction->Address);
		domain->GlobalTranslationDirectory[mvpn].MPPN = (MPPN_type)transaction->PPA;
		domain->GlobalTranslationDirectory[mvpn].TimeStamp = CurrentTimeStamp;
	}

	PPA_type Address_Mapping_Unit_Zone_Level::online_create_entry_for_reads(LPA_type lpa, const stream_id_type stream_id, NVM::FlashMemory::Physical_Page_Address& read_address, uint64_t read_sectors_bitmap)
	{
		AddressMappingDomain* domain = domains[stream_id];

		unsigned int zone_size_in_byte, block_size_in_byte, zone_p_level, total_level;
		unsigned int zoneOffset, subzoneID, subzoneOffset, blockID, blockOffset, pageID;

		zone_size_in_byte = fzm->zone_size * 1024 * 1024;
		block_size_in_byte = pages_no_per_block * page_size_in_byte;
		zone_p_level = domain->Channel_No_Per_Zone * domain->Chip_No_Per_Zone * domain->Die_No_Per_Zone * domain->Plane_No_Per_Zone;
		total_level = domain->Channel_no * domain->Chip_no * domain->Die_no * domain->Plane_no;

		Zone_ID_type zoneID = lpa / zone_size_in_byte;
		zoneOffset = lpa % zone_size_in_byte;

		//unsigned int subzone_no_per_zone 
		unsigned int block_no_per_subzone = zone_size_in_byte / zone_p_level / block_size_in_byte;

		unsigned int index;	

		switch (domain->ZoneAllocationScheme) {
			case Zone_Allocation_Scheme_Type::CDPW:
				if (domain->Channel_No_Per_Zone > 1) {	// one zone is spread to two channels at least
					if (domain->Channel_No_Per_Zone == channel_count && 
						domain->Chip_No_Per_Zone == chip_no_per_channel && 
						domain->Die_No_Per_Zone == die_no_per_chip && 
						domain->Plane_No_Per_Zone == plane_no_per_die) { // maximum parallelism
							//blockID = zoneOffset / block_size_in_byte;
							pageID = zoneOffset / page_size_in_byte;

							index = pageID;
							read_address.ChannelID = domain->Channel_ids[(unsigned int)(index % channel_count)];
							read_address.ChipID = domain->Chip_ids[(unsigned int)((index / (channel_count * die_no_per_chip * plane_no_per_die)) % chip_no_per_channel)];
							read_address.DieID = domain->Die_ids[(unsigned int)(index / channel_count % die_no_per_chip)];
							read_address.PlaneID = domain->Plane_ids[(unsigned int)(index / (channel_count*die_no_per_chip) % plane_no_per_die)];
							read_address.BlockID = (unsigned int)((zoneID * block_no_per_subzone) + ((pageID / total_level) / pages_no_per_block));
							read_address.PageID = (unsigned int)((pageID / total_level) % pages_no_per_block);
						}

				}
				else if (domain->Chip_No_Per_Zone > 1) {

				} 
				else if (domain->Die_No_Per_Zone > 1) {

				}
				else if (domain->Plane_No_Per_Zone > 1) {

				}
				else {	// 1*1*1*1 = minimum parallelism in one zone, zone_p_level is 1 
					blockID = zoneOffset / zone_size_in_byte;
					pageID = (zoneOffset % zone_size_in_byte) / page_size_in_byte;

					index = zoneID;
					read_address.ChannelID = domain->Channel_ids[(unsigned int)(index % channel_count)];
					read_address.ChipID = domain->Chip_ids[(unsigned int)((index / (channel_count * die_no_per_chip * plane_no_per_die)) % chip_no_per_channel)];
					read_address.DieID = domain->Die_ids[(unsigned int)(index / channel_count % die_no_per_chip)];
					read_address.PlaneID = domain->Plane_ids[(unsigned int)(index / (channel_count*die_no_per_chip) % plane_no_per_die)]; 
					read_address.BlockID = (unsigned int)((zoneID / total_level) + blockID);
					read_address.PageID = pageID;
				}
			default:
				PRINT_ERROR("Unknown plane allocation scheme type!")
		}

		block_manager->Allocate_block_and_page_in_plane_for_user_write_in_Zone(stream_id, read_address);
		PPA_type ppa = Convert_address_to_ppa(read_address);
		domain->Update_mapping_info(ideal_mapping_table, stream_id, lpa, ppa, read_sectors_bitmap);

		return ppa;
	}

	inline void Address_Mapping_Unit_Zone_Level::Get_data_mapping_info_for_gc(const stream_id_type stream_id, const LPA_type lpa, PPA_type& ppa, page_status_type& page_state)
	{
		if (domains[stream_id]->Mapping_entry_accessible(ideal_mapping_table, stream_id, lpa)) {
			ppa = domains[stream_id]->Get_ppa(ideal_mapping_table, stream_id, lpa);
			page_state = domains[stream_id]->Get_page_status(ideal_mapping_table, stream_id, lpa);
		} else {
			ppa = domains[stream_id]->GlobalMappingTable[lpa].PPA;
			page_state = domains[stream_id]->GlobalMappingTable[lpa].WrittenStateBitmap;
		}
	}

	inline void Address_Mapping_Unit_Zone_Level::Get_translation_mapping_info_for_gc(const stream_id_type stream_id, const MVPN_type mvpn, MPPN_type& mppa, sim_time_type& timestamp)
	{
		mppa = domains[stream_id]->GlobalTranslationDirectory[mvpn].MPPN;
		timestamp = domains[stream_id]->GlobalTranslationDirectory[mvpn].TimeStamp;
	}

	inline MVPN_type Address_Mapping_Unit_Zone_Level::get_MVPN(const LPA_type lpn, stream_id_type stream_id)
	{
		//return (MVPN_type)((lpn % (domains[stream_id]->Total_logical_pages_no)) / no_of_translation_entries_per_page);
		return (MVPN_type)(lpn / no_of_translation_entries_per_page);
	}

	inline LPA_type Address_Mapping_Unit_Zone_Level::get_start_LPN_in_MVP(const MVPN_type mvpn)
	{
		return (MVPN_type)(mvpn * no_of_translation_entries_per_page);
	}

	inline LPA_type Address_Mapping_Unit_Zone_Level::get_end_LPN_in_MVP(const MVPN_type mvpn)
	{
		return (MVPN_type)(mvpn * no_of_translation_entries_per_page + no_of_translation_entries_per_page - 1);
	}

	LPA_type Address_Mapping_Unit_Zone_Level::Get_logical_pages_count(stream_id_type stream_id)
	{
		return this->domains[stream_id]->Total_logical_pages_no;
	}
	
	inline NVM::FlashMemory::Physical_Page_Address Address_Mapping_Unit_Zone_Level::Convert_ppa_to_address(const PPA_type ppa)
	{
		NVM::FlashMemory::Physical_Page_Address target;
		target.ChannelID = (flash_channel_ID_type)(ppa / page_no_per_channel);
		target.ChipID = (flash_chip_ID_type)((ppa % page_no_per_channel) / page_no_per_chip);
		target.DieID = (flash_die_ID_type)(((ppa % page_no_per_channel) % page_no_per_chip) / page_no_per_die);
		target.PlaneID = (flash_plane_ID_type)((((ppa % page_no_per_channel) % page_no_per_chip) % page_no_per_die) / page_no_per_plane);
		target.BlockID = (flash_block_ID_type)(((((ppa % page_no_per_channel) % page_no_per_chip) % page_no_per_die) % page_no_per_plane) / pages_no_per_block);
		target.PageID = (flash_page_ID_type)((((((ppa % page_no_per_channel) % page_no_per_chip) % page_no_per_die) % page_no_per_plane) % pages_no_per_block) % pages_no_per_block);

		return target;
	}

	inline void Address_Mapping_Unit_Zone_Level::Convert_ppa_to_address(const PPA_type ppn, NVM::FlashMemory::Physical_Page_Address& address)
	{
		address.ChannelID = (flash_channel_ID_type)(ppn / page_no_per_channel);
		address.ChipID = (flash_chip_ID_type)((ppn % page_no_per_channel) / page_no_per_chip);
		address.DieID = (flash_die_ID_type)(((ppn % page_no_per_channel) % page_no_per_chip) / page_no_per_die);
		address.PlaneID = (flash_plane_ID_type)((((ppn % page_no_per_channel) % page_no_per_chip) % page_no_per_die) / page_no_per_plane);
		address.BlockID = (flash_block_ID_type)(((((ppn % page_no_per_channel) % page_no_per_chip) % page_no_per_die) % page_no_per_plane) / pages_no_per_block);
		address.PageID = (flash_page_ID_type)((((((ppn % page_no_per_channel) % page_no_per_chip) % page_no_per_die) % page_no_per_plane) % pages_no_per_block) % pages_no_per_block);

	}

	inline PPA_type Address_Mapping_Unit_Zone_Level::Convert_address_to_ppa(const NVM::FlashMemory::Physical_Page_Address& pageAddress)
	{
		return (PPA_type)this->page_no_per_chip * (PPA_type)(pageAddress.ChannelID * this->chip_no_per_channel + pageAddress.ChipID)
			+ this->page_no_per_die * pageAddress.DieID + this->page_no_per_plane * pageAddress.PlaneID
			+ this->pages_no_per_block * pageAddress.BlockID + pageAddress.PageID;
	}

	bool Address_Mapping_Unit_Zone_Level::request_mapping_entry(const stream_id_type stream_id, const LPA_type lpa)
	{
		AddressMappingDomain* domain = domains[stream_id];
		MVPN_type mvpn = get_MVPN(lpa, stream_id);

		/*This is the first time that a user request accesses this address.
		Just create an entry in cache! No flash read is needed.*/
		if (domain->GlobalTranslationDirectory[mvpn].MPPN == NO_MPPN) {
			if (!domain->CMT->Check_free_slot_availability()) {
				LPA_type evicted_lpa;
				CMTSlotType evictedItem = domain->CMT->Evict_one_slot(evicted_lpa);
				if (evictedItem.Dirty) {
					/* In order to eliminate possible race conditions for the requests that
					* will access the evicted lpa in the near future (before the translation
					* write finishes), MQSim updates GMT (the on flash mapping table) right
					* after eviction happens.*/
					domain->GlobalMappingTable[evicted_lpa].PPA = evictedItem.PPA;
					domain->GlobalMappingTable[evicted_lpa].WrittenStateBitmap = evictedItem.WrittenStateBitmap;
					if (domain->GlobalMappingTable[evicted_lpa].TimeStamp > CurrentTimeStamp)
						throw std::logic_error("Unexpected situation occurred in handling GMT!");
					domain->GlobalMappingTable[evicted_lpa].TimeStamp = CurrentTimeStamp;
					generate_flash_writeback_request_for_mapping_data(stream_id, evicted_lpa);
				}
			}
			domain->CMT->Reserve_slot_for_lpn(stream_id, lpa);
			domain->CMT->Insert_new_mapping_info(stream_id, lpa, NO_PPA, UNWRITTEN_LOGICAL_PAGE);
			
			return true;
		}

		/*A read transaction is already under process to retrieve the MVP content.
		* This situation may happen in two different cases:
		* 1. A read has been issued to retrieve unchanged parts of the mapping data and merge them
		*     with the changed parts (i.e., an update read of MVP). This read will be followed
		*     by a writeback of MVP content to a new flash page.
		* 2. A read has been issued to retrieve the mapping data for some previous user requests*/
		if (domain->ArrivingMappingEntries.find(mvpn) != domain->ArrivingMappingEntries.end())
		{
			if (domain->CMT->Is_slot_reserved_for_lpn_and_waiting(stream_id, lpa)) {
				return false;
			} else { //An entry should be created in the cache
				if (!domain->CMT->Check_free_slot_availability()) {
					LPA_type evicted_lpa;
					CMTSlotType evictedItem = domain->CMT->Evict_one_slot(evicted_lpa);
					if (evictedItem.Dirty) {
						/* In order to eliminate possible race conditions for the requests that
						* will access the evicted lpa in the near future (before the translation
						* write finishes), MQSim updates GMT (the on flash mapping table) right
						* after eviction happens.*/
						domain->GlobalMappingTable[evicted_lpa].PPA = evictedItem.PPA;
						domain->GlobalMappingTable[evicted_lpa].WrittenStateBitmap = evictedItem.WrittenStateBitmap;
						if (domain->GlobalMappingTable[evicted_lpa].TimeStamp > CurrentTimeStamp)
							throw std::logic_error("Unexpected situation occured in handling GMT!");
						domain->GlobalMappingTable[evicted_lpa].TimeStamp = CurrentTimeStamp;
						generate_flash_writeback_request_for_mapping_data(stream_id, evicted_lpa);
					}
				}
				domain->CMT->Reserve_slot_for_lpn(stream_id, lpa);
				domain->ArrivingMappingEntries.insert(std::pair<MVPN_type, LPA_type>(mvpn, lpa));

				return false;
			}
		}

		/*MQSim assumes that the data of all departing (evicted from CMT) translation pages are in memory, until
		the flash program operation finishes and the entry it is cleared from DepartingMappingEntries.*/
		if (domain->DepartingMappingEntries.find(mvpn) != domain->DepartingMappingEntries.end()) {
			if (!domain->CMT->Check_free_slot_availability()) {
				LPA_type evicted_lpa;
				CMTSlotType evictedItem = domain->CMT->Evict_one_slot(evicted_lpa);
				if (evictedItem.Dirty) {
					/* In order to eliminate possible race conditions for the requests that
					* will access the evicted lpa in the near future (before the translation
					* write finishes), MQSim updates GMT (the on flash mapping table) right
					* after eviction happens.*/
					domain->GlobalMappingTable[evicted_lpa].PPA = evictedItem.PPA;
					domain->GlobalMappingTable[evicted_lpa].WrittenStateBitmap = evictedItem.WrittenStateBitmap;
					if (domain->GlobalMappingTable[evicted_lpa].TimeStamp > CurrentTimeStamp)
						throw std::logic_error("Unexpected situation occured in handling GMT!");
					domain->GlobalMappingTable[lpa].TimeStamp = CurrentTimeStamp;
					generate_flash_writeback_request_for_mapping_data(stream_id, evicted_lpa);
				}
			}
			domain->CMT->Reserve_slot_for_lpn(stream_id, lpa);
			/*Hack: since we do not actually save the values of translation requests, we copy the mapping
			data from GlobalMappingTable (which actually must be stored on flash)*/
			domain->CMT->Insert_new_mapping_info(stream_id, lpa,
				domain->GlobalMappingTable[lpa].PPA, domain->GlobalMappingTable[lpa].WrittenStateBitmap);
			
			return true;
		}

		//Non of the above options provide mapping data. So, MQSim, must read the translation data from flash memory
		if (!domain->CMT->Check_free_slot_availability()) {
			LPA_type evicted_lpa;
			CMTSlotType evictedItem = domain->CMT->Evict_one_slot(evicted_lpa);
			if (evictedItem.Dirty) {
				/* In order to eliminate possible race conditions for the requests that
				* will access the evicted lpa in the near future (before the translation
				* write finishes), MQSim updates GMT (the on flash mapping table) right
				* after eviction happens.*/
				domain->GlobalMappingTable[evicted_lpa].PPA = evictedItem.PPA;
				domain->GlobalMappingTable[evicted_lpa].WrittenStateBitmap = evictedItem.WrittenStateBitmap;
				if (domain->GlobalMappingTable[evicted_lpa].TimeStamp > CurrentTimeStamp) {
					throw std::logic_error("Unexpected situation occured in handling GMT!");
				}
				domain->GlobalMappingTable[evicted_lpa].TimeStamp = CurrentTimeStamp;
				generate_flash_writeback_request_for_mapping_data(stream_id, evicted_lpa);
			}
		}
		domain->CMT->Reserve_slot_for_lpn(stream_id, lpa);
		generate_flash_read_request_for_mapping_data(stream_id, lpa);//consult GTD and create read transaction
		
		return false;
	}

	void Address_Mapping_Unit_Zone_Level::generate_flash_writeback_request_for_mapping_data(const stream_id_type stream_id, const LPA_type lpn)
	{
		MVPN_type mvpn = get_MVPN(lpn, stream_id);
		if (is_mvpn_locked_for_gc(stream_id, mvpn)) {
			manage_mapping_transaction_facing_barrier(stream_id, mvpn, false);
			domains[stream_id]->DepartingMappingEntries.insert(get_MVPN(lpn, stream_id));
		} else {
			ftl->TSU->Prepare_for_transaction_submit();

			//Writing back all dirty CMT entries that fall into the same translation virtual page (MVPN)
			unsigned int read_size = 0;
			page_status_type readSectorsBitmap = 0;
			LPA_type startLPN = get_start_LPN_in_MVP(mvpn);
			LPA_type endLPN = get_end_LPN_in_MVP(mvpn);
			for (LPA_type lpn_itr = startLPN; lpn_itr <= endLPN; lpn_itr++) {
				if (domains[stream_id]->CMT->Exists(stream_id, lpn_itr)) {
					if (domains[stream_id]->CMT->Is_dirty(stream_id, lpn_itr)) {
						domains[stream_id]->CMT->Make_clean(stream_id, lpn_itr);
						domains[stream_id]->GlobalMappingTable[lpn_itr].PPA = domains[stream_id]->CMT->Retrieve_ppa(stream_id, lpn_itr);
					} else {
						page_status_type bitlocation = (((page_status_type)0x1) << (((lpn_itr - startLPN) * GTD_entry_size) / SECTOR_SIZE_IN_BYTE));
						if ((readSectorsBitmap & bitlocation) == 0) {
							readSectorsBitmap |= bitlocation;
							read_size += SECTOR_SIZE_IN_BYTE;
						}
					}
				}
			}

			//Read the unchaged mapping entries from flash to merge them with updated parts of MVPN
			NVM_Transaction_Flash_RD* readTR = NULL;
			MPPN_type mppn = domains[stream_id]->GlobalTranslationDirectory[mvpn].MPPN;
			if (mppn != NO_MPPN) {
				readTR = new NVM_Transaction_Flash_RD(Transaction_Source_Type::MAPPING, stream_id, read_size,
					mvpn, mppn, NULL, mvpn, NULL, readSectorsBitmap, CurrentTimeStamp);
				Convert_ppa_to_address(mppn, readTR->Address);
				block_manager->Read_transaction_issued(readTR->Address);//Inform block_manager as soon as the transaction's target address is determined
				domains[stream_id]->ArrivingMappingEntries.insert(std::pair<MVPN_type, LPA_type>(mvpn, lpn));
				ftl->TSU->Submit_transaction(readTR);
			}

			NVM_Transaction_Flash_WR* writeTR = new NVM_Transaction_Flash_WR(Transaction_Source_Type::MAPPING, stream_id, SECTOR_SIZE_IN_BYTE * sector_no_per_page,
				mvpn, mppn, NULL, mvpn, readTR, (((page_status_type)0x1) << sector_no_per_page) - 1, CurrentTimeStamp);
			allocate_plane_for_translation_write(writeTR);
			allocate_page_in_plane_for_translation_write(writeTR, mvpn, false);
			domains[stream_id]->DepartingMappingEntries.insert(get_MVPN(lpn, stream_id));
			ftl->TSU->Submit_transaction(writeTR);

			Stats::Total_flash_reads_for_mapping++;
			Stats::Total_flash_writes_for_mapping++;
			Stats::Total_flash_reads_for_mapping_per_stream[stream_id]++;
			Stats::Total_flash_writes_for_mapping_per_stream[stream_id]++;

			ftl->TSU->Schedule();
		}
	}

	void Address_Mapping_Unit_Zone_Level::generate_flash_read_request_for_mapping_data(const stream_id_type stream_id, const LPA_type lpn)
	{
		MVPN_type mvpn = get_MVPN(lpn, stream_id);

		if (mvpn >= domains[stream_id]->Total_translation_pages_no) {
			PRINT_ERROR("Out of range virtual translation page number!")
		}

		domains[stream_id]->ArrivingMappingEntries.insert(std::pair<MVPN_type, LPA_type>(mvpn, lpn));

		if (is_mvpn_locked_for_gc(stream_id, mvpn)) {
			manage_mapping_transaction_facing_barrier(stream_id, mvpn, true);
		} else {
			ftl->TSU->Prepare_for_transaction_submit();

			PPA_type ppn = domains[stream_id]->GlobalTranslationDirectory[mvpn].MPPN;

			if (ppn == NO_MPPN){
				PRINT_ERROR("Reading an invalid physical flash page address in function generate_flash_read_request_for_mapping_data!")
			}

			NVM_Transaction_Flash_RD* readTR = new NVM_Transaction_Flash_RD(Transaction_Source_Type::MAPPING, stream_id,
					SECTOR_SIZE_IN_BYTE, NO_LPA, NO_PPA, NULL, mvpn, ((page_status_type)0x1) << sector_no_per_page, CurrentTimeStamp);
			Convert_ppa_to_address(ppn, readTR->Address);
			block_manager->Read_transaction_issued(readTR->Address);//Inform block_manager as soon as the transaction's target address is determined
			readTR->PPA = ppn;
			ftl->TSU->Submit_transaction(readTR);

			Stats::Total_flash_reads_for_mapping++;
			Stats::Total_flash_reads_for_mapping_per_stream[stream_id]++;

			ftl->TSU->Schedule();
		}
	}

	inline void Address_Mapping_Unit_Zone_Level::handle_transaction_serviced_signal_from_PHY(NVM_Transaction_Flash* transaction)
	{
		//First check if the transaction source is Mapping Module
		if (transaction->Source != Transaction_Source_Type::MAPPING) {
			return;
		}

		if (_my_instance->ideal_mapping_table){
			throw std::logic_error("There should not be any flash read/write when ideal mapping is enabled!");
		}

		if (transaction->Type == Transaction_Type::WRITE) {
			_my_instance->domains[transaction->Stream_id]->DepartingMappingEntries.erase((MVPN_type)((NVM_Transaction_Flash_WR*)transaction)->Content);
		} else {
			/*If this is a read for an MVP that is required for merging unchanged mapping enries
			* (stored on flash) with those updated entries that are evicted from CMT*/
			if (((NVM_Transaction_Flash_RD*)transaction)->RelatedWrite != NULL) {
				((NVM_Transaction_Flash_RD*)transaction)->RelatedWrite->RelatedRead = NULL;
			}

			_my_instance->ftl->TSU->Prepare_for_transaction_submit();
			MVPN_type mvpn = (MVPN_type)((NVM_Transaction_Flash_RD*)transaction)->Content;
			std::multimap<MVPN_type, LPA_type>::iterator it = _my_instance->domains[transaction->Stream_id]->ArrivingMappingEntries.find(mvpn);
			while (it != _my_instance->domains[transaction->Stream_id]->ArrivingMappingEntries.end()) {
				if ((*it).first == mvpn) {
					LPA_type lpa = (*it).second;

					//This mapping entry may arrived due to an update read request that is required for merging new and old mapping entries.
					//If that is the case, we should not insert it into CMT
					if (_my_instance->domains[transaction->Stream_id]->CMT->Is_slot_reserved_for_lpn_and_waiting(transaction->Stream_id, lpa)) {
						_my_instance->domains[transaction->Stream_id]->CMT->Insert_new_mapping_info(transaction->Stream_id, lpa,
							_my_instance->domains[transaction->Stream_id]->GlobalMappingTable[lpa].PPA,
							_my_instance->domains[transaction->Stream_id]->GlobalMappingTable[lpa].WrittenStateBitmap);
						auto it2 = _my_instance->domains[transaction->Stream_id]->Waiting_unmapped_read_transactions.find(lpa);
						while (it2 != _my_instance->domains[transaction->Stream_id]->Waiting_unmapped_read_transactions.end() &&
							(*it2).first == lpa) {
							if (_my_instance->is_lpa_locked_for_gc(transaction->Stream_id, lpa)) {
								_my_instance->manage_user_transaction_facing_barrier(it2->second);
							} else {
								if (_my_instance->translate_lpa_to_ppa(transaction->Stream_id, it2->second)) {
									_my_instance->ftl->TSU->Submit_transaction(it2->second);
								}
								else {
									_my_instance->mange_unsuccessful_translation(it2->second);
								}
							}
							_my_instance->domains[transaction->Stream_id]->Waiting_unmapped_read_transactions.erase(it2++);
						}
						it2 = _my_instance->domains[transaction->Stream_id]->Waiting_unmapped_program_transactions.find(lpa);
						while (it2 != _my_instance->domains[transaction->Stream_id]->Waiting_unmapped_program_transactions.end() &&
							(*it2).first == lpa) {
							if (_my_instance->is_lpa_locked_for_gc(transaction->Stream_id, lpa)) {
								_my_instance->manage_user_transaction_facing_barrier(it2->second);
							} else {
								if (_my_instance->translate_lpa_to_ppa(transaction->Stream_id, it2->second)) {
									_my_instance->ftl->TSU->Submit_transaction(it2->second);
									if (((NVM_Transaction_Flash_WR*)it2->second)->RelatedRead != NULL) {
										_my_instance->ftl->TSU->Submit_transaction(((NVM_Transaction_Flash_WR*)it2->second)->RelatedRead);
									}
								} else {
									_my_instance->mange_unsuccessful_translation(it2->second);
								}
							}
							_my_instance->domains[transaction->Stream_id]->Waiting_unmapped_program_transactions.erase(it2++);
						}
					}
				} else {
					break;
				}
				_my_instance->domains[transaction->Stream_id]->ArrivingMappingEntries.erase(it++);
			}
			_my_instance->ftl->TSU->Schedule();
		}
	}

	inline bool Address_Mapping_Unit_Zone_Level::is_lpa_locked_for_gc(stream_id_type stream_id, LPA_type lpa)
	{
		return domains[stream_id]->Locked_LPAs.find(lpa) != domains[stream_id]->Locked_LPAs.end();
	}

	inline bool Address_Mapping_Unit_Zone_Level::is_mvpn_locked_for_gc(stream_id_type stream_id, MVPN_type mvpn)
	{
		return domains[stream_id]->Locked_MVPNs.find(mvpn) != domains[stream_id]->Locked_MVPNs.end();
	}

	inline void Address_Mapping_Unit_Zone_Level::Set_barrier_for_accessing_lpa(stream_id_type stream_id, LPA_type lpa)
	{
		auto itr = domains[stream_id]->Locked_LPAs.find(lpa);
		if (itr != domains[stream_id]->Locked_LPAs.end()) {
			PRINT_ERROR("Illegal operation: Locking an LPA that has already been locked!");
		}
		domains[stream_id]->Locked_LPAs.insert(lpa);
	}

	inline void Address_Mapping_Unit_Zone_Level::Set_barrier_for_accessing_mvpn(stream_id_type stream_id, MVPN_type mvpn)
	{
		auto itr = domains[stream_id]->Locked_MVPNs.find(mvpn);
		if (itr != domains[stream_id]->Locked_MVPNs.end()) {
			PRINT_ERROR("Illegal operation: Locking an MVPN that has already been locked!");
		}
		domains[stream_id]->Locked_MVPNs.insert(mvpn);
	}

	inline void Address_Mapping_Unit_Zone_Level::Set_barrier_for_accessing_physical_block(const NVM::FlashMemory::Physical_Page_Address& block_address)
	{
		//The LPAs are actually not known until they are read one-by-one from flash storage. But, to reduce MQSim's complexity, we assume that LPAs are stored in DRAM and thus no read from flash storage is needed.
		Block_Pool_Slot_Type* block = &(block_manager->plane_manager[block_address.ChannelID][block_address.ChipID][block_address.DieID][block_address.PlaneID].Blocks[block_address.BlockID]);
		NVM::FlashMemory::Physical_Page_Address addr(block_address);
		for (flash_page_ID_type pageID = 0; pageID < block->Current_page_write_index; pageID++) {
			if (block_manager->Is_page_valid(block, pageID)) {
				addr.PageID = pageID;
				if (block->Holds_mapping_data) {
					MVPN_type mpvn = (MVPN_type)flash_controller->Get_metadata(addr.ChannelID, addr.ChipID, addr.DieID, addr.PlaneID, addr.BlockID, addr.PageID);
					if (domains[block->Stream_id]->GlobalTranslationDirectory[mpvn].MPPN != Convert_address_to_ppa(addr)) {
						PRINT_ERROR("Inconsistency in the global translation directory when locking an MPVN!")
						Set_barrier_for_accessing_mvpn(block->Stream_id, mpvn);
					}
				} else {
					LPA_type lpa = flash_controller->Get_metadata(addr.ChannelID, addr.ChipID, addr.DieID, addr.PlaneID, addr.BlockID, addr.PageID);
					LPA_type ppa = domains[block->Stream_id]->GlobalMappingTable[lpa].PPA;
					if (domains[block->Stream_id]->CMT->Exists(block->Stream_id, lpa)) {
						ppa = domains[block->Stream_id]->CMT->Retrieve_ppa(block->Stream_id, lpa);
					}
					if (ppa != Convert_address_to_ppa(addr)) {
						PRINT_ERROR("Inconsistency in the global mapping table when locking an LPA!")
					}
					Set_barrier_for_accessing_lpa(block->Stream_id, lpa);
				}
			}
		}
	}

	inline void Address_Mapping_Unit_Zone_Level::Remove_barrier_for_accessing_lpa(stream_id_type stream_id, LPA_type lpa)
	{
		auto itr = domains[stream_id]->Locked_LPAs.find(lpa);
		if (itr == domains[stream_id]->Locked_LPAs.end()) {
			PRINT_ERROR("Illegal operation: Unlocking an LPA that has not been locked!");
		}
		domains[stream_id]->Locked_LPAs.erase(itr);

		//If there are read requests waiting behind the barrier, then MQSim assumes they can be serviced with the actual page data that is accessed during GC execution
		auto read_tr = domains[stream_id]->Read_transactions_behind_LPA_barrier.find(lpa);
		while (read_tr != domains[stream_id]->Read_transactions_behind_LPA_barrier.end()) {
			handle_transaction_serviced_signal_from_PHY((*read_tr).second);
			delete (*read_tr).second;
			domains[stream_id]->Read_transactions_behind_LPA_barrier.erase(read_tr);
			read_tr = domains[stream_id]->Read_transactions_behind_LPA_barrier.find(lpa);
		}

		//If there are write requests waiting behind the barrier, then MQSim assumes they can be serviced with the actual page data that is accessed during GC execution. This may not be 100% true for all write requests, but, to avoid more complexity in the simulation, we accept this assumption.
		auto write_tr = domains[stream_id]->Write_transactions_behind_LPA_barrier.find(lpa);
		while (write_tr != domains[stream_id]->Write_transactions_behind_LPA_barrier.end()) {
			handle_transaction_serviced_signal_from_PHY((*write_tr).second);
			delete (*write_tr).second;
			domains[stream_id]->Write_transactions_behind_LPA_barrier.erase(write_tr);
			write_tr = domains[stream_id]->Write_transactions_behind_LPA_barrier.find(lpa);
		}
	}

	inline void Address_Mapping_Unit_Zone_Level::Remove_barrier_for_accessing_mvpn(stream_id_type stream_id, MVPN_type mvpn)
	{
		auto itr = domains[stream_id]->Locked_MVPNs.find(mvpn);
		if (itr == domains[stream_id]->Locked_MVPNs.end()) {
			PRINT_ERROR("Illegal operation: Unlocking an MVPN that has not been locked!");
		}
		domains[stream_id]->Locked_MVPNs.erase(itr);

		//If there are read requests waiting behind the barrier, then MQSim assumes they can be serviced with the actual page data that is accessed during GC execution
		if (domains[stream_id]->MVPN_read_transactions_waiting_behind_barrier.find(mvpn) != domains[stream_id]->MVPN_read_transactions_waiting_behind_barrier.end()) {
			domains[stream_id]->MVPN_read_transactions_waiting_behind_barrier.erase(mvpn);
			PPA_type ppn = domains[stream_id]->GlobalTranslationDirectory[mvpn].MPPN;
			if (ppn == NO_MPPN) {
				PRINT_ERROR("Reading an invalid physical flash page address in function generate_flash_read_request_for_mapping_data!")
			}

			NVM_Transaction_Flash_RD* readTR = new NVM_Transaction_Flash_RD(Transaction_Source_Type::MAPPING, stream_id,
					SECTOR_SIZE_IN_BYTE, NO_LPA, NO_PPA, NULL, mvpn, ((page_status_type)0x1) << sector_no_per_page, CurrentTimeStamp);
			Convert_ppa_to_address(ppn, readTR->Address);
			readTR->PPA = ppn;
			Stats::Total_flash_reads_for_mapping++;
			Stats::Total_flash_reads_for_mapping_per_stream[stream_id]++;

			handle_transaction_serviced_signal_from_PHY(readTR);
			
			delete readTR;
		}

		//If there are write requests waiting behind the barrier, then MQSim assumes they can be serviced with the actual page data that is accessed during GC execution. This may not be 100% true for all write requests, but, to avoid more complexity in the simulation, we accept this assumption.
		if (domains[stream_id]->MVPN_write_transaction_waiting_behind_barrier.find(mvpn) != domains[stream_id]->MVPN_write_transaction_waiting_behind_barrier.end()) {
			domains[stream_id]->MVPN_write_transaction_waiting_behind_barrier.erase(mvpn);
			//Writing back all dirty CMT entries that fall into the same translation virtual page (MVPN)
			unsigned int read_size = 0;
			page_status_type readSectorsBitmap = 0;
			LPA_type start_lpn = get_start_LPN_in_MVP(mvpn);
			LPA_type end_lpn = get_end_LPN_in_MVP(mvpn);
			for (LPA_type lpn_itr = start_lpn; lpn_itr <= end_lpn; lpn_itr++) {
				if (domains[stream_id]->CMT->Exists(stream_id, lpn_itr)) {
					if (domains[stream_id]->CMT->Is_dirty(stream_id, lpn_itr)) {
						domains[stream_id]->CMT->Make_clean(stream_id, lpn_itr);
					} else {
						page_status_type bitlocation = (((page_status_type)0x1) << (((lpn_itr - start_lpn) * GTD_entry_size) / SECTOR_SIZE_IN_BYTE));
						if ((readSectorsBitmap & bitlocation) == 0) {
							readSectorsBitmap |= bitlocation;
							read_size += SECTOR_SIZE_IN_BYTE;
						}
					}
				}
			}

			//Read the unchaged mapping entries from flash to merge them with updated parts of MVPN
			MPPN_type mppn = domains[stream_id]->GlobalTranslationDirectory[mvpn].MPPN;
			NVM_Transaction_Flash_WR* writeTR = new NVM_Transaction_Flash_WR(Transaction_Source_Type::MAPPING, stream_id, SECTOR_SIZE_IN_BYTE * sector_no_per_page,
				mvpn, mppn, NULL, mvpn, NULL, (((page_status_type)0x1) << sector_no_per_page) - 1, CurrentTimeStamp);

			Stats::Total_flash_reads_for_mapping++;
			Stats::Total_flash_writes_for_mapping++;
			Stats::Total_flash_reads_for_mapping_per_stream[stream_id]++;
			Stats::Total_flash_writes_for_mapping_per_stream[stream_id]++;

			handle_transaction_serviced_signal_from_PHY(writeTR);

			delete writeTR;
		}
	}

	inline void Address_Mapping_Unit_Zone_Level::manage_user_transaction_facing_barrier(NVM_Transaction_Flash* transaction)
	{
		std::pair<LPA_type, NVM_Transaction_Flash*> entry(transaction->LPA, transaction);
		if (transaction->Type == Transaction_Type::READ) {
			domains[transaction->Stream_id]->Read_transactions_behind_LPA_barrier.insert(entry);
		} else {
			domains[transaction->Stream_id]->Write_transactions_behind_LPA_barrier.insert(entry);
		}
	}

	inline void Address_Mapping_Unit_Zone_Level::manage_mapping_transaction_facing_barrier(stream_id_type stream_id, MVPN_type mvpn, bool read)
	{
		if (read) {
			domains[stream_id]->MVPN_read_transactions_waiting_behind_barrier.insert(mvpn);
		} else {
			domains[stream_id]->MVPN_write_transaction_waiting_behind_barrier.insert(mvpn);
		}
	}

	void Address_Mapping_Unit_Zone_Level::mange_unsuccessful_translation(NVM_Transaction_Flash* transaction)
	{
		//Currently, the only unsuccessfull translation would be for program translations that are accessing a plane that is running out of free pages
		Write_transactions_for_overfull_planes[transaction->Address.ChannelID][transaction->Address.ChipID][transaction->Address.DieID][transaction->Address.PlaneID].insert((NVM_Transaction_Flash_WR*)transaction);
	}

	void Address_Mapping_Unit_Zone_Level::Start_servicing_writes_for_overfull_plane(const NVM::FlashMemory::Physical_Page_Address plane_address)
	{
		std::set<NVM_Transaction_Flash_WR*>& waiting_write_list = Write_transactions_for_overfull_planes[plane_address.ChannelID][plane_address.ChipID][plane_address.DieID][plane_address.PlaneID];

		ftl->TSU->Prepare_for_transaction_submit();
		auto program = waiting_write_list.begin();
		while (program != waiting_write_list.end()) {
			if (translate_lpa_to_ppa((*program)->Stream_id, *program)) {
				ftl->TSU->Submit_transaction(*program);
				if ((*program)->RelatedRead != NULL) {
					ftl->TSU->Submit_transaction((*program)->RelatedRead);
				}
				waiting_write_list.erase(program++);
			}
			else {
				break;
			}
		}
		ftl->TSU->Schedule();
	}

	Zone_ID_type Address_Mapping_Unit_Zone_Level::get_zone_block_list(std::list<NVM_Transaction*> transaction_list, std::list<NVM::FlashMemory::Physical_Page_Address*> &list)
	{
		Zone_ID_type zoneID = translate_lpa_to_zoneID_for_gc(transaction_list);
		NVM_Transaction_Flash_ER* tr = static_cast<NVM_Transaction_Flash_ER*>(transaction_list.front());
		AddressMappingDomain* domain = domains[tr->Stream_id];

		switch (domain->ZoneAllocationScheme) {
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

	Zone_ID_type Address_Mapping_Unit_Zone_Level::translate_lpa_to_zoneID_for_gc(std::list<NVM_Transaction*> transaction_list)
	{
		if (transaction_list.size() == 0) {
			PRINT_ERROR("No transactions in NVM_Transaction_Flash_ER!!");
		}

		NVM_Transaction_Flash_ER* tr = static_cast<NVM_Transaction_Flash_ER*>(transaction_list.front());
		LPA_type lpn = tr->LPA;

		return lpn / (fzm->zone_size * 1024 * 1024);
	}

}
