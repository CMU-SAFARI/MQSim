#ifndef SSD_DEVICE_H
#define SSD_DEVICE_H

#include <vector>
#include "../sim/Sim_Object.h"
#include "../sim/Sim_Reporter.h"
#include "../ssd/SSD_Defs.h"
#include "../ssd/Host_Interface_Base.h"
#include "../ssd/Host_Interface_SATA.h"
#include "../ssd/Host_Interface_NVMe.h"
#include "../ssd/Data_Cache_Manager_Base.h"
#include "../ssd/Data_Cache_Flash.h"
#include "../ssd/NVM_Firmware.h"
#include "../ssd/NVM_PHY_Base.h"
#include "../ssd/NVM_Channel_Base.h"
#include "../host/PCIe_Switch.h"
#include "../nvm_chip/NVM_Types.h"
#include "Device_Parameter_Set.h"
#include "IO_Flow_Parameter_Set.h"
#include "../utils/Workload_Statistics.h"

/*********************************************************************************************************
* An SSD device has the following components:
* 
* Host_Interface <---> Data_Cache_Manager <----> NVM_Firmware <---> NVM_PHY <---> NVM_Channel <---> Chips
*
*********************************************************************************************************/

class SSD_Device : public MQSimEngine::Sim_Object, public MQSimEngine::Sim_Reporter
{
public:
	SSD_Device(Device_Parameter_Set* parameters, std::vector<IO_Flow_Parameter_Set*>* io_flows);
	~SSD_Device();
	bool Preconditioning_required;
	NVM::NVM_Type Memory_Type;
	SSD_Components::Host_Interface_Base *Host_interface;
	SSD_Components::Data_Cache_Manager_Base *Cache_manager;
	SSD_Components::NVM_Firmware* Firmware;
	SSD_Components::NVM_PHY_Base* PHY;
	std::vector<SSD_Components::NVM_Channel_Base*> Channels;
	void Report_results_in_XML(std::string name_prefix, Utils::XmlWriter& xmlwriter);
	unsigned int Get_no_of_LHAs_in_an_NVM_write_unit();

	void Attach_to_host(Host_Components::PCIe_Switch* pcie_switch);
	void Perform_preconditioning(std::vector<Utils::Workload_Statistics*> workload_stats);
	void Start_simulation();
	void Validate_simulation_config();
	void Execute_simulator_event(MQSimEngine::Sim_Event* event);
	static LPA_type Convert_host_logical_address_to_device_address(LHA_type lha);
	static page_status_type Find_NVM_subunit_access_bitmap(LHA_type lha);

	unsigned int Channel_count;
	unsigned int Chip_no_per_channel;

private:
	static SSD_Device * my_instance;//Used in static functions
};

#endif //!SSD_DEVICE_H
