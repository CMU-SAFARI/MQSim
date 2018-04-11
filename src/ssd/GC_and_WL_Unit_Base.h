#ifndef GC_AND_WL_UNIT_BASE_H
#define GC_AND_WL_UNIT_BASE_H

#include "../sim/Sim_Object.h"
#include "../nvm_chip/flash_memory/Flash_Chip.h"
#include "../nvm_chip/flash_memory/Physical_Page_Address.h"
#include "Address_Mapping_Unit_Base.h"
#include "Flash_Block_Manager_Base.h"
#include "TSU_Base.h"
#include "NVM_PHY_ONFI.h"


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

	class Address_Mapping_Unit_Base;
	class Flash_Block_Manager_Base;
	class TSU_Base;
	class NVM_PHY_ONFI;
	/*
	* This class implements thet the Garbage Collection and Wear Leveling module of MQSim.
	*/
	class GC_and_WL_Unit_Base : public MQSimEngine::Sim_Object
	{
	public:
		GC_and_WL_Unit_Base(const sim_object_id_type& id, 
			Address_Mapping_Unit_Base* address_mapping_unit, Flash_Block_Manager_Base* block_manager, TSU_Base* tsu, NVM_PHY_ONFI* flash_controller,
			GC_Block_Selection_Policy_Type block_selection_policy, double gc_threshold,	bool preemptible_gc_enabled, double gc_hard_threshold,
			unsigned int channel_count, unsigned int chip_no_per_channel, unsigned int die_no_per_chip, unsigned int plane_no_per_die,
			unsigned int block_no_per_plane, unsigned int page_no_per_block, unsigned int sector_no_per_page);
		void Setup_triggers();
		void Start_simulation();
		void Validate_simulation_config();
		void Execute_simulator_event(MQSimEngine::Sim_Event*);

		virtual bool GC_is_in_urgent_mode(const NVM::FlashMemory::Flash_Chip*) = 0;
		virtual void Check_gc_required(const unsigned int BlockPoolSize, const NVM::FlashMemory::Physical_Page_Address& planeAddress) = 0;
		virtual void Check_wl_required(const double staticWLFactor, const NVM::FlashMemory::Physical_Page_Address planeAddress) = 0;
		GC_Block_Selection_Policy_Type Get_gc_policy();
	protected:
		GC_Block_Selection_Policy_Type block_selection_policy;
		static GC_and_WL_Unit_Base * _my_instance;
		Address_Mapping_Unit_Base* address_mapping_unit;
		Flash_Block_Manager_Base* block_manager;
		TSU_Base* tsu;
		NVM_PHY_ONFI* flash_controller;
		bool force_gc;
		double gc_threshold;//As the ratio of free pages to the total number of physical pages
		unsigned int block_pool_gc_threshold;
		static void handle_transaction_serviced_signal_from_PHY(NVM_Transaction_Flash* transaction);

		//Used to implement: "Preemptible I/O Scheduling of Garbage Collection for Solid State Drives", TCAD 2013.
		bool preemptible_gc_enabled;
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
