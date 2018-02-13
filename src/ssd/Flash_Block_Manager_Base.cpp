#include "Flash_Block_Manager.h"


namespace SSD_Components
{
	unsigned int BlockPoolSlotType::PageVectorSize = 0;
	Flash_Block_Manager_Base::Flash_Block_Manager_Base(GC_and_WL_Unit_Base* gc_and_wl_unit, unsigned int MaxAllowedBlockEraseCount, unsigned int total_concurrent_streams_no)
		: gc_and_wl_unit(gc_and_wl_unit), max_allowed_block_erase_count(MaxAllowedBlockEraseCount), total_concurrent_streams_no(total_concurrent_streams_no) {}

	void Flash_Block_Manager_Base::Set_GC_and_WL_Unit(GC_and_WL_Unit_Base* gcwl) { this->gc_and_wl_unit = gcwl; }
}