
#include "../nvm_chip/flash_memory/Physical_Page_Address.h"
#include "Flash_Block_Manager.h"

namespace SSD_Components
{
	Flash_Block_Manager::Flash_Block_Manager(GC_and_WL_Unit_Base* gc_and_wl_unit, unsigned int MaxAllowedBlockEraseCount, unsigned int TotalStreamNo,
		unsigned int ChannelCount, unsigned int ChipNoPerChannel, unsigned int DieNoPerChip, unsigned int PlaneNoPerDie,
		unsigned int Block_no_per_plane, unsigned int Page_no_per_block)
		: Flash_Block_Manager_Base(gc_and_wl_unit, MaxAllowedBlockEraseCount, TotalStreamNo),
		channel_count(ChannelCount), chip_no_per_channel(ChipNoPerChannel), die_no_per_chip(DieNoPerChip), plane_no_per_die(PlaneNoPerDie),
		block_no_per_plane(Block_no_per_plane), pages_no_per_block(Page_no_per_block)
	{
		planeManager = new PlaneBookKeepingType***[channel_count];
		for (unsigned int channelID = 0; channelID < channel_count; channelID++)
		{
			planeManager[channelID] = new PlaneBookKeepingType**[chip_no_per_channel];
			for (unsigned int chipID = 0; chipID < chip_no_per_channel; chipID++)
			{
				planeManager[channelID][chipID] = new PlaneBookKeepingType*[die_no_per_chip];
				for (unsigned int dieID = 0; dieID < die_no_per_chip; dieID++)
				{
					planeManager[channelID][chipID][dieID] = new PlaneBookKeepingType[plane_no_per_die];
					for (unsigned int planeID = 0; planeID < plane_no_per_die; planeID++)
					{
						planeManager[channelID][chipID][dieID][planeID].TotalPagesCount = block_no_per_plane * pages_no_per_block;
						planeManager[channelID][chipID][dieID][planeID].FreePagesCount = block_no_per_plane * pages_no_per_block;
						planeManager[channelID][chipID][dieID][planeID].ValidPagesCount = 0;
						planeManager[channelID][chipID][dieID][planeID].InvalidPagesCount = 0;
						planeManager[channelID][chipID][dieID][planeID].BlockEraseCount = new unsigned int[max_allowed_block_erase_count];
						planeManager[channelID][chipID][dieID][planeID].BlockEraseCount[0] = block_no_per_plane * pages_no_per_block; //At the start of the simulation all pages have zero erase count
						for (unsigned int i = 1; i < max_allowed_block_erase_count; ++i)
							planeManager[channelID][chipID][dieID][planeID].BlockEraseCount[i] = 0;
						planeManager[channelID][chipID][dieID][planeID].Blocks = new BlockPoolSlotType[block_no_per_plane];
						for (unsigned int blockID = 0; blockID < block_no_per_plane; blockID++)
						{
							planeManager[channelID][chipID][dieID][planeID].Blocks[blockID].BlockID = blockID;
							planeManager[channelID][chipID][dieID][planeID].Blocks[blockID].CurrentPageWriteIndex = 0;
							planeManager[channelID][chipID][dieID][planeID].Blocks[blockID].InvalidPageCount = 0;
							planeManager[channelID][chipID][dieID][planeID].Blocks[blockID].EraseCount = 0;
							planeManager[channelID][chipID][dieID][planeID].Blocks[blockID].HoldsMappingData = false;
							BlockPoolSlotType::PageVectorSize = pages_no_per_block / 64;
							planeManager[channelID][chipID][dieID][planeID].Blocks[blockID].InvalidPageVector = new uint64_t[BlockPoolSlotType::PageVectorSize];
							for (unsigned int i = 0; i < BlockPoolSlotType::PageVectorSize; i++)
								planeManager[channelID][chipID][dieID][planeID].Blocks[blockID].InvalidPageVector[i] = All_VALID_PAGE;
							planeManager[channelID][chipID][dieID][planeID].FreeBlockPool.push_back(&planeManager[channelID][chipID][dieID][planeID].Blocks[blockID]);
						}
						planeManager[channelID][chipID][dieID][planeID].DataWF = new BlockPoolSlotType*[total_concurrent_streams_no];
						planeManager[channelID][chipID][dieID][planeID].TranslationWF = new BlockPoolSlotType*[total_concurrent_streams_no];
						for (unsigned int i = 0; i < total_concurrent_streams_no; i++)
						{
							planeManager[channelID][chipID][dieID][planeID].DataWF[i] = planeManager[channelID][chipID][dieID][planeID].FreeBlockPool.front();
							planeManager[channelID][chipID][dieID][planeID].FreeBlockPool.pop_front();
							planeManager[channelID][chipID][dieID][planeID].DataWF[i]->HoldsMappingData = false;
							planeManager[channelID][chipID][dieID][planeID].TranslationWF[i] = planeManager[channelID][chipID][dieID][planeID].FreeBlockPool.front();
							planeManager[channelID][chipID][dieID][planeID].TranslationWF[i]->HoldsMappingData = true;;
							planeManager[channelID][chipID][dieID][planeID].FreeBlockPool.pop_front();
						}
					}
				}
			}
		}
	}

