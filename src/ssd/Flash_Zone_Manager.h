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
							unsigned int page_capacity, unsigned int zone_size,
							Zone_Allocation_Scheme_Type zone_allocation_type, SubZone_Allocation_Scheme_Type subzone_allocation_type);
		~Flash_Zone_Manager();

		Zone_Allocation_Scheme_Type ZoneAllocationType;
		SubZone_Allocation_Scheme_Type SubZoneAllocationType;

		void GC_WL_started(Zone_ID_type zoneID);
		NVM::FlashMemory::Zone_Status Change_Zone_State(Zone_ID_type zoneID, NVM::FlashMemory::Zone_Status new_zone_state);
		void Get_Zone_Block_list(Zone_ID_type zoneID, std::list<NVM::FlashMemory::Physical_Page_Address*> list);

	private:
		void Update_Zone_Write_Point(Zone_ID_type zoneID, unsigned int write_amount);


	};
}

#endif//!FLASH_ZONE_MANAGER_H
