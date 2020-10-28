#ifndef GC_AND_WL_UNIT_ZONE_LEVEL_H
#define GC_AND_WL_UNIT_ZONE_LEVEL_H

#include "GC_and_WL_Unit_Base.h"
#include "NVM_PHY_ONFI.h"
#include "../utils/RandomGenerator.h"
#include <queue>


namespace SSD_Components
{
	class GC_and_WL_Unit_Zone_Level : public GC_and_WL_Unit_Base
	{
	public:
		GC_and_WL_Unit_Zone_Level(const sim_object_id_type& id,
			Address_Mapping_Unit_Base* address_mapping_unit, Flash_Zone_Manager_Base* zone_manager, Flash_Block_Manager_Base* block_manager, TSU_Base* tsu, NVM_PHY_ONFI* flash_controller,
			GC_Block_Selection_Policy_Type block_selection_policy, double gc_threshold, bool preemptible_gc_enabled, double gc_hard_threshold,
			unsigned int channel_count, unsigned int chip_no_per_channel, unsigned int die_no_per_chip, unsigned int plane_no_per_die,
			unsigned int block_no_per_plane, unsigned int page_no_per_block, unsigned int sectors_per_page, 
			bool use_copyback, double rho, unsigned int max_ongoing_gc_reqs_per_plane = 10, 
			bool dynamic_wearleveling_enabled = true, bool static_wearleveling_enabled = true, unsigned int static_wearleveling_threshold = 100, int seed = 432);

			bool GC_is_in_urgent_mode(const NVM::FlashMemory::Flash_Chip*);
			void Check_gc_required(const unsigned int BlockPoolSize, const NVM::FlashMemory::Physical_Page_Address& planeAddress);
			void Do_GC_for_Zone(User_Request* user_request);

	
	private:
		Flash_Zone_Manager_Base* my_zone_manager;
	};
}
#endif // !GC_AND_WL_UNIT_ZONE_LEVEL_H
