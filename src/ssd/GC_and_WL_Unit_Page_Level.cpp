#include <math.h>
#include <vector>
#include "GC_and_WL_Unit_Page_Level.h"
#include "Flash_Block_Manager.h"
#include "FTL.h"

namespace SSD_Components
{
	GC_and_WL_Unit_Page_Level::GC_and_WL_Unit_Page_Level(FTL* ftl, Flash_Block_Manager_Base* BlockManager, double GCThreshold,
		GC_Block_Selection_Policy_Type BlockSelectionPolicy,
		bool PreemptibleGCEnabled, double GCHardThreshold,
		unsigned int ChannelCount, unsigned int ChipNoPerChannel, unsigned int DieNoPerChip, unsigned int PlaneNoPerDie,
		unsigned int Block_no_per_plane, unsigned int Page_no_per_block, unsigned int SectorsPerPage, int seed)
		: GC_and_WL_Unit_Base(ftl, BlockManager, GCThreshold, PreemptibleGCEnabled, GCHardThreshold,
		ChannelCount, ChipNoPerChannel, DieNoPerChip, PlaneNoPerDie, Block_no_per_plane, Page_no_per_block, SectorsPerPage),
		block_selection_policy(BlockSelectionPolicy), random_generator(seed)
	{
		rga_set_size = (unsigned int)log2(Block_no_per_plane);
		random_pp_threshold = (unsigned int)(0.5 * pages_no_per_block);
	}

	void GC_and_WL_Unit_Page_Level::Setup_triggers()
	{
		flash_controller->ConnectToTransactionServicedSignal(handle_transaction_serviced_signal_from_PHY);
	}

	bool GC_and_WL_Unit_Page_Level::GC_is_in_urgent_mode(const NVM::FlashMemory::Chip* chip)
	{
		NVM::FlashMemory::Physical_Page_Address addr;
		addr.ChannelID = chip->ChannelID; addr.ChipID = chip->ChipID;
		for (unsigned int die_id = 0; die_id < die_no_per_chip; die_id++)
			for (unsigned int plane_id = 0; plane_id < plane_no_per_die; plane_id++)
			{
				addr.DieID = die_id; addr.PlaneID = plane_id;
				if (block_manager->Get_pool_size(addr) < gc_hard_threshold)
					return true;
			}
		return false;
	}

