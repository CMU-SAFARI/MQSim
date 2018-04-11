#include <math.h>
#include <vector>
#include <set>
#include "GC_and_WL_Unit_Page_Level.h"
#include "Flash_Block_Manager.h"
#include "FTL.h"

namespace SSD_Components
{

	GC_and_WL_Unit_Page_Level::GC_and_WL_Unit_Page_Level(const sim_object_id_type& id,
		Address_Mapping_Unit_Base* address_mapping_unit, Flash_Block_Manager_Base* block_manager, TSU_Base* tsu, NVM_PHY_ONFI* flash_controller, 
		GC_Block_Selection_Policy_Type block_selection_policy, double gc_threshold, bool preemptible_gc_enabled, double gc_hard_threshold,
		unsigned int ChannelCount, unsigned int chip_no_per_channel, unsigned int die_no_per_chip, unsigned int plane_no_per_die,
		unsigned int block_no_per_plane, unsigned int Page_no_per_block, unsigned int sectors_per_page, 
		bool use_copyback, int seed, unsigned int max_ongoing_gc_reqs_per_plane)
		: GC_and_WL_Unit_Base(id, address_mapping_unit, block_manager, tsu, flash_controller, block_selection_policy, gc_threshold, preemptible_gc_enabled, gc_hard_threshold,
		ChannelCount, chip_no_per_channel, die_no_per_chip, plane_no_per_die, block_no_per_plane, Page_no_per_block, sectors_per_page),
		random_generator(seed), max_ongoing_gc_reqs_per_plane(max_ongoing_gc_reqs_per_plane),
		use_copyback(use_copyback)
	{
		rga_set_size = (unsigned int)log2(block_no_per_plane);
		random_pp_threshold = (unsigned int)(0.25 * pages_no_per_block);
	}
	
	bool GC_and_WL_Unit_Page_Level::GC_is_in_urgent_mode(const NVM::FlashMemory::Flash_Chip* chip)
	{
		if (!preemptible_gc_enabled)
			return true;

		NVM::FlashMemory::Physical_Page_Address addr;
		addr.ChannelID = chip->ChannelID; addr.ChipID = chip->ChipID;
		for (unsigned int die_id = 0; die_id < die_no_per_chip; die_id++)
			for (unsigned int plane_id = 0; plane_id < plane_no_per_die; plane_id++)
			{
				addr.DieID = die_id; addr.PlaneID = plane_id;
				if (block_manager->Get_pool_size(addr) < block_pool_gc_hard_threshold)
					return true;
			}
		return false;
	}

	bool GC_and_WL_Unit_Page_Level::is_wf(PlaneBookKeepingType* pbke, flash_block_ID_type gc_candidate_block_id)
	{
		for (unsigned int stream_id = 0; stream_id < address_mapping_unit->Get_no_of_input_streams(); stream_id++)
			if ((&pbke->Blocks[gc_candidate_block_id]) == pbke->DataWF[stream_id] || (&pbke->Blocks[gc_candidate_block_id]) == pbke->Translation_wf[stream_id])
				return true;
		return false;
	}

