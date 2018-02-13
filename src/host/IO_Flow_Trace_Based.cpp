#include "IO_Flow_Trace_Based.h"

namespace Host_Components
{
	IO_Flow_Trace_Based::IO_Flow_Trace_Based(const sim_object_id_type& name, LSA_type start_lsa_on_device, LSA_type end_lsa_address_on_device, uint16_t io_queue_id,
		uint16_t nvme_submission_queue_size, uint16_t nvme_completion_queue_size, IO_Flow_Priority_Class priority_class,
		sim_time_type stop_time, unsigned int total_req_count,
		HostInterfaceType SSD_device_type, PCIe_Root_Complex* pcie_root_complex,
		Trace_Time_Unit time_unit, unsigned int percentage_to_be_simulated) :
		IO_Flow_Base(name, start_lsa_on_device, end_lsa_on_device, io_queue_id, nvme_submission_queue_size, nvme_completion_queue_size, priority_class, stop_time, total_req_count, SSD_device_type, pcie_root_complex),
		time_unit(time_unit), percentage_to_be_simulated(percentage_to_be_simulated)
	{}

	Host_IO_Reqeust* IO_Flow_Trace_Based::Generate_next_request()
	{
		Host_IO_Reqeust* request = NULL;
		return request;
	}

	void IO_Flow_Trace_Based::NVMe_consume_io_request(Completion_Queue_Entry*)
	{}

	void IO_Flow_Trace_Based::Start_simulation()
	{}

	void IO_Flow_Trace_Based::Validate_simulation_config()
	{}

	void IO_Flow_Trace_Based::Execute_simulator_event(MQSimEngine::Sim_Event*)
	{}
}