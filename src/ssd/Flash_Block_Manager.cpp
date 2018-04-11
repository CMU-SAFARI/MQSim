
#include "../nvm_chip/flash_memory/Physical_Page_Address.h"
#include "Flash_Block_Manager.h"
#include "Stats.h"

namespace SSD_Components
{
	Flash_Block_Manager::Flash_Block_Manager(GC_and_WL_Unit_Base* gc_and_wl_unit, unsigned int MaxAllowedBlockEraseCount, unsigned int TotalStreamNo,
		unsigned int ChannelCount, unsigned int ChipNoPerChannel, unsigned int DieNoPerChip, unsigned int PlaneNoPerDie,
		unsigned int Block_no_per_plane, unsigned int Page_no_per_block)
		: Flash_Block_Manager_Base(gc_and_wl_unit, MaxAllowedBlockEraseCount, TotalStreamNo),
		channel_count(ChannelCount), chip_no_per_channel(ChipNoPerChannel), die_no_per_chip(DieNoPerChip), plane_no_per_die(PlaneNoPerDie),
		block_no_per_plane(Block_no_per_plane), pages_no_per_block(Page_no_per_block)
	{
		plane_manager = new PlaneBookKeepingType***[channel_count];
		for (unsigned int channelID = 0; channelID < channel_count; channelID++)
		{
			plane_manager[channelID] = new PlaneBookKeepingType**[chip_no_per_channel];
			for (unsigned int chipID = 0; chipID < chip_no_per_channel; chipID++)
			{
				plane_manager[channelID][chipID] = new PlaneBookKeepingType*[die_no_per_chip];
				for (unsigned int dieID = 0; dieID < die_no_per_chip; dieID++)
				{
					plane_manager[channelID][chipID][dieID] = new PlaneBookKeepingType[plane_no_per_die];
					for (unsigned int planeID = 0; planeID < plane_no_per_die; planeID++)
					{
						plane_manager[channelID][chipID][dieID][planeID].Total_pages_count = block_no_per_plane * pages_no_per_block;
						plane_manager[channelID][chipID][dieID][planeID].Free_pages_count = block_no_per_plane * pages_no_per_block;
						plane_manager[channelID][chipID][dieID][planeID].Valid_pages_count = 0;
						plane_manager[channelID][chipID][dieID][planeID].Invalid_pages_count = 0;
						plane_manager[channelID][chipID][dieID][planeID].Ongoing_erase_operations.clear();
						plane_manager[channelID][chipID][dieID][planeID].Blocks = new BlockPoolSlotType[block_no_per_plane];
						for (unsigned int blockID = 0; blockID < block_no_per_plane; blockID++)
						{
							plane_manager[channelID][chipID][dieID][planeID].Blocks[blockID].BlockID = blockID;
							plane_manager[channelID][chipID][dieID][planeID].Blocks[blockID].Current_page_write_index = 0;
							plane_manager[channelID][chipID][dieID][planeID].Blocks[blockID].Invalid_page_count = 0;
							plane_manager[channelID][chipID][dieID][planeID].Blocks[blockID].Erase_count = 0; 
							plane_manager[channelID][chipID][dieID][planeID].Blocks[blockID].Holds_mapping_data = false;
							plane_manager[channelID][chipID][dieID][planeID].Blocks[blockID].Erase_transaction = NULL;
								BlockPoolSlotType::Page_vector_size = pages_no_per_block / 64;
							plane_manager[channelID][chipID][dieID][planeID].Blocks[blockID].Invalid_page_bitmap = new uint64_t[BlockPoolSlotType::Page_vector_size];
							for (unsigned int i = 0; i < BlockPoolSlotType::Page_vector_size; i++)
								plane_manager[channelID][chipID][dieID][planeID].Blocks[blockID].Invalid_page_bitmap[i] = All_VALID_PAGE;
							plane_manager[channelID][chipID][dieID][planeID].Free_block_pool.push_back(&plane_manager[channelID][chipID][dieID][planeID].Blocks[blockID]);
						}
						plane_manager[channelID][chipID][dieID][planeID].DataWF = new BlockPoolSlotType*[total_concurrent_streams_no];
						plane_manager[channelID][chipID][dieID][planeID].Translation_wf = new BlockPoolSlotType*[total_concurrent_streams_no];
						for (unsigned int stream_cntr = 0; stream_cntr < total_concurrent_streams_no; stream_cntr++)
						{
							plane_manager[channelID][chipID][dieID][planeID].DataWF[stream_cntr] = plane_manager[channelID][chipID][dieID][planeID].Free_block_pool.front();
							plane_manager[channelID][chipID][dieID][planeID].Block_usage_history.push(plane_manager[channelID][chipID][dieID][planeID].Free_block_pool.front()->BlockID);
							plane_manager[channelID][chipID][dieID][planeID].Free_block_pool.pop_front();
							plane_manager[channelID][chipID][dieID][planeID].DataWF[stream_cntr]->Stream_id = stream_cntr;
							plane_manager[channelID][chipID][dieID][planeID].DataWF[stream_cntr]->Holds_mapping_data = false;
							plane_manager[channelID][chipID][dieID][planeID].Translation_wf[stream_cntr] = plane_manager[channelID][chipID][dieID][planeID].Free_block_pool.front();
							plane_manager[channelID][chipID][dieID][planeID].Block_usage_history.push(plane_manager[channelID][chipID][dieID][planeID].Free_block_pool.front()->BlockID);
							plane_manager[channelID][chipID][dieID][planeID].Free_block_pool.pop_front();
							plane_manager[channelID][chipID][dieID][planeID].Translation_wf[stream_cntr]->Stream_id = stream_cntr;;
							plane_manager[channelID][chipID][dieID][planeID].Translation_wf[stream_cntr]->Holds_mapping_data = true;;
						}
					}
				}
			}
		}
	}

