#include "Flash_Zone_Manager_Base.h"
#include "Flash_Zone_Manager.h"


namespace SSD_Components
{
	Flash_Zone_Manager::Flash_Zone_Manager(unsigned int channel_count, unsigned int chip_no_per_channel, 
							unsigned int die_no_per_chip, unsigned int plane_no_per_die,
							unsigned int block_no_per_plane, unsigned int page_no_per_block,
							unsigned int page_capacity, unsigned int zone_size)
							: Flash_Zone_Manager_Base(channel_count, chip_no_per_channel, die_no_per_chip, plane_no_per_die, block_no_per_plane, page_no_per_block, page_capacity, zone_size)
	{

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
}
