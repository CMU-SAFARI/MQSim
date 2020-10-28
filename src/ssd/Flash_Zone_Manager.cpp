#include "Flash_Zone_Manager_Base.h"
#include "Flash_Zone_Manager.h"


namespace SSD_Components
{
	Flash_Zone_Manager::Flash_Zone_Manager(unsigned int channel_count, unsigned int chip_no_per_channel, 
							unsigned int die_no_per_chip, unsigned int plane_no_per_die,
							unsigned int block_no_per_plane, unsigned int page_no_per_block,
							unsigned int page_capacity, unsigned int zone_size,
							Zone_Allocation_Scheme_Type zone_allocation_type, 
							SubZone_Allocation_Scheme_Type subzone_allocateion_type)
							: Flash_Zone_Manager_Base(channel_count, chip_no_per_channel, die_no_per_chip, plane_no_per_die, block_no_per_plane, page_no_per_block, page_capacity, zone_size)
	{
		ZoneAllocationType = zone_allocation_type;
		SubZoneAllocationType = subzone_allocateion_type;
	}

	Flash_Zone_Manager::~Flash_Zone_Manager() 
	{
	}

	void Flash_Zone_Manager::Update_Zone_Write_Point(Zone_ID_type zoneID, unsigned int write_amount)
	{
		NVM::FlashMemory::Zone *zone = zones[zoneID];
		unsigned int current_write_point = zone->write_point;

		if (current_write_point + write_amount > zone_size)
			PRINT_ERROR("write pointe exceeds the zone size")

		zone->write_point += write_amount;
	}

	void Flash_Zone_Manager::GC_WL_started(Zone_ID_type zoneID)
	{
		zones[zoneID]->Has_ongoing_gc_wl = true;
	}

	NVM::FlashMemory::Zone_Status Flash_Zone_Manager::Change_Zone_State(Zone_ID_type zoneID, NVM::FlashMemory::Zone_Status new_zone_state)
	{
		NVM::FlashMemory::Zone_Status old_state = zones[zoneID]->zone_status;
		zones[zoneID]->zone_status = new_zone_state;

		return old_state;
	}

	void Flash_Zone_Manager::Get_Zone_Block_list(Zone_ID_type zoneID, std::list<NVM::FlashMemory::Physical_Page_Address*> list)
	{
		NVM::FlashMemory::Physical_Page_Address* address = new NVM::FlashMemory::Physical_Page_Address;
		// fill the address
		list.push_back(address);

	}
}

