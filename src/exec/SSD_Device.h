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
#include "../ssd/NVM_Firmware.h"
#include "../ssd/NVM_PHY_Base.h"
#include "../ssd/NVM_Channel_Base.h"
#include "../host/PCIe_Switch.h"
#include "Device_Parameter_Set.h"
#include "IO_Flow_Parameter_Set.h"

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
	//Memory_Type Memory_Type;
	SSD_Components::Host_Interface_Base *Host_interface;
	SSD_Components::Data_Cache_Manager_Base *Cache_manager;
	SSD_Components::NVM_Firmware* Firmware;
	SSD_Components::NVM_PHY_Base* PHY;
	std::vector<SSD_Components::NVM_Channel_Base*> Channels;
	void Report_results_in_XML(Utils::XmlWriter& xmlwriter);

	void Attach_to_host(Host_Components::PCIe_Switch* pcie_switch);
	void Perform_preconditioning();
	void Start_simulation();
	void Validate_simulation_config();
	void Execute_simulator_event(MQSimEngine::Sim_Event* event);
};

#endif //!SSD_DEVICE_H