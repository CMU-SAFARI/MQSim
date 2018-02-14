#ifndef HOST_SYSTEM_H
#define HOST_SYSTEM_H

#include <vector>
#include "../sim/Sim_Object.h"
#include "../host/PCIe_Root_Complex.h"
#include "../host/PCIe_Link.h"
#include "../host/PCIe_Switch.h"
#include "../host/PCIe_Message.h"
#include "../host/IO_Flow_Base.h"
#include "../host/Host_IO_Request.h"
#include "../ssd/Host_Interface_Base.h"
#include "Host_Parameter_Set.h"
#include "SSD_Device.h"

class Host_System : MQSimEngine::Sim_Object
{
public:
	Host_System(Host_Parameter_Set* parameters, SSD_Components::Host_Interface_Base* ssd_host_interface);
	void Start_simulation();
	void Validate_simulation_config();
	void Execute_simulator_event(MQSimEngine::Sim_Event* event);

	void Attach_ssd_device(SSD_Device* ssd_device);
	const std::vector<Host_Components::IO_Flow_Base*> Get_io_flows();
private:
	Host_Components::PCIe_Root_Complex* PCIe_root_complex;
	Host_Components::PCIe_Link* Link;
	Host_Components::PCIe_Switch* PCIe_switch;
	std::vector<Host_Components::IO_Flow_Base*> IO_flows;
	SSD_Device* ssd_device;
};

#endif // !HOST_SYSTEM_H
