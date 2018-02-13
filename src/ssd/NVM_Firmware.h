#ifndef NVM_FIRMWARE_H
#define NVM_FIRMWARE_H

#include <vector>
#include "../sim/Sim_Object.h"
#include "NVM_Transaction.h"
#include "Data_Cache_Manager_Base.h"

namespace SSD_Components
{
	class Data_Cache_Manager_Base;
	class NVM_Firmware : public MQSimEngine::Sim_Object
	{
	public:
		NVM_Firmware(const sim_object_id_type& id, Data_Cache_Manager_Base* data_cache_manager);
		void Validate_simulation_config();
		Data_Cache_Manager_Base* Data_cache_manager;
	};
}

#endif // !NVM_FIRMWARE_H
