#ifndef GC_AND_WL_UNIT_BASE_H
#define GC_AND_WL_UNIT_BASE_H

#include "../nvm_chip/flash_memory/Chip.h"
#include "../nvm_chip/flash_memory/Physical_Page_Address.h"
#include "FTL.h"
#include "Flash_Block_Manager_Base.h"


namespace SSD_Components
{
	class FTL;
	class Flash_Block_Manager_Base;
	/*
	* This class implements thet the Garbage Collection and Wear Leveling module of MQSim.
	*/
	class GC_and_WL_Unit_Base
	{
	public:
		GC_and_WL_Unit_Base(FTL* ftl, Flash_Block_Manager_Base* BlockManager, double GCThreshold,
			bool PreemptibleGCEnabled, double GCHardThreshold,
			unsigned int ChannelCount, unsigned int ChipNoPerChannel, unsigned int DieNoPerChip, unsigned int PlaneNoPerDie,
			unsigned int Block_no_per_plane, unsigned int Page_no_per_block, unsigned int SectorsPerPage);
		virtual bool GC_is_in_urgent_mode(const NVM::FlashMemory::Chip*) = 0;
		virtual void Check_gc_required(const unsigned int BlockPoolSize, const NVM::FlashMemory::Physical_Page_Address& planeAddress) = 0;
		virtual void Check_wl_required(const double staticWLFactor, const NVM::FlashMemory::Physical_Page_Address planeAddress) = 0;
	protected:
		FTL* ftl;
		bool force_gc;
		Flash_Block_Manager_Base* block_manager;
		double gc_threshold;//As the ratio of free pages to the total number of physical pages
		unsigned int block_pool_gc_threshold;

		//Used to implement: "Preemptible I/O Scheduling of Garbage Collection for Solid State Drives", TCAD 2013.
		bool preemptibleGCEnabled;
		double gc_hard_threshold;
		unsigned int block_pool_gc_hard_threshold;

		unsigned int channel_count;
		unsigned int chip_no_per_channel;
		unsigned int die_no_per_chip;
		unsigned int plane_no_per_die;
		unsigned int block_no_per_plane;
		unsigned int pages_no_per_block;
		unsigned int sector_no_per_page;
	};
}

#endif // !GC_AND_WL_UNIT_BASE_H
