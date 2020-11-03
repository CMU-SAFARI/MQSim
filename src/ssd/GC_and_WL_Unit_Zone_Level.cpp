#include <math.h>
#include <vector>
#include <set>
#include "GC_and_WL_Unit_Zone_Level.h"
#include "Flash_Block_Manager.h"
#include "Flash_Zone_Manager.h"
#include "FTL.h"

#include "Address_Mapping_Unit_Zone_Level.h"

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
			PRINT_ERROR("in GC_and_WL_Unit_Zone_Level, No zone manager!!");

		my_zone_manager = zone_manager;
	}
	

	bool GC_and_WL_Unit_Zone_Level::GC_is_in_urgent_mode(const NVM::FlashMemory::Flash_Chip* chip)
	{
		return false;
	}

	void GC_and_WL_Unit_Zone_Level::Check_gc_required(const unsigned int free_block_pool_size, const NVM::FlashMemory::Physical_Page_Address& planeAddress)
	{
		if (free_block_pool_size < block_pool_gc_threshold) {
			// we need GC, there's no ENOUGH free space
			// But, the host should do
		}
		return ;
	}

	void GC_and_WL_Unit_Zone_Level::Do_GC_for_Zone(User_Request* user_request)
	{
		Flash_Zone_Manager* zm = static_cast<Flash_Zone_Manager*>(my_zone_manager);

		// 1. get ZoneID and a list of blocks in the Zone 
		std::list<NVM::FlashMemory::Physical_Page_Address*> block_list_in_a_zone;
		Zone_ID_type zoneID = static_cast<Address_Mapping_Unit_Zone_Level*>(address_mapping_unit)->get_zone_block_list(user_request->Transaction_list, block_list_in_a_zone);
		
		// 2. we need to lock the zone and change its state to GC (Internal state)
		zm->GC_WL_started(zoneID);	// Has_ongoing_erase = true;
		zm->Change_Zone_State(zoneID, NVM::FlashMemory::Zone_Status::GC);

		// 3. For each block, Submit the erase operation
		std::list<NVM::FlashMemory::Physical_Page_Address*>::iterator address;
		for (address = block_list_in_a_zone.begin(); address != block_list_in_a_zone.end(); address++)
		{
			std::cout << (*address)->BlockID << std::endl;
			PlaneBookKeepingType* pbke = block_manager->Get_plane_bookkeeping_entry(*(*address));	// Get a controller for the plane
			Block_Pool_Slot_Type* block = &pbke->Blocks[(*address)->BlockID];

			block_manager->GC_WL_started(*(*address));
			pbke->Ongoing_erase_operations.insert((*address)->BlockID);
			address_mapping_unit->Set_barrier_for_accessing_physical_block(*(*address));

			Stats::Total_gc_executions++;
			tsu->Prepare_for_transaction_submit();

			NVM_Transaction_Flash_ER* gc_erase_tr = 
					new NVM_Transaction_Flash_ER(Transaction_Source_Type::GC_WL, pbke->Blocks[(*address)->BlockID].Stream_id, *(*address)); 
			// Transaction_Source_type is userIO? GC_WL? how does it affect?

			block->Erase_transaction = gc_erase_tr;
			tsu->Submit_transaction(gc_erase_tr);
		}

		tsu->Schedule();

		// 4. change the zone state to Empty and release the lock 

		// we should wait here? 

		// 5. ack to the host?
	}

	
}
