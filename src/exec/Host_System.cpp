#include "../sim/Engine.h"
#include "Host_System.h"
#include "../ssd/Host_Interface_Base.h"
#include "../ssd/Host_Interface_NVMe.h"
#include "../host/PCIe_Root_Complex.h"
#include "../host/IO_Flow_Synthetic.h"
#include "../host/IO_Flow_Trace_Based.h"

Host_System::Host_System(Host_Parameter_Set* parameters, SSD_Components::Host_Interface_Base* ssd_host_interface):
	MQSimEngine::Sim_Object("Host")
{
	Simulator->AddObject(this);

	//Create the main components of the host system
	this->Link = new Host_Components::PCIe_Link(this->ID() + ".PCIeLink", NULL, NULL, parameters->PCIe_Lane_Bandwidth, parameters->PCIe_Lane_Count);
	this->PCIe_root_complex = new Host_Components::PCIe_Root_Complex(this->Link, ssd_host_interface->GetType(), NULL);
	this->Link->Set_root_complex(this->PCIe_root_complex);
	this->PCIe_switch = new Host_Components::PCIe_Switch(this->Link, ssd_host_interface);
	this->Link->Set_pcie_switch(this->PCIe_switch);
	Simulator->AddObject(this->Link);

	//Create IO flows
	LSA_type address_range_per_flow = ssd_host_interface->Get_max_logical_sector_address() / parameters->IO_Flow_Definitions.size();
	for (uint16_t flow_id = 0; flow_id < parameters->IO_Flow_Definitions.size(); flow_id++)
	{
		Host_Components::IO_Flow_Base* io_flow = NULL;
		//No flow should ask for I/O queue id 0, it is reserved for NVMe Admin command queue pair
		//Hence, we use flow_id + 1 (which is equal to 1, 2, ...) as the requested I/O queue id
		switch (parameters->IO_Flow_Definitions[flow_id]->Type)
		{
		case Flow_Type::SYNTHETIC:
		{
			IO_Flow_Parameter_Set_Synthetic* flow_param = (IO_Flow_Parameter_Set_Synthetic*)parameters->IO_Flow_Definitions[flow_id];
			io_flow = new Host_Components::IO_Flow_Synthetic(this->ID() + ".IO_Flow.Synth.No_" + std::to_string(flow_id),
				address_range_per_flow * flow_id, address_range_per_flow * (flow_id + 1) - 1, 
				FLOW_ID_TO_Q_ID(flow_id), ((SSD_Components::Host_Interface_NVMe*)ssd_host_interface)->Get_submission_queue_depth(),
				((SSD_Components::Host_Interface_NVMe*)ssd_host_interface)->Get_completion_queue_depth(),
				flow_param->Priority_Class, flow_param->Read_Percentage / double(100.0), flow_param->Address_Distribution, flow_param->Percentage_of_Hot_Region / double(100.0),
				flow_param->Request_Size_Distribution, flow_param->Average_Request_Size, flow_param->Variance_Request_Size,
				Host_Components::Request_Generator_Type::DEMAND_BASED, 0, flow_param->Average_No_of_Reqs_in_Queue,
				flow_param->Seed, flow_param->Stop_Time, flow_param->Total_Requests_To_Generate, ssd_host_interface->GetType(), this->PCIe_root_complex);
			this->IO_flows.push_back(io_flow);
			break;
		}
		case Flow_Type::TRACE:
		{
			IO_Flow_Parameter_Set_Trace_Based * flow_param = (IO_Flow_Parameter_Set_Trace_Based*)parameters->IO_Flow_Definitions[flow_id];
			io_flow = new Host_Components::IO_Flow_Trace_Based(this->ID() + ".IO_Flow.Trace." + flow_param->File_Path,
				address_range_per_flow * flow_id, address_range_per_flow * (flow_id + 1) - 1,
				FLOW_ID_TO_Q_ID(flow_id), ((SSD_Components::Host_Interface_NVMe*)ssd_host_interface)->Get_submission_queue_depth(),
				((SSD_Components::Host_Interface_NVMe*)ssd_host_interface)->Get_completion_queue_depth(),
				flow_param->Priority_Class, flow_param->File_Path, flow_param->Time_Unit, flow_param->Relay_Count, flow_param->Percentage_To_Be_Executed,
				ssd_host_interface->GetType(), this->PCIe_root_complex);

			this->IO_flows.push_back(io_flow);
			break;
		}
		default:
			throw "The specified IO flow type is not supported.\n";
		}
		Simulator->AddObject(io_flow);
	}
	this->PCIe_root_complex->Set_io_flows(&this->IO_flows);
}

void Host_System::Attach_ssd_device(SSD_Device* ssd_device)
{
	ssd_device->Attach_to_host(this->PCIe_switch);
	this->PCIe_switch->Attach_ssd_device(ssd_device->Host_interface);
	this->ssd_device = ssd_device;
}

const std::vector<Host_Components::IO_Flow_Base*> Host_System::Get_io_flows()
{
	return IO_flows;
}

void Host_System::Start_simulation()
{
	switch (ssd_device->Host_interface->GetType())
	{
	case HostInterfaceType::NVME:
		for (uint16_t flow_cntr = 0; flow_cntr < IO_flows.size(); flow_cntr++)
			((SSD_Components::Host_Interface_NVMe*) ssd_device->Host_interface)->Create_new_stream(
				IO_flows[flow_cntr]->Get_start_lsa_on_device(), IO_flows[flow_cntr]->Get_end_lsa_address_on_device(),
				IO_flows[flow_cntr]->Get_nvme_queue_pair_info()->Submission_queue_memory_base_address, IO_flows[flow_cntr]->Get_nvme_queue_pair_info()->Completion_queue_memory_base_address);
		break;
	case HostInterfaceType::SATA:
		break;
	default:
		break;
	}
}

void Host_System::Validate_simulation_config() 
{
	if (this->IO_flows.size() == 0)
		PRINT_ERROR("No IO flow is set for host system")
	if (this->PCIe_root_complex == NULL)
		PRINT_ERROR("PCIe Root Complex is not set for host system");
	if (this->Link == NULL)
		PRINT_ERROR("PCIe Link is not set for host system");
	if (this->PCIe_switch == NULL)
		PRINT_ERROR("PCIe Switch is not set for host system")
	if (!this->PCIe_switch->Is_ssd_connected())
		PRINT_ERROR("No SSD is connected to the host system")
}

void Host_System::Execute_simulator_event(MQSimEngine::Sim_Event* event) {}

void Host_System::Report_results_in_XML(std::string name_prefix, Utils::XmlWriter& xmlwriter)
{
	std::string tmp;
	tmp = ID();
	xmlwriter.Write_open_tag(tmp);

	for (auto flow : IO_flows)
		flow->Report_results_in_XML("Host", xmlwriter);

	xmlwriter.Write_close_tag();
}