	void Flash_Block_Manager::Allocate_block_and_page_in_plane_for_user_write(const stream_id_type stream_id, NVM::FlashMemory::Physical_Page_Address& page_address, bool is_for_gc)
	{
		PlaneBookKeepingType *plane_record = &plane_manager[page_address.ChannelID][page_address.ChipID][page_address.DieID][page_address.PlaneID];
		plane_record->Free_pages_count--;
		plane_record->Valid_pages_count++;
		page_address.BlockID = plane_record->DataWF[stream_id]->BlockID;
		page_address.PageID = plane_record->DataWF[stream_id]->Current_page_write_index++;
		if(plane_record->DataWF[stream_id]->Current_page_write_index == pages_no_per_block)//The current write frontier block is written to the end
		{
			plane_record->DataWF[stream_id] = plane_record->Free_block_pool.front();//Assign a new write frontier block
			plane_record->Block_usage_history.push(plane_record->Free_block_pool.front()->BlockID);
			plane_record->Free_block_pool.pop_front();
			plane_record->DataWF[stream_id]->Stream_id = stream_id;
			plane_record->DataWF[stream_id]->Holds_mapping_data = false;
			if (!is_for_gc)
				gc_and_wl_unit->Check_gc_required((unsigned int)plane_record->Free_block_pool.size(), page_address);
		}
	}
	
	void Flash_Block_Manager::Allocate_Pages_in_block_and_invalidate_remaining_for_preconditioning(const stream_id_type stream_id, const std::vector<NVM::FlashMemory::Physical_Page_Address>& page_addresses)
	{
		if(page_addresses.size() > pages_no_per_block)
			PRINT_ERROR("The size of the address list is larger than the pages_no_per_block!")
			
		NVM::FlashMemory::Physical_Page_Address target_address(page_addresses[0]);
		PlaneBookKeepingType *plane_record = &plane_manager[target_address.ChannelID][target_address.ChipID][target_address.DieID][target_address.PlaneID];
		if (plane_record->DataWF[stream_id]->Current_page_write_index > 0)
			PRINT_ERROR("The Allocate_Pages_in_block_and_invalidate_remaining_for_preconditioning function should be executed for an erased block!")

		for (auto address : page_addresses)
		{
			plane_record->Free_pages_count--;
			plane_record->Valid_pages_count++;
			address.BlockID = plane_record->DataWF[stream_id]->BlockID;
			address.PageID = plane_record->DataWF[stream_id]->Current_page_write_index++;
		}

		while (plane_record->Free_pages_count > 0)
		{
			target_address.BlockID = plane_record->DataWF[stream_id]->BlockID;
			target_address.PageID = plane_record->DataWF[stream_id]->Current_page_write_index++;
			plane_record->Free_pages_count--;
			Invalidate_page_in_block_for_preconditioning(stream_id, target_address);
		}
		plane_record->DataWF[stream_id] = plane_record->Free_block_pool.front();//Assign a new write frontier block
		plane_record->Block_usage_history.push(plane_record->Free_block_pool.front()->BlockID);
		plane_record->Free_block_pool.pop_front();
		plane_record->DataWF[stream_id]->Stream_id = stream_id;
		plane_record->DataWF[stream_id]->Holds_mapping_data = false;
	}

