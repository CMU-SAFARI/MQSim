#include "NVM_Firmware.h"

namespace SSD_Components
{
	NVM_Firmware::NVM_Firmware(const sim_object_id_type& id, Data_Cache_Manager_Base* data_cache_manager)
		: MQSimEngine::Sim_Object(id), Data_cache_manager(data_cache_manager)
	{
	}

	void NVM_Firmware::Validate_simulation_config()
	{
	}
}