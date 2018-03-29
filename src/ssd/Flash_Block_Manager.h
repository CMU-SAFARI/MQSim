#ifndef FLASH_BLOCK_MANAGER_H
#define FLASH_BLOCK_MANAGER_H

#include <list>
#include "Flash_Block_Manager_Base.h"
#include "../nvm_chip/flash_memory/FlashTypes.h"
#include "../nvm_chip/flash_memory/Physical_Page_Address.h"

namespace SSD_Components
{
	class Flash_Block_Manager : public Flash_Block_Manager_Base
	{
	public:
		Flash_Block_Manager(GC_and_WL_Unit_Base* gc_and_wl_unit, unsigned int MaxAllowedBlockEraseCount, unsigned int TotalStreamNo, 
			unsigned int ChannelCount, unsigned int ChipNoPerChannel, unsigned int DieNoPerChip, unsigned int PlaneNoPerDie,
			unsigned int Block_no_per_plane, unsigned int Page_no_per_block);
		void Allocate_block_and_page_in_plane_for_user_write(const stream_id_type streamID, NVM::FlashMemory::Physical_Page_Address& address, bool is_for_gc);
		void Allocate_block_and_page_in_plane_for_translation_write(const stream_id_type streamID, NVM::FlashMemory::Physical_Page_Address& address, bool is_for_gc);
		void Invalidate_page_in_block(const stream_id_type streamID, const NVM::FlashMemory::Physical_Page_Address& address);
		void Add_erased_block_to_pool(const NVM::FlashMemory::Physical_Page_Address& address);
		void Get_wearleveling_blocks(BlockPoolSlotType*& hotBlock, BlockPoolSlotType*& coldBlock);
		unsigned int Get_pool_size(const NVM::FlashMemory::Physical_Page_Address& plane_address);
	private:
		unsigned int channel_count;
		unsigned int chip_no_per_channel;
		unsigned int die_no_per_chip;
		unsigned int plane_no_per_die;
		unsigned int block_no_per_plane;
		unsigned int pages_no_per_block;
	};
}

#endif