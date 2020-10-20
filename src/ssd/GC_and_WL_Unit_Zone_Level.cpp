#include <math.h>
#include <vector>
#include <set>
#include "GC_and_WL_Unit_Zone_Level.h"
#include "Flash_Block_Manager.h"
#include "Flash_Zone_Manager.h"
#include "FTL.h"

namespace SSD_Components
{

	GC_and_WL_Unit_Zone_Level::GC_and_WL_Unit_Zone_Level(const sim_object_id_type& id,
		Address_Mapping_Unit_Base* address_mapping_unit, Flash_Zone_Manager_Base* zone_manager, Flash_Block_Manager_Base* block_manager, TSU_Base* tsu, NVM_PHY_ONFI* flash_controller, 
		GC_Block_Selection_Policy_Type block_selection_policy, double gc_threshold, bool preemptible_gc_enabled, double gc_hard_threshold,
		unsigned int ChannelCount, unsigned int chip_no_per_channel, unsigned int die_no_per_chip, unsigned int plane_no_per_die,
		unsigned int block_no_per_plane, unsigned int Page_no_per_block, unsigned int sectors_per_page, 
		bool use_copyback, double rho, unsigned int max_ongoing_gc_reqs_per_plane, bool dynamic_wearleveling_enabled, bool static_wearleveling_enabled, unsigned int static_wearleveling_threshold, int seed)
		: GC_and_WL_Unit_Base(id, address_mapping_unit, block_manager, tsu, flash_controller, block_selection_policy, gc_threshold, preemptible_gc_enabled, gc_hard_threshold,
		ChannelCount, chip_no_per_channel, die_no_per_chip, plane_no_per_die, block_no_per_plane, Page_no_per_block, sectors_per_page, use_copyback, rho, max_ongoing_gc_reqs_per_plane, 
			dynamic_wearleveling_enabled, static_wearleveling_enabled, static_wearleveling_threshold, seed)
	{
		if (zone_manager == NULL)
			PRINT_ERROR("in GC_and_WL_Unit_Zone_Level, No zone manager!!")
		
	}
	

	bool GC_and_WL_Unit_Zone_Level::GC_is_in_urgent_mode(const NVM::FlashMemory::Flash_Chip* chip)
	{
		return false;
	}

	void GC_and_WL_Unit_Zone_Level::Check_gc_required(const unsigned int free_block_pool_size, const NVM::FlashMemory::Physical_Page_Address& planeAddress)
	{
		if (free_block_pool_size < block_pool_gc_threshold) {
			// we need GC 
			// there's no ENOUGH free space


		}
	}

	void GC_and_WL_Unit_Zone_Level::Do_GC_for_Zone(User_Request* user_request)
	{
		std::cout << "here is GC_and_WL_Unit_Zone_Level::Do_GC_for_Zone()" << std::endl;
	}

	
}
