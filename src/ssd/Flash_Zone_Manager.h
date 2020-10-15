#ifndef FLASH_ZONE_MANAGER_H
#define FLASH_ZONE_MANAGER_H

#include <list>
#include <cstdint>
#include <queue>
#include <set>
#include "Flash_Zone_Manager_Base.h"
#include "../nvm_chip/flash_memory/FlashTypes.h"
#include "../nvm_chip/flash_memory/Physical_Page_Address.h"

namespace SSD_Components
{
	class Flash_Zone_Manager : public Flash_Zone_Manager_Base
	{
	public:
		Flash_Zone_Manager(unsigned int channel_count, unsigned int chip_no_per_channel, 
							unsigned int die_no_per_chip, unsigned int plane_no_per_die,
							unsigned int block_no_per_plane, unsigned int page_no_per_block,
							unsigned int page_capacity, unsigned int zone_size);
		~Flash_Zone_Manager();

		
	private:
		void Update_Zone_Write_Point(Zone_ID_type zoneID, unsigned int write_amount);


	};
}

#endif//!FLASH_ZONE_MANAGER_H
