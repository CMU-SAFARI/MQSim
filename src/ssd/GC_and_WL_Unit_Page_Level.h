#ifndef GC_AND_WL_UNIT_PAGE_LEVEL_H
#define GC_AND_WL_UNIT_PAGE_LEVEL_H

#include "GC_and_WL_Unit_Base.h"
#include "NVM_PHY_ONFI.h"
#include "../utils/RandomGenerator.h"
#include <queue>


namespace SSD_Components
{
	enum class GC_Block_Selection_Policy_Type {
		GREEDY,
		RGA,						/*The randomized-greedy algorithm described in: "B. Van Houdt, A Mean Field Model 
									for a Class of Garbage Collection Algorithms in Flash - based Solid State Drives,
									SIGMETRICS, 2013" and "Stochastic Modeling of Large-Scale Solid-State Storage
									Systems: Analysis, Design Tradeoffs and Optimization, SIGMETRICS, 2013".*/
		RANDOM, RANDOM_P, RANDOM_PP,/*The RANDOM, RANDOM+, and RANDOM++ algorithms described in: "B. Van Houdt, A Mean
									Field Model  for a Class of Garbage Collection Algorithms in Flash - based Solid
									State Drives, SIGMETRICS, 2013".*/
		FIFO						/*The FIFO algortihm described in "P. Desnoyers, "Analytic  Modeling  of  SSD Write
									Performance, SYSTOR, 2012".*/
		};
	class GC_and_WL_Unit_Page_Level : public GC_and_WL_Unit_Base
	{
	public:
		GC_and_WL_Unit_Page_Level(FTL* ftl, Flash_Block_Manager_Base* BlockManager, double GCThreshold, 
			GC_Block_Selection_Policy_Type BlockSelectionPolicy,
			bool PreemptibleGCEnabled, double GCHardThreshold,
			unsigned int ChannelCount, unsigned int ChipNoPerChannel, unsigned int DieNoPerChip, unsigned int PlaneNoPerDie,
			unsigned int Block_no_per_plane, unsigned int Page_no_per_block, unsigned int SectorsPerPage, int seed);
		void Setup_triggers();

		/*This function is used for implementing preemptible GC execution. If for a flash chip the free block
		* pool becomes close to empty, then the GC requests for that flash chip should be prioritized and
		* GC should go on in non-preemptible mode.*/
		bool GC_is_in_urgent_mode(const NVM::FlashMemory::Chip*);

		void Check_gc_required(const unsigned int free_block_pool_size, const NVM::FlashMemory::Physical_Page_Address& plane_address);
		void Check_wl_required(const double static_wl_factor, const NVM::FlashMemory::Physical_Page_Address plane_address);
	private:
		GC_Block_Selection_Policy_Type block_selection_policy;
		NVM_PHY_ONFI * flash_controller;
		static void handle_transaction_serviced_signal_from_PHY(NVM_Transaction_Flash* transaction);

		//Following variabels are used based on the type of GC block selection policy
		unsigned int rga_set_size;//The number of random flash blocks that are radnomly selected 
		Utils::RandomGenerator random_generator;
		unsigned int random_pp_threshold;
		std::queue<BlockPoolSlotType*> block_usage_fifo;
	};
}
#endif // !GC_AND_WL_UNIT_PAGE_LEVEL_H