	void GC_and_WL_Unit_Page_Level::Check_gc_required(const unsigned int free_block_pool_size, const NVM::FlashMemory::Physical_Page_Address& plane_address)
	{
		if (free_block_pool_size < block_pool_gc_threshold)
		{
			flash_block_ID_type gc_candidate_block_id;
			PlaneBookKeepingType* pbke = &((Flash_Block_Manager*)block_manager)->plane_manager[plane_address.ChannelID][plane_address.ChipID][plane_address.DieID][plane_address.PlaneID];

			if (pbke->Ongoing_erase_operations.size() >= max_ongoing_gc_reqs_per_plane)
				return;

			switch (block_selection_policy)
			{
			case SSD_Components::GC_Block_Selection_Policy_Type::GREEDY://Find the set of blocks with maximum number of invalid pages and no free pages
			{
				gc_candidate_block_id = 0;
				if (pbke->Ongoing_erase_operations.find(0) != pbke->Ongoing_erase_operations.end())
					gc_candidate_block_id++;
				for (flash_block_ID_type block_id = 1; block_id < block_no_per_plane; block_id++)
				{
					if (pbke->Blocks[block_id].Invalid_page_count > pbke->Blocks[gc_candidate_block_id].Invalid_page_count
						&& pbke->Blocks[block_id].Current_page_write_index == pages_no_per_block
						&& pbke->Ongoing_erase_operations.find(block_id) == pbke->Ongoing_erase_operations.end())
						gc_candidate_block_id = block_id;
				}
				break;
			}
			case SSD_Components::GC_Block_Selection_Policy_Type::RGA:
			{
				std::set<flash_block_ID_type> random_set;
				while (random_set.size() < rga_set_size)
				{
					flash_block_ID_type block_id = random_generator.Uniform_uint(0, block_no_per_plane - 1);
					if (pbke->Ongoing_erase_operations.find(block_id) == pbke->Ongoing_erase_operations.end())
						random_set.insert(block_id);
				}
				gc_candidate_block_id = *random_set.begin();
				for(auto block_id : random_set)
					if (pbke->Blocks[block_id].Invalid_page_count > pbke->Blocks[gc_candidate_block_id].Invalid_page_count
						&& pbke->Blocks[block_id].Current_page_write_index == pages_no_per_block)
						gc_candidate_block_id = block_id;
				break;
			}
			case SSD_Components::GC_Block_Selection_Policy_Type::RANDOM:
			{
				gc_candidate_block_id = random_generator.Uniform_uint(0, block_no_per_plane - 1);
				unsigned int repeat = 0;
				while (is_wf(pbke, gc_candidate_block_id) && repeat++ < block_no_per_plane)//A write frontier block should not be selected for garbage collection
					gc_candidate_block_id = random_generator.Uniform_uint(0, block_no_per_plane - 1);
				break;
			}
			case SSD_Components::GC_Block_Selection_Policy_Type::RANDOM_P:
			{
				gc_candidate_block_id = random_generator.Uniform_uint(0, block_no_per_plane - 1);
				unsigned int repeat = 0;

				//A write frontier block or a block with free pages should not be selected for garbage collection
				while ((pbke->Blocks[gc_candidate_block_id].Current_page_write_index < pages_no_per_block || is_wf(pbke, gc_candidate_block_id))
					&& repeat++ < block_no_per_plane)
					gc_candidate_block_id = random_generator.Uniform_uint(0, block_no_per_plane - 1);
				break;
			}
			case SSD_Components::GC_Block_Selection_Policy_Type::RANDOM_PP:
			{
				gc_candidate_block_id = random_generator.Uniform_uint(0, block_no_per_plane - 1);
				unsigned int repeat = 0;

				//The selected gc block should have a minimum number of invalid pages
				while ((pbke->Blocks[gc_candidate_block_id].Current_page_write_index < pages_no_per_block 
					|| pbke->Blocks[gc_candidate_block_id].Invalid_page_count < random_pp_threshold
					|| is_wf(pbke, gc_candidate_block_id)
					|| pbke->Ongoing_erase_operations.find(gc_candidate_block_id) != pbke->Ongoing_erase_operations.end())
					&& repeat++ < block_no_per_plane)
					gc_candidate_block_id = random_generator.Uniform_uint(0, block_no_per_plane - 1);
				break;
			}
			case SSD_Components::GC_Block_Selection_Policy_Type::FIFO:
				gc_candidate_block_id = pbke->Block_usage_history.front();
				pbke->Block_usage_history.pop();
				break;
			default:
				break;
			}
			
			if (pbke->Ongoing_erase_operations.find(gc_candidate_block_id) != pbke->Ongoing_erase_operations.end())
				return;

			NVM::FlashMemory::Physical_Page_Address gc_candidate_address(plane_address);
			gc_candidate_address.BlockID = gc_candidate_block_id;
			BlockPoolSlotType* block = &pbke->Blocks[gc_candidate_block_id];
			if (block->Current_page_write_index == 0 || block->Invalid_page_count == 0)//No invalid page to erase
				return;

			tsu->Prepare_for_transaction_submit();
			NVM_Transaction_Flash_ER* gc_erase_tr = new NVM_Transaction_Flash_ER(Transaction_Source_Type::GC, pbke->Blocks[gc_candidate_block_id].Stream_id, gc_candidate_address);
			if (block->Current_page_write_index - block->Invalid_page_count > 0)//If there are some valid pages in block, then prepare flash transactions for page movement
			{
				NVM_Transaction_Flash_RD* gc_read = NULL;
				NVM_Transaction_Flash_WR* gc_write = NULL;
				for (flash_page_ID_type pageID = 0; pageID < block->Current_page_write_index; pageID++)
				{
					if ((block->Invalid_page_bitmap[pageID / 64] & (((uint64_t) 1) << pageID)) == 0)
					{
						gc_candidate_address.PageID = pageID;
						if (use_copyback)
						{
							gc_write = new NVM_Transaction_Flash_WR(Transaction_Source_Type::GC, block->Stream_id, sector_no_per_page * SECTOR_SIZE_IN_BYTE,
								NO_LPA, address_mapping_unit->Convert_address_to_ppa(gc_candidate_address), NULL, 0, NULL, 0, INVALID_TIME_STAMP);
							gc_write->ExecutionMode = WriteExecutionModeType::COPYBACK;
						}
						else
						{
							gc_read = new NVM_Transaction_Flash_RD(Transaction_Source_Type::GC, block->Stream_id, sector_no_per_page * SECTOR_SIZE_IN_BYTE,
								NO_LPA, address_mapping_unit->Convert_address_to_ppa(gc_candidate_address), gc_candidate_address, NULL, 0, NULL, 0, INVALID_TIME_STAMP);
							tsu->Submit_transaction(gc_read);
							gc_write = new NVM_Transaction_Flash_WR(Transaction_Source_Type::GC, block->Stream_id, sector_no_per_page * SECTOR_SIZE_IN_BYTE,
								NO_LPA, NO_PPA, gc_candidate_address, NULL, 0, gc_read, 0, INVALID_TIME_STAMP);
							gc_write->ExecutionMode = WriteExecutionModeType::SIMPLE;
							gc_read->RelatedWrite = gc_write;
						}
						gc_erase_tr->Page_movement_activities.push_back(gc_write);
					}
				}
			}
			pbke->Ongoing_erase_operations.insert(gc_candidate_block_id);
			block->Erase_transaction = gc_erase_tr;
			tsu->Submit_transaction(gc_erase_tr);
			
			tsu->Schedule();
		}
	}

	void GC_and_WL_Unit_Page_Level::Check_wl_required(const double static_wl_factor, const NVM::FlashMemory::Physical_Page_Address plane_address)
	{
	}
}