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
		FIFO						/*The FIFO algortihm described in P. Desnoyers, "Analytic  Modeling  of  SSD Write
									Performance, SYSTOR, 2012".*/
		};
	class GC_and_WL_Unit_Page_Level : public GC_and_WL_Unit_Base
	{
	public:
		GC_and_WL_Unit_Page_Level(const sim_object_id_type& id,
			Address_Mapping_Unit_Base* address_mapping_unit, Flash_Block_Manager_Base* block_manager, TSU_Base* tsu, NVM_PHY_ONFI* flash_controller,
			GC_Block_Selection_Policy_Type block_selection_policy, double gc_threshold, bool preemptible_gc_enabled, double gc_hard_threshold,
			unsigned int channel_count, unsigned int chip_no_per_channel, unsigned int die_no_per_chip, unsigned int plane_no_per_die,
			unsigned int block_no_per_plane, unsigned int page_no_per_block, unsigned int sectors_per_page, 
			bool use_copyback, int seed = 432, unsigned int max_ongoing_gc_reqs_per_plane = 10);

		/*This function is used for implementing preemptible GC execution. If for a flash chip the free block
		* pool becomes close to empty, then the GC requests for that flash chip should be prioritized and
		* GC should go on in non-preemptible mode.*/
		bool GC_is_in_urgent_mode(const NVM::FlashMemory::Flash_Chip*);

		void Check_gc_required(const unsigned int free_block_pool_size, const NVM::FlashMemory::Physical_Page_Address& plane_address);
		void Check_wl_required(const double static_wl_factor, const NVM::FlashMemory::Physical_Page_Address plane_address);
	private:
		GC_Block_Selection_Policy_Type block_selection_policy;
		NVM_PHY_ONFI * flash_controller;
		bool use_copyback;
		bool is_wf(PlaneBookKeepingType* pbke, flash_block_ID_type gc_candidate_block_id);

		//Following variabels are used based on the type of GC block selection policy
		unsigned int rga_set_size;//The number of random flash blocks that are radnomly selected 
		Utils::RandomGenerator random_generator;
		unsigned int random_pp_threshold;
		std::queue<BlockPoolSlotType*> block_usage_fifo;
		unsigned int max_ongoing_gc_reqs_per_plane;
	};
}
#endif // !GC_AND_WL_UNIT_PAGE_LEVEL_H
