#ifndef BLOCK_POOL_MANAGER_BASE_H
#define BLOCK_POOL_MANAGER_BASE_H

#include <list>
#include <cstdint>
#include <queue>
#include "../nvm_chip/flash_memory/FlashTypes.h"
#include "../nvm_chip/flash_memory/Physical_Page_Address.h"
#include "GC_and_WL_Unit_Base.h"

namespace SSD_Components
{
#define All_VALID_PAGE 0x0000000000000000ULL
	class GC_and_WL_Unit_Base;
	struct BlockPoolSlotType
	{
		flash_block_ID_type BlockID;
		flash_page_ID_type CurrentPageWriteIndex;
		unsigned int InvalidPageCount;
		unsigned int EraseCount;
		static unsigned int PageVectorSize;
		uint64_t* InvalidPageVector;//A bit sequence that keeps track of valid/invalid status of pages in the block. A "0" means valid, and a "1" means invalid.
		stream_id_type Stream_id = NO_STREAM;
		bool HoldsMappingData = false;
	};
	class PlaneBookKeepingType
	{
	public:
		unsigned int TotalPagesCount;
		unsigned int FreePagesCount;
		unsigned int ValidPagesCount;
		unsigned int InvalidPagesCount;
		BlockPoolSlotType* Blocks;
		std::list<BlockPoolSlotType*> FreeBlockPool;
		BlockPoolSlotType** DataWF, **TranslationWF; //The write frontier blocks for data and translation pages
		unsigned int* BlockEraseCount;
		bool Ongoing_gc_reqs_count;
		std::queue<flash_block_ID_type> BlockUsageHistory;//A fifo queue that keeps track of flash blocks based on their usage history
	};

	class Flash_Block_Manager_Base
	{
	public:
		Flash_Block_Manager_Base(GC_and_WL_Unit_Base* gc_and_wl_unit, unsigned int MaxAllowedBlockEraseCount, unsigned int total_concurrent_streams_no);
		virtual void Allocate_block_and_page_in_plane_for_user_write(const stream_id_type streamID, NVM::FlashMemory::Physical_Page_Address& address) = 0;
		virtual void Allocate_block_and_page_in_plane_for_translation_write(const stream_id_type streamID, NVM::FlashMemory::Physical_Page_Address& address) = 0;
		virtual void Invalidate_page_in_block(const stream_id_type streamID, const NVM::FlashMemory::Physical_Page_Address& address) = 0;
		virtual void Add_erased_block_to_pool(const NVM::FlashMemory::Physical_Page_Address& address) = 0;
		virtual void GetWearlevelingBlocks(BlockPoolSlotType*& hotBlock, BlockPoolSlotType*& coldBlock) = 0;
		virtual unsigned int Get_pool_size(const NVM::FlashMemory::Physical_Page_Address& plane_address) = 0;
		void Set_GC_and_WL_Unit(GC_and_WL_Unit_Base* );
	protected:
		GC_and_WL_Unit_Base* gc_and_wl_unit;
		unsigned int max_allowed_block_erase_count;
		unsigned int total_concurrent_streams_no;
	};
}

#endif//!BLOCK_POOL_MANAGER_BASE_H