	void Flash_Block_Manager::Allocate_block_and_page_in_plane_for_user_write(const stream_id_type streamID, NVM::FlashMemory::Physical_Page_Address& PageAddress)
	{
		PlaneBookKeepingType *plane_record = &planeManager[PageAddress.ChannelID][PageAddress.ChipID][PageAddress.DieID][PageAddress.PlaneID];
		plane_record->FreePagesCount--;
		plane_record->ValidPagesCount++;
		PageAddress.BlockID = plane_record->DataWF[streamID]->BlockID;
		PageAddress.PageID = plane_record->DataWF[streamID]->CurrentPageWriteIndex++;
		if(plane_record->DataWF[streamID]->CurrentPageWriteIndex == pages_no_per_block)//The current write frontier block is written to the end
		{
			plane_record->DataWF[streamID] = plane_record->FreeBlockPool.front();//Assign a new write frontier block
			plane_record->BlockUsageHistory.push(plane_record->FreeBlockPool.front()->BlockID);
			plane_record->FreeBlockPool.pop_front();
			plane_record->DataWF[streamID]->Stream_id = streamID;
			plane_record->DataWF[streamID]->HoldsMappingData = false;
			gc_and_wl_unit->Check_gc_required((unsigned int)plane_record->FreeBlockPool.size(), PageAddress);
		}
	}
	void Flash_Block_Manager::Allocate_block_and_page_in_plane_for_translation_write(const stream_id_type streamID, NVM::FlashMemory::Physical_Page_Address& PageAddress)
	{
		PlaneBookKeepingType *plane = &planeManager[PageAddress.ChannelID][PageAddress.ChipID][PageAddress.DieID][PageAddress.PlaneID];
		PageAddress.BlockID = plane->TranslationWF[streamID]->BlockID;
		PageAddress.PageID = plane->TranslationWF[streamID]->CurrentPageWriteIndex++;
		if (plane->TranslationWF[streamID]->CurrentPageWriteIndex == pages_no_per_block)//The current write frontier block for translation pages is written to the end
		{
			plane->TranslationWF[streamID] = plane->FreeBlockPool.front();//Assign a new write frontier block
			plane->FreeBlockPool.pop_front();
			plane->TranslationWF[streamID]->Stream_id = streamID;
			plane->TranslationWF[streamID]->HoldsMappingData = true;
			gc_and_wl_unit->Check_gc_required((unsigned int)plane->FreeBlockPool.size(), PageAddress);
		}
	}
	void Flash_Block_Manager::Invalidate_page_in_block(const stream_id_type streamID, const NVM::FlashMemory::Physical_Page_Address& PageAddress)
	{
		planeManager[PageAddress.ChannelID][PageAddress.ChipID][PageAddress.DieID][PageAddress.PlaneID].InvalidPagesCount++;
		planeManager[PageAddress.ChannelID][PageAddress.ChipID][PageAddress.DieID][PageAddress.PlaneID].Blocks[PageAddress.BlockID].InvalidPageCount++;
		planeManager[PageAddress.ChannelID][PageAddress.ChipID][PageAddress.DieID][PageAddress.PlaneID].Blocks[PageAddress.BlockID].InvalidPageVector[PageAddress.PageID / 64]
			|= ((uint64_t)0x1) << (PageAddress.PageID % 64);
	}

	void Flash_Block_Manager::Add_erased_block_to_pool(const NVM::FlashMemory::Physical_Page_Address& BloackAddress)
	{
		PlaneBookKeepingType *plane = &planeManager[BloackAddress.ChannelID][BloackAddress.ChipID][BloackAddress.DieID][BloackAddress.PlaneID];
		BlockPoolSlotType* block = &(plane->Blocks[BloackAddress.BlockID]);
		plane->BlockEraseCount++;
		plane->FreePagesCount += block->InvalidPageCount;
		plane->InvalidPagesCount -= block->InvalidPageCount;

		block->CurrentPageWriteIndex = 0;
		plane->BlockEraseCount[block->EraseCount]--;
		block->EraseCount++;
		plane->BlockEraseCount[block->EraseCount]++;
		block->HoldsMappingData = false;
		block->InvalidPageCount = 0;
		for (unsigned int i = 0; i < BlockPoolSlotType::PageVectorSize; i++)
			block->InvalidPageVector[i] = All_VALID_PAGE;
		plane->FreeBlockPool.push_back(block);
	}
	inline unsigned int Flash_Block_Manager::Get_pool_size(const NVM::FlashMemory::Physical_Page_Address& plane_address)
	{
		return (unsigned int) planeManager[plane_address.ChannelID][plane_address.ChipID][plane_address.DieID][plane_address.PlaneID].FreeBlockPool.size();
	}
	void Flash_Block_Manager::GetWearlevelingBlocks(BlockPoolSlotType*& hotBlock, BlockPoolSlotType*& coldBlock)
	{
	}
}