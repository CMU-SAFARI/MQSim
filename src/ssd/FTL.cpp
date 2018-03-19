#include <stdexcept>
#include "FTL.h"

namespace SSD_Components
{
	FTL::FTL(const sim_object_id_type& id, Data_Cache_Manager_Base* data_cache_manager) :
		NVM_Firmware(id, data_cache_manager)
	{}

	void FTL::Validate_simulation_config()
	{
		NVM_Firmware::Validate_simulation_config();
		if (this->Data_cache_manager == NULL)
			throw std::logic_error("The cache manager is not set for FTL!");
		if (this->Address_Mapping_Unit == NULL)
			throw std::logic_error("The mapping module is not set for FTL!");
		if (this->BlockManager == NULL)
			throw std::logic_error("The block manager is not set for FTL!");
		if (this->GC_and_WL_Unit == NULL)
			throw std::logic_error("The garbage collector is not set for FTL!");
	}

	void FTL::Perform_precondition()
	{

	}


	void FTL::Start_simulation() {}
	void FTL::Execute_simulator_event(MQSimEngine::Sim_Event*) {}

	void FTL::handle_user_request(User_Request* user_request)
	{
	}

}