	void Flash_Block_Manager::Allocate_block_and_page_in_plane_for_translation_write(const stream_id_type streamID, NVM::FlashMemory::Physical_Page_Address& page_address, bool is_for_gc)
	{
		PlaneBookKeepingType *plane_record = &plane_manager[page_address.ChannelID][page_address.ChipID][page_address.DieID][page_address.PlaneID];
		plane_record->Free_pages_count--;
		plane_record->Valid_pages_count++;
		page_address.BlockID = plane_record->Translation_wf[streamID]->BlockID;
		page_address.PageID = plane_record->Translation_wf[streamID]->Current_page_write_index++;
		if (plane_record->Translation_wf[streamID]->Current_page_write_index == pages_no_per_block)//The current write frontier block for translation pages is written to the end
		{
			plane_record->Translation_wf[streamID] = plane_record->Free_block_pool.front();//Assign a new write frontier block
			plane_record->Block_usage_history.push(plane_record->Free_block_pool.front()->BlockID);
			plane_record->Free_block_pool.pop_front();
			plane_record->Translation_wf[streamID]->Stream_id = streamID;
			plane_record->Translation_wf[streamID]->Holds_mapping_data = true;
			if (!is_for_gc)
				gc_and_wl_unit->Check_gc_required((unsigned int)plane_record->Free_block_pool.size(), page_address);
		}
	}

	inline void Flash_Block_Manager::Invalidate_page_in_block(const stream_id_type stream_id, const NVM::FlashMemory::Physical_Page_Address& page_address)
	{
		plane_manager[page_address.ChannelID][page_address.ChipID][page_address.DieID][page_address.PlaneID].Invalid_pages_count++;
		if(plane_manager[page_address.ChannelID][page_address.ChipID][page_address.DieID][page_address.PlaneID].Blocks[page_address.BlockID].Stream_id != stream_id)
			PRINT_ERROR("Inconsistent status occured in the Invalidate_page_in_block function! The accessed block is not allocated to stream " << stream_id)
		plane_manager[page_address.ChannelID][page_address.ChipID][page_address.DieID][page_address.PlaneID].Blocks[page_address.BlockID].Invalid_page_count++;
		plane_manager[page_address.ChannelID][page_address.ChipID][page_address.DieID][page_address.PlaneID].Blocks[page_address.BlockID].Invalid_page_bitmap[page_address.PageID / 64]
			|= ((uint64_t)0x1) << (page_address.PageID % 64);
	}

	inline void Flash_Block_Manager::Invalidate_page_in_block_for_preconditioning(const stream_id_type stream_id, const NVM::FlashMemory::Physical_Page_Address& address)
	{
		Invalidate_page_in_block(stream_id, address);
	}

	void Flash_Block_Manager::Add_erased_block_to_pool(const NVM::FlashMemory::Physical_Page_Address& block_address)
	{
		PlaneBookKeepingType *plane = &plane_manager[block_address.ChannelID][block_address.ChipID][block_address.DieID][block_address.PlaneID];
		BlockPoolSlotType* block = &(plane->Blocks[block_address.BlockID]);
		plane->Free_pages_count += block->Invalid_page_count;
		plane->Invalid_pages_count -= block->Invalid_page_count;

		Stats::Block_erase_histogram[block_address.ChannelID][block_address.ChipID][block_address.DieID][block_address.PlaneID][block->Erase_count]--;
		block->Erase();
		Stats::Block_erase_histogram[block_address.ChannelID][block_address.ChipID][block_address.DieID][block_address.PlaneID][block->Erase_count]++;
		plane->Free_block_pool.push_back(block);
	}

	inline unsigned int Flash_Block_Manager::Get_pool_size(const NVM::FlashMemory::Physical_Page_Address& plane_address)
	{
		return (unsigned int) plane_manager[plane_address.ChannelID][plane_address.ChipID][plane_address.DieID][plane_address.PlaneID].Free_block_pool.size();
	}

	void Flash_Block_Manager::Get_wearleveling_blocks(BlockPoolSlotType*& hotBlock, BlockPoolSlotType*& coldBlock)
	{
	}
}