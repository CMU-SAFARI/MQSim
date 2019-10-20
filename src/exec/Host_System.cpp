#include "../sim/Engine.h"
#include "Host_System.h"
#include "../ssd/Host_Interface_Base.h"
#include "../ssd/Host_Interface_NVMe.h"
#include "../host/PCIe_Root_Complex.h"
#include "../host/IO_Flow_Synthetic.h"
#include "../host/IO_Flow_Trace_Based.h"
#include "../utils/StringTools.h"
#include "../utils/Logical_Address_Partitioning_Unit.h"

Host_System::Host_System(Host_Parameter_Set* parameters, bool preconditioning_required, SSD_Components::Host_Interface_Base* ssd_host_interface):
	MQSimEngine::Sim_Object("Host"), preconditioning_required(preconditioning_required)
{
	Simulator->AddObject(this);

	//Create the main components of the host system
	if (((SSD_Components::Host_Interface_NVMe*)ssd_host_interface)->GetType() == HostInterface_Types::SATA) {
		this->SATA_hba = new Host_Components::SATA_HBA(ID() + ".SATA_HBA", ((SSD_Components::Host_Interface_SATA*)ssd_host_interface)->Get_ncq_depth(), parameters->SATA_Processing_Delay, NULL, NULL);
	} else {
		this->SATA_hba = NULL;
	}
	this->Link = new Host_Components::PCIe_Link(this->ID() + ".PCIeLink", NULL, NULL, parameters->PCIe_Lane_Bandwidth, parameters->PCIe_Lane_Count);
	this->PCIe_root_complex = new Host_Components::PCIe_Root_Complex(this->Link, ssd_host_interface->GetType(), this->SATA_hba, NULL);
	this->Link->Set_root_complex(this->PCIe_root_complex);
	this->PCIe_switch = new Host_Components::PCIe_Switch(this->Link, ssd_host_interface);
	this->Link->Set_pcie_switch(this->PCIe_switch);
	Simulator->AddObject(this->Link);

	//Create IO flows
	LHA_type address_range_per_flow = ssd_host_interface->Get_max_logical_sector_address() / parameters->IO_Flow_Definitions.size();
	for (uint16_t flow_id = 0; flow_id < parameters->IO_Flow_Definitions.size(); flow_id++) {
		Host_Components::IO_Flow_Base* io_flow = NULL;
		//No flow should ask for I/O queue id 0, it is reserved for NVMe Admin command queue pair
		//Hence, we use flow_id + 1 (which is equal to 1, 2, ...) as the requested I/O queue id
		uint16_t nvme_sq_size = 0, nvme_cq_size = 0;
		switch (((SSD_Components::Host_Interface_NVMe*)ssd_host_interface)->GetType()) {
			case HostInterface_Types::NVME:
				nvme_sq_size = ((SSD_Components::Host_Interface_NVMe*)ssd_host_interface)->Get_submission_queue_depth();
				nvme_cq_size = ((SSD_Components::Host_Interface_NVMe*)ssd_host_interface)->Get_completion_queue_depth();
				break;
			default:
				break;
		}

		switch (parameters->IO_Flow_Definitions[flow_id]->Type) {
			case Flow_Type::SYNTHETIC: {
				IO_Flow_Parameter_Set_Synthetic* flow_param = (IO_Flow_Parameter_Set_Synthetic*)parameters->IO_Flow_Definitions[flow_id];
				if (flow_param->Working_Set_Percentage > 100 || flow_param->Working_Set_Percentage < 1) {
					flow_param->Working_Set_Percentage = 100;
				}
				io_flow = new Host_Components::IO_Flow_Synthetic(this->ID() + ".IO_Flow.Synth.No_" + std::to_string(flow_id), flow_id,
					Utils::Logical_Address_Partitioning_Unit::Start_lha_available_to_flow(flow_id),
					Utils::Logical_Address_Partitioning_Unit::End_lha_available_to_flow(flow_id),
					((double)flow_param->Working_Set_Percentage / 100.0), FLOW_ID_TO_Q_ID(flow_id), nvme_sq_size, nvme_cq_size,
					flow_param->Priority_Class, flow_param->Read_Percentage / double(100.0), flow_param->Address_Distribution, flow_param->Percentage_of_Hot_Region / double(100.0),
					flow_param->Request_Size_Distribution, flow_param->Average_Request_Size, flow_param->Variance_Request_Size,
					flow_param->Synthetic_Generator_Type, (flow_param->Bandwidth == 0? 0 :NanoSecondCoeff / ((flow_param->Bandwidth / SECTOR_SIZE_IN_BYTE) / flow_param->Average_Request_Size)),
					flow_param->Average_No_of_Reqs_in_Queue, flow_param->Generated_Aligned_Addresses, flow_param->Address_Alignment_Unit,
					flow_param->Seed, flow_param->Stop_Time, flow_param->Initial_Occupancy_Percentage / double(100.0), flow_param->Total_Requests_To_Generate, ssd_host_interface->GetType(), this->PCIe_root_complex, this->SATA_hba,
					parameters->Enable_ResponseTime_Logging, parameters->ResponseTime_Logging_Period_Length, parameters->Input_file_path + ".IO_Flow.No_" + std::to_string(flow_id) + ".log");
				this->IO_flows.push_back(io_flow);
				break;
			}
			case Flow_Type::TRACE: {
				IO_Flow_Parameter_Set_Trace_Based * flow_param = (IO_Flow_Parameter_Set_Trace_Based*)parameters->IO_Flow_Definitions[flow_id];
				io_flow = new Host_Components::IO_Flow_Trace_Based(this->ID() + ".IO_Flow.Trace." + flow_param->File_Path, flow_id,
					Utils::Logical_Address_Partitioning_Unit::Start_lha_available_to_flow(flow_id), Utils::Logical_Address_Partitioning_Unit::End_lha_available_to_flow(flow_id),
					FLOW_ID_TO_Q_ID(flow_id), nvme_sq_size, nvme_cq_size,
					flow_param->Priority_Class, flow_param->Initial_Occupancy_Percentage / double(100.0),
					flow_param->File_Path, flow_param->Time_Unit, flow_param->Relay_Count, flow_param->Percentage_To_Be_Executed,
					ssd_host_interface->GetType(), this->PCIe_root_complex, this->SATA_hba,
					parameters->Enable_ResponseTime_Logging, parameters->ResponseTime_Logging_Period_Length, parameters->Input_file_path + ".IO_Flow.No_" + std::to_string(flow_id) + ".log");

				this->IO_flows.push_back(io_flow);
				break;
			}
			default:
				throw "The specified IO flow type is not supported.\n";
		}
		Simulator->AddObject(io_flow);
	}
	this->PCIe_root_complex->Set_io_flows(&this->IO_flows);
	if (((SSD_Components::Host_Interface_NVMe*)ssd_host_interface)->GetType() == HostInterface_Types::SATA) {
		this->SATA_hba->Set_io_flows(&this->IO_flows);
		this->SATA_hba->Set_root_complex(this->PCIe_root_complex);
	}
}

