#include "GC_and_WL_Unit_Base.h"

namespace SSD_Components
{
	GC_and_WL_Unit_Base::GC_and_WL_Unit_Base(FTL* ftl, Flash_Block_Manager_Base* BlockManager, double gc_threshold,
		bool PreemptibleGCEnabled, double gc_hard_threshold,
		unsigned int ChannelCount, unsigned int ChipNoPerChannel, unsigned int DieNoPerChip, unsigned int PlaneNoPerDie,
		unsigned int Block_no_per_plane, unsigned int Page_no_per_block, unsigned int SectorsPerPage) :
		ftl(ftl), force_gc(false), block_manager(BlockManager), gc_threshold(gc_threshold),
		preemptibleGCEnabled(), gc_hard_threshold(gc_hard_threshold),
		channel_count(ChannelCount), chip_no_per_channel(ChipNoPerChannel), die_no_per_chip(DieNoPerChip), plane_no_per_die(PlaneNoPerDie),
		block_no_per_plane(Block_no_per_plane), pages_no_per_block(Page_no_per_block), sector_no_per_page(SectorsPerPage)
	{
		block_pool_gc_threshold = (unsigned int)(gc_threshold * (double)block_no_per_plane);
		block_pool_gc_hard_threshold = (unsigned int)(gc_hard_threshold * (double)block_no_per_plane);
	}

}