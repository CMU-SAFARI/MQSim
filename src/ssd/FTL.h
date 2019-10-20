#ifndef FTL_H
#define FTL_H

#include "../sim/Sim_Reporter.h"
#include "../utils/RandomGenerator.h"
#include "NVM_Firmware.h"
#include "TSU_Base.h"
#include "Address_Mapping_Unit_Base.h"
#include "Flash_Block_Manager_Base.h"
#include "GC_and_WL_Unit_Base.h"
#include "NVM_PHY_ONFI.h"
#include "Stats.h"

namespace SSD_Components
{
	enum class SimulationMode { STANDALONE, FULL_SYSTEM };

	class Flash_Block_Manager_Base;
	class Address_Mapping_Unit_Base;
	class GC_and_WL_Unit_Base;
	class TSU_Base;

	class FTL : public NVM_Firmware
	{
	public:
		FTL(const sim_object_id_type& id, Data_Cache_Manager_Base* data_cache, 
			unsigned int channel_no, unsigned int chip_no_per_channel, unsigned int die_no_per_chip, unsigned int plane_no_per_die, 
			unsigned int block_no_per_plane, unsigned int page_no_per_block, unsigned int page_size_in_sectors, 
			sim_time_type avg_flash_read_latency, sim_time_type avg_flash_program_latency, double over_provisioning_ratio, unsigned int max_allowed_block_erase_count, int seed);
		~FTL();
		void Perform_precondition(std::vector<Utils::Workload_Statistics*> workload_stats);
		void Validate_simulation_config();
		void Start_simulation();
		void Execute_simulator_event(MQSimEngine::Sim_Event*);
		LPA_type Convert_host_logical_address_to_device_address(LHA_type lha);
		page_status_type Find_NVM_subunit_access_bitmap(LHA_type lha);
		Address_Mapping_Unit_Base* Address_Mapping_Unit;
		Flash_Block_Manager_Base* BlockManager;
		GC_and_WL_Unit_Base* GC_and_WL_Unit;
		TSU_Base * TSU;
		NVM_PHY_ONFI* PHY;
		void Report_results_in_XML(std::string name_prefix, Utils::XmlWriter& xmlwriter);
	private:
		unsigned int channel_no, chip_no_per_channel, die_no_per_chip, plane_no_per_die;
		unsigned int block_no_per_plane, page_no_per_block, page_size_in_sectors;
		unsigned int max_allowed_block_erase_count;
		int preconditioning_seed;
		Utils::RandomGenerator random_generator;
		double over_provisioning_ratio;
		sim_time_type avg_flash_read_latency;
		sim_time_type avg_flash_program_latency;
	};
}

#endif // !FTL_H
