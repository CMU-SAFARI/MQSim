#ifndef IO_FLOW_TRACE_BASED_H
#define IO_FLOW_TRACE_BASED_H
#include <string>
#include "IO_Flow_Base.h"
#include "ASCII_Trace_Definition.h"

namespace Host_Components
{
	class IO_Flow_Trace_Based : public IO_Flow_Base
	{
		IO_Flow_Trace_Based(const sim_object_id_type& name, LSA_type start_lsa_on_device, LSA_type end_lsa_address_on_device, uint16_t io_queue_id,
			uint16_t nvme_submission_queue_size, uint16_t nvme_completion_queue_size, IO_Flow_Priority_Class priority_class,
			sim_time_type stop_time, unsigned int total_req_count,
			HostInterfaceType SSD_device_type, PCIe_Root_Complex* pcie_root_complex, Trace_Time_Unit time_unit = Trace_Time_Unit::NANOSECOND, unsigned int percentage_to_be_simulated = 100);
		Host_IO_Reqeust* Generate_next_request();
		void NVMe_consume_io_request(Completion_Queue_Entry*);
		void Start_simulation();
		void Validate_simulation_config();
		void Execute_simulator_event(MQSimEngine::Sim_Event*);
	private:
		Trace_Time_Unit time_unit;
		unsigned int percentage_to_be_simulated;
	};
}

#endif// !IO_FLOW_TRACE_BASED_H