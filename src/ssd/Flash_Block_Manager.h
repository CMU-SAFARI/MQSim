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
		Flash_Block_Manager(GC_and_WL_Unit_Base* gc_and_wl_unit, unsigned int max_allowed_block_erase_count, unsigned int total_concurrent_streams_no,
			unsigned int channel_count, unsigned int chip_no_per_channel, unsigned int die_no_per_chip, unsigned int plane_no_per_die,
			unsigned int block_no_per_plane, unsigned int page_no_per_block);
		~Flash_Block_Manager();
		void Allocate_block_and_page_in_plane_for_user_write(const stream_id_type stream_id, NVM::FlashMemory::Physical_Page_Address& address);
		void Allocate_block_and_page_in_plane_for_gc_write(const stream_id_type stream_id, NVM::FlashMemory::Physical_Page_Address& address);
		void Allocate_Pages_in_block_and_invalidate_remaining_for_preconditioning(const stream_id_type stream_id, const NVM::FlashMemory::Physical_Page_Address& plane_address, std::vector<NVM::FlashMemory::Physical_Page_Address>& page_addresses);
		void Allocate_block_and_page_in_plane_for_translation_write(const stream_id_type stream_id, NVM::FlashMemory::Physical_Page_Address& address, bool is_for_gc);
		void Invalidate_page_in_block(const stream_id_type streamID, const NVM::FlashMemory::Physical_Page_Address& address);
		void Invalidate_page_in_block_for_preconditioning(const stream_id_type streamID, const NVM::FlashMemory::Physical_Page_Address& address);
		void Add_erased_block_to_pool(const NVM::FlashMemory::Physical_Page_Address& address);
		unsigned int Get_pool_size(const NVM::FlashMemory::Physical_Page_Address& plane_address);
	private:
	};
}

#endif // !FLASH_BLOCK_MANAGER_H