	void GC_and_WL_Unit_Page_Level::Check_gc_required(const unsigned int free_block_pool_size, const NVM::FlashMemory::Physical_Page_Address& plane_address)
	{
		if (free_block_pool_size < block_pool_gc_threshold)
		{
			flash_block_ID_type gc_candidate_block_id = 0;
			PlaneBookKeepingType pbke = ((Flash_Block_Manager*)block_manager)->planeManager[plane_address.ChannelID][plane_address.ChipID][plane_address.DieID][plane_address.PlaneID];

			switch (block_selection_policy)
			{
			case SSD_Components::GC_Block_Selection_Policy_Type::GREEDY://Find the set of blocks with maximum number of invalid pages and no free pages
			{
				for (flash_block_ID_type blockID = 1; blockID < block_no_per_plane; blockID++)
				{
					if (pbke.Blocks[blockID].InvalidPageCount > pbke.Blocks[gc_candidate_block_id].InvalidPageCount && pbke.Blocks[blockID].CurrentPageWriteIndex == block_no_per_plane)
						gc_candidate_block_id = blockID;
				}
				break;
			}
			case SSD_Components::GC_Block_Selection_Policy_Type::RGA:
			{
				std::vector<flash_block_ID_type> random_set;
				for (unsigned int i = 0; i < rga_set_size; i++)
					random_set.push_back(random_generator.Uniform_uint(0, block_no_per_plane - 1));
				gc_candidate_block_id = random_set.front();
				for(auto blockID : random_set)
					if (pbke.Blocks[blockID].InvalidPageCount > pbke.Blocks[gc_candidate_block_id].InvalidPageCount && pbke.Blocks[blockID].CurrentPageWriteIndex == block_no_per_plane)
						gc_candidate_block_id = blockID;
				break;
			}
			case SSD_Components::GC_Block_Selection_Policy_Type::RANDOM:
				gc_candidate_block_id = random_generator.Uniform_uint(0, block_no_per_plane - 1);
				break;
			case SSD_Components::GC_Block_Selection_Policy_Type::RANDOM_P:
			{
				gc_candidate_block_id = random_generator.Uniform_uint(0, block_no_per_plane - 1);
				unsigned int repeat = 0;
				while (pbke.Blocks[gc_candidate_block_id].CurrentPageWriteIndex < block_no_per_plane && repeat++ < block_no_per_plane)
					gc_candidate_block_id = random_generator.Uniform_uint(0, block_no_per_plane - 1);
				break;
			}
			case SSD_Components::GC_Block_Selection_Policy_Type::RANDOM_PP:
			{
				gc_candidate_block_id = random_generator.Uniform_uint(0, block_no_per_plane - 1);
				unsigned int repeat = 0;
				while (pbke.Blocks[gc_candidate_block_id].CurrentPageWriteIndex < block_no_per_plane && repeat++ < block_no_per_plane
					&& pbke.Blocks[gc_candidate_block_id].InvalidPageCount < random_pp_threshold)
					gc_candidate_block_id = random_generator.Uniform_uint(0, block_no_per_plane - 1);
				break;
			}
			case SSD_Components::GC_Block_Selection_Policy_Type::FIFO:
				gc_candidate_block_id = pbke.BlockUsageHistory.front();
				pbke.BlockUsageHistory.pop();
				break;
			default:
				break;
			}


			NVM::FlashMemory::Physical_Page_Address gc_candidate_address(plane_address);
			gc_candidate_address.BlockID = gc_candidate_block_id;
			NVM_Transaction_Flash_ER* gc_erase_tr = new NVM_Transaction_Flash_ER(Transaction_Source_Type::GC, pbke.Blocks[gc_candidate_block_id].Stream_id,	gc_candidate_address);
			BlockPoolSlotType* block = &pbke.Blocks[gc_candidate_block_id];
			int total_writen_pages = block_no_per_plane - block->CurrentPageWriteIndex;
			if (total_writen_pages - block->InvalidPageCount > 0)//If there are some valid pages in block, then prepare flash transactions for page movement
			{
				NVM_Transaction_Flash_RD* gc_read;
				NVM_Transaction_Flash_WR* gc_write;
				for (flash_page_ID_type pageID = 0; pageID < block->CurrentPageWriteIndex; pageID++)
				{
					if ((block->InvalidPageVector[pageID / 64] & (((uint64_t) 1) << pageID)) != 0)
					{
						//gc_read = new NVM_Transaction_Flash_RD(Transaction_Source_Type::GC, block->Stream_id, 1, 0, NULL, 
						gc_erase_tr->Page_movement_activities.push_back(gc_write);
					}
				}
			}
			else
			{
			}
			//ftl->TSU->Prepare_for_transaction_submit();
			//ftl->TSU->Submit_transaction(gc_write);
			//ftl->TSU->Schedule();
		}
	}

	void GC_and_WL_Unit_Page_Level::Check_wl_required(const double static_wl_factor, const NVM::FlashMemory::Physical_Page_Address plane_address)
	{
	}

	void GC_and_WL_Unit_Page_Level::handle_transaction_serviced_signal_from_PHY(NVM_Transaction_Flash* transaction)
	{
		if (transaction->Source != Transaction_Source_Type::GC)
			return;

		//NVM_Transaction_Flash_ER* gc_erase;
		//ftl->TSU->Prepare_for_transaction_submit();
		//ftl->TSU->Submit_transaction(gc_write);
		//ftl->TSU->Schedule();
	}

}