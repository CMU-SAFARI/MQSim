#ifndef FTL_H
#define FTL_H

#include "Data_Cache_Manager_Flash.h"
#include "NVM_Firmware.h"
#include "TSU_Base.h"
#include "Address_Mapping_Unit_Base.h"
#include "Flash_Block_Manager_Base.h"
#include "GC_and_WL_Unit_Base.h"

namespace SSD_Components
{

	enum class InitialSimulationStatus { EMPTY, STEADY_STATE }; //Empty: All pages are free, STEADY_STATE: pages are invalidated/validated based on the input workload behavior
	enum class SimulationMode { STANDALONE, FULL_SYSTEM };

	class Flash_Block_Manager_Base;
	class Address_Mapping_Unit_Base;
	class GC_and_WL_Unit_Base;
	class TSU_Base;
	class FTL : public NVM_Firmware
	{
	public:
		FTL(const sim_object_id_type& id, Data_Cache_Manager_Base* data_cache);
		void Perform_precondition();
		void Validate_simulation_config();
		void Start_simulation();
		void Execute_simulator_event(MQSimEngine::Sim_Event*);
		Address_Mapping_Unit_Base* Address_Mapping_Unit;
		Flash_Block_Manager_Base* BlockManager;
		GC_and_WL_Unit_Base* GC_and_WL_Unit;
		TSU_Base * TSU;
	private:
		void handle_user_request(User_Request* user_request);
	};
}


#endif // !FTL_H
