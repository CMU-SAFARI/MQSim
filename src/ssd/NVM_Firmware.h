#ifndef NVM_FIRMWARE_H
#define NVM_FIRMWARE_H

#include <vector>
#include "../sim/Sim_Object.h"
#include "../utils/Workload_Statistics.h"
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
		virtual LPA_type Convert_host_logical_address_to_device_address(LHA_type lha) = 0;
		virtual page_status_type Find_NVM_subunit_access_bitmap(LHA_type lha) = 0;
		virtual void Perform_precondition(std::vector<Utils::Workload_Statistics*> workload_stats) = 0;
		virtual void Report_results_in_XML(std::string name_prefix, Utils::XmlWriter& xmlwriter) = 0;
	};
}

#endif // !NVM_FIRMWARE_H