Host_System::~Host_System() 
{
	delete this->Link;
	delete this->PCIe_root_complex;
	delete this->PCIe_switch;
	if (ssd_device->Host_interface->GetType() == HostInterface_Types::SATA) {
		delete this->SATA_hba;
	}
	for (uint16_t flow_id = 0; flow_id < this->IO_flows.size(); flow_id++) {
		delete this->IO_flows[flow_id];
	}
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
	switch (ssd_device->Host_interface->GetType()) {
		case HostInterface_Types::NVME:
			for (uint16_t flow_cntr = 0; flow_cntr < IO_flows.size(); flow_cntr++) {
				((SSD_Components::Host_Interface_NVMe*) ssd_device->Host_interface)->Create_new_stream(
					IO_flows[flow_cntr]->Priority_class(),
					IO_flows[flow_cntr]->Get_start_lsa_on_device(), IO_flows[flow_cntr]->Get_end_lsa_address_on_device(),
					IO_flows[flow_cntr]->Get_nvme_queue_pair_info()->Submission_queue_memory_base_address, IO_flows[flow_cntr]->Get_nvme_queue_pair_info()->Completion_queue_memory_base_address);
			}
			break;
		case HostInterface_Types::SATA:
			((SSD_Components::Host_Interface_SATA*) ssd_device->Host_interface)->Set_ncq_address(
				SATA_hba->Get_sata_ncq_info()->Submission_queue_memory_base_address, SATA_hba->Get_sata_ncq_info()->Completion_queue_memory_base_address);
		default:
			break;
	}

	if (preconditioning_required) {
		std::vector<Utils::Workload_Statistics*> workload_stats = get_workloads_statistics();
		ssd_device->Perform_preconditioning(workload_stats);
		for (auto &stat : workload_stats) {
			delete stat;
		}
	}
}

void Host_System::Validate_simulation_config() 
{
	if (this->IO_flows.size() == 0) {
		PRINT_ERROR("No IO flow is set for host system")
	}
	if (this->PCIe_root_complex == NULL) {
		PRINT_ERROR("PCIe Root Complex is not set for host system");
	}
	if (this->Link == NULL) {
		PRINT_ERROR("PCIe Link is not set for host system");
	}
	if (this->PCIe_switch == NULL) {
		PRINT_ERROR("PCIe Switch is not set for host system")
	}
	if (!this->PCIe_switch->Is_ssd_connected()) {
		PRINT_ERROR("No SSD is connected to the host system")
	}
}

void Host_System::Execute_simulator_event(MQSimEngine::Sim_Event* event)
{
}

void Host_System::Report_results_in_XML(std::string name_prefix, Utils::XmlWriter& xmlwriter)
{
	std::string tmp;
	tmp = ID();
	xmlwriter.Write_open_tag(tmp);

	for (auto &flow : IO_flows) {
		flow->Report_results_in_XML("Host", xmlwriter);
	}

	xmlwriter.Write_close_tag();
}

std::vector<Utils::Workload_Statistics*> Host_System::get_workloads_statistics()
{
	std::vector<Utils::Workload_Statistics*> stats;

	for (auto &workload : IO_flows) {
		Utils::Workload_Statistics* s = new Utils::Workload_Statistics;
		workload->Get_statistics(*s, ssd_device->Convert_host_logical_address_to_device_address, ssd_device->Find_NVM_subunit_access_bitmap);
		stats.push_back(s);
	}

	return stats;
}

