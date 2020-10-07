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
	Address_Mapping_Unit_Zone_Level::Address_Mapping_Unit_Zone_Level(const sim_object_id_type& id, FTL* ftl, NVM_PHY_ONFI* flash_controller, Flash_Block_Manager_Base* block_manager,
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
		_my_instance = this;
		
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

	void Address_Mapping_Unit_Zone_Level::allocate_plane_for_user_write(NVM_Transaction_Flash_WR* transaction)
	{
		LPA_type lpn = transaction->LPA;
		NVM::FlashMemory::Physical_Page_Address& targetAddress = transaction->Address;
		AddressMappingDomain* domain = domains[transaction->Stream_id];
		
		NVM::FlashMemory::Zone **zones = ftl->ZoneManager->zones;
		unsigned int zone_size = ftl->ZoneManager->zone_size;

		unsigned int zoneID = lpn / (zone_size * 1024 * 1024);
		unsigned int zoneOffset = lpn % (zone_size * 1024 * 1024);

		if (zoneOffset < zones[zoneID]->write_point)
			PRINT_ERROR("write off set is smaller than write_point in a zone. Something wrong!");

        switch (domain->ZoneAllocationeScheme) {
			case Zone_Allocation_Scheme_Type::CDPW:
				//targetAddress.ChannelID = domain->Channel_ids[(unsigned int)(lpn % domain->Channel_no)];
				//targetAddress.ChipID = domain->Chip_ids[(unsigned int)((lpn / domain->Channel_no) % domain->Chip_no)];
				//targetAddress.DieID = domain->Die_ids[(unsigned int)((lpn / (domain->Chip_no * domain->Channel_no)) % domain->Die_no)];
				//targetAddress.PlaneID = domain->Plane_ids[(unsigned int)((lpn / (domain->Die_no * domain->Chip_no * domain->Channel_no)) % domain->Plane_no)]; 
				break;
			default:
				PRINT_ERROR("Unknown zone allocation scheme type!")
		}

		
	}
}
