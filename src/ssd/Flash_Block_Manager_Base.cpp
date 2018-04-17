#include "Flash_Block_Manager.h"


namespace SSD_Components
{
	unsigned int BlockPoolSlotType::Page_vector_size = 0;
	Flash_Block_Manager_Base::Flash_Block_Manager_Base(GC_and_WL_Unit_Base* gc_and_wl_unit, unsigned int MaxAllowedBlockEraseCount, unsigned int total_concurrent_streams_no,
		unsigned int channel_count, unsigned int chip_no_per_channel, unsigned int die_no_per_chip, unsigned int plane_no_per_die,
		unsigned int block_no_per_plane, unsigned int page_no_per_block)
		: gc_and_wl_unit(gc_and_wl_unit), max_allowed_block_erase_count(MaxAllowedBlockEraseCount), total_concurrent_streams_no(total_concurrent_streams_no),
		channel_count(channel_count), chip_no_per_channel(chip_no_per_channel), die_no_per_chip(die_no_per_chip), plane_no_per_die(plane_no_per_die),
		block_no_per_plane(block_no_per_plane), pages_no_per_block(page_no_per_block)
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
							BlockPoolSlotType::Page_vector_size = pages_no_per_block / (sizeof(uint64_t) * 8) + (pages_no_per_block % (sizeof(uint64_t) * 8) == 0 ? 0 : 1);
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

	Flash_Block_Manager_Base::~Flash_Block_Manager_Base() 
	{
		for (unsigned int channel_id = 0; channel_id < channel_count; channel_id++)
		{
			for (unsigned int chip_id = 0; chip_id < chip_no_per_channel; chip_id++)
			{
				for (unsigned int die_id = 0; die_id < die_no_per_chip; die_id++)
				{
					for (unsigned int plane_id = 0; plane_id < plane_no_per_die; plane_id++)
					{
						for (unsigned int blockID = 0; blockID < block_no_per_plane; blockID++)
							delete[] plane_manager[channel_id][chip_id][die_id][plane_id].Blocks[blockID].Invalid_page_bitmap;
						delete[] plane_manager[channel_id][chip_id][die_id][plane_id].Blocks;
						delete[] plane_manager[channel_id][chip_id][die_id][plane_id].DataWF;
						delete[] plane_manager[channel_id][chip_id][die_id][plane_id].Translation_wf;
					}
					delete[] plane_manager[channel_id][chip_id][die_id];
				}
				delete[] plane_manager[channel_id][chip_id];
			}
			delete[] plane_manager[channel_id];
		}
		delete[] plane_manager;
	}

	void Flash_Block_Manager_Base::Set_GC_and_WL_Unit(GC_and_WL_Unit_Base* gcwl) { this->gc_and_wl_unit = gcwl; }

	void BlockPoolSlotType::Erase()
	{
		Current_page_write_index = 0;
		Invalid_page_count = 0;
		Erase_count++;
		for (unsigned int i = 0; i < BlockPoolSlotType::Page_vector_size; i++)
			Invalid_page_bitmap[i] = All_VALID_PAGE;
		Stream_id = NO_STREAM;
		Holds_mapping_data = false;
		Erase_transaction = NULL;
	}
}