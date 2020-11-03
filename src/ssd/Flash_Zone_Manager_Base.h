#ifndef ZONE_POOL_MANAGER_BASE_H
#define ZONE_POOL_MANAGER_BASE_H

#include <list>
#include <cstdint>
#include <queue>
#include <set>
#include "../nvm_chip/flash_memory/FlashTypes.h"
#include "../nvm_chip/flash_memory/Physical_Page_Address.h"
#include "../nvm_chip/flash_memory/Zone.h"
#include "GC_and_WL_Unit_Base.h"
#include "../nvm_chip/flash_memory/FlashTypes.h"

namespace SSD_Components
{


	class Flash_Zone_Manager_Base
	{
		friend class Address_mapping_Unit_Base;
		friend class Address_Mapping_Unit_Zone_Level;
		friend class GC_and_WL_Unit_Page_Level;
		friend class GC_and_WL_Unit_Zone_Level;
		friend class GC_and_WL_Unit_Base;
	public:
		Flash_Zone_Manager_Base(unsigned int channel_count, unsigned int chip_no_per_channel, 
							unsigned int die_no_per_chip, unsigned int plane_no_per_die,
							unsigned int block_no_per_plane, unsigned int page_no_per_block,
							unsigned int page_capacity, unsigned int zone_size, 
							unsigned int channel_no_per_zone, unsigned int chip_no_per_zone, unsigned int die_no_per_zone, unsigned int plane_no_per_zone);
		virtual ~Flash_Zone_Manager_Base();
		unsigned int zone_count;
		unsigned int zone_size;
		NVM::FlashMemory::Zone **zones;


		unsigned int Channel_no_per_zone;
		unsigned int Chip_no_per_zone;
		unsigned int Die_no_per_zone;
		unsigned int Plane_no_per_zone;
		

	protected:
		//GC_and_WL_Unit_Base *gc_and_wl_unit;
		unsigned int channel_count;
		unsigned int chip_no_per_channel;
		unsigned int die_no_per_chip;
		unsigned int plane_no_per_die;
		unsigned int block_no_per_plane;
		unsigned int pages_no_per_block;

		unsigned int page_capacity;

	};
}

#endif//!ZONE_POOL_MANAGER_BASE_H
