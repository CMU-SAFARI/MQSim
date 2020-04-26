#ifndef IO_FLOW_SYNTHETIC_H
#define IO_FLOW_SYNTHETIC_H

#include <string>
#include "IO_Flow_Base.h"
#include "../utils/RandomGenerator.h"
#include "../utils/DistributionTypes.h"

namespace Host_Components
{
class IO_Flow_Synthetic : public IO_Flow_Base
{
public:
	IO_Flow_Synthetic(const sim_object_id_type &name, uint16_t flow_id, LHA_type start_lsa_on_device, LHA_type end_lsa_on_device, double working_set_ratio, uint16_t io_queue_id,
					  uint16_t nvme_submission_queue_size, uint16_t nvme_completion_queue_size, IO_Flow_Priority_Class::Priority priority_class,
					  double read_ratio, Utils::Address_Distribution_Type address_distribution, double hot_address_ratio,
					  Utils::Request_Size_Distribution_Type request_size_distribution, unsigned int average_request_size, unsigned int variance_request_size,
					  Utils::Request_Generator_Type generator_type, sim_time_type Average_inter_arrival_time_nano_sec, unsigned int average_number_of_enqueued_requests,
					  bool generate_aligned_addresses, unsigned int alignment_value,
					  int seed, sim_time_type stop_time, double initial_occupancy_ratio, unsigned int total_req_count, HostInterface_Types SSD_device_type, PCIe_Root_Complex *pcie_root_complex, SATA_HBA *sata_hba,
					  bool enabled_logging, sim_time_type logging_period, std::string logging_file_path);
	~IO_Flow_Synthetic();
	Host_IO_Request *Generate_next_request();
	void NVMe_consume_io_request(Completion_Queue_Entry *);
	void SATA_consume_io_request(Host_IO_Request *);
	void Start_simulation();
	void Validate_simulation_config();
	void Execute_simulator_event(MQSimEngine::Sim_Event *);
	void Get_statistics(Utils::Workload_Statistics &stats, LPA_type (*Convert_host_logical_address_to_device_address)(LHA_type lha),
						page_status_type (*Find_NVM_subunit_access_bitmap)(LHA_type lha));

private:
	double read_ratio;
	double working_set_ratio;
	Utils::RandomGenerator *random_request_type_generator;
	int random_request_type_generator_seed;
	Utils::Address_Distribution_Type address_distribution;
	double hot_region_ratio;
	Utils::RandomGenerator *random_address_generator;
	int random_address_generator_seed;
	Utils::RandomGenerator *random_hot_cold_generator;
	int random_hot_cold_generator_seed;
	Utils::RandomGenerator *random_hot_address_generator;
	int random_hot_address_generator_seed;
	LHA_type hot_region_end_lsa;
	LHA_type streaming_next_address;
	Utils::Request_Size_Distribution_Type request_size_distribution;
	unsigned int average_request_size;
	unsigned int variance_request_size;
	Utils::RandomGenerator *random_request_size_generator;
	int random_request_size_generator_seed;
	Utils::Request_Generator_Type generator_type;
	Utils::RandomGenerator *random_time_interval_generator;
	int random_time_interval_generator_seed;
	sim_time_type Average_inter_arrival_time_nano_sec;
	unsigned int average_number_of_enqueued_requests;
	bool generate_aligned_addresses;
	unsigned int alignment_value;
	int seed;
};
} // namespace Host_Components

#endif // !IO_FLOW_SYNTHETIC_H
