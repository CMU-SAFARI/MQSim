#ifndef IO_FLOW_SYNTHETIC_H
#define IO_FLOW_SYNTHETIC_H

#include "IO_Flow_Base.h"
#include "../utils/RandomGenerator.h"
#include "../utils/DistributionTypes.h"

namespace Host_Components
{
	enum class Request_Generator_Type {TIMED, DEMAND_BASED};//Time_INTERVAL: general requests based on the arrival rate definitions, DEMAND_BASED: just generate a request, every time that there is a demand
	
	class IO_Flow_Synthetic : public IO_Flow_Base
	{
	public:
		IO_Flow_Synthetic(const sim_object_id_type& name, LSA_type start_lsa_on_device, LSA_type end_lsa_on_device, double working_set_ratio, uint16_t io_queue_id,
			uint16_t nvme_submission_queue_size, uint16_t nvme_completion_queue_size, IO_Flow_Priority_Class priority_class,
			double read_ratio, Utils::Address_Distribution_Type address_distribution, double hot_address_ratio,
			Utils::Request_Size_Distribution_Type request_size_distribution, unsigned int average_request_size, unsigned int variance_request_size,
			Request_Generator_Type generator_type, sim_time_type average_inter_arrival_time, unsigned int average_number_of_enqueued_requests,
			int seed, sim_time_type stop_time, unsigned int total_req_count, HostInterfaceType SSD_device_type, PCIe_Root_Complex* pcie_root_complex);
		~IO_Flow_Synthetic();
		Host_IO_Reqeust* Generate_next_request();
		void NVMe_consume_io_request(Completion_Queue_Entry*);
		void Start_simulation();
		void Validate_simulation_config();
		void Execute_simulator_event(MQSimEngine::Sim_Event*);
		void Get_statistics(Preconditioning::Workload_Statistics& stats);
	private:
		double read_ratio;
		double working_set_ratio;
		Utils::RandomGenerator* random_request_type_generator;
		int random_request_type_generator_seed;
		Utils::Address_Distribution_Type address_distribution;
		double hot_region_ratio;
		Utils::RandomGenerator* random_address_generator;
		int random_address_generator_seed;
		Utils::RandomGenerator* random_hot_cold_generator;
		int random_hot_cold_generator_seed;
		Utils::RandomGenerator* random_hot_address_generator;
		int random_hot_address_generator_seed;
		LSA_type hot_region_end_lsa;
		LSA_type streaming_next_address;
		Utils::Request_Size_Distribution_Type request_size_distribution;
		unsigned int average_request_size;
		unsigned int variance_request_size;
		Utils::RandomGenerator* random_request_size_generator;
		int random_request_size_generator_seed;
		Request_Generator_Type generator_type;
		Utils::RandomGenerator* random_time_interval_generator;
		int random_time_interval_generator_seed;
		sim_time_type average_inter_arrival_time;
		unsigned int average_number_of_enqueued_requests;
		int seed;
	};
}

#endif // !IO_FLOW_SYNTHETIC_H
