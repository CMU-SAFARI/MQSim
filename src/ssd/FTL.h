#ifndef FTL_H
#define FTL_H

#include "../sim/Sim_Reporter.h"
#include "../utils/RandomGenerator.h"
#include "Data_Cache_Manager_Flash.h"
#include "NVM_Firmware.h"
#include "TSU_Base.h"
#include "Address_Mapping_Unit_Base.h"
#include "Flash_Block_Manager_Base.h"
#include "GC_and_WL_Unit_Base.h"

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
		FTL(const sim_object_id_type& id, Data_Cache_Manager_Base* data_cache, int seed);
		void Perform_precondition(std::vector<Preconditioning::Workload_Statistics*> workload_stats);
		void Validate_simulation_config();
		void Start_simulation();
		void Execute_simulator_event(MQSimEngine::Sim_Event*);
		Address_Mapping_Unit_Base* Address_Mapping_Unit;
		Flash_Block_Manager_Base* BlockManager;
		GC_and_WL_Unit_Base* GC_and_WL_Unit;
		TSU_Base * TSU;
		void Report_results_in_XML(std::string name_prefix, Utils::XmlWriter& xmlwriter);
	private:
		int preconditioning_seed;
		Utils::RandomGenerator random_generator;
	};
}


#endif // !FTL_H
