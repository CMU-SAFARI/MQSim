#include <math.h>
#include <stdexcept>
#include "../sim/Engine.h"
#include "IO_Flow_Synthetic.h"

namespace Host_Components
{
IO_Flow_Synthetic::IO_Flow_Synthetic(const sim_object_id_type &name, uint16_t flow_id,
									 LHA_type start_lsa_on_device, LHA_type end_lsa_on_device, double working_set_ratio, uint16_t io_queue_id,
									 uint16_t nvme_submission_queue_size, uint16_t nvme_completion_queue_size, IO_Flow_Priority_Class::Priority priority_class,
									 double read_ratio, Utils::Address_Distribution_Type address_distribution, double hot_region_ratio,
									 Utils::Request_Size_Distribution_Type request_size_distribution, unsigned int average_request_size, unsigned int variance_request_size,
									 Utils::Request_Generator_Type generator_type, sim_time_type Average_inter_arrival_time_nano_sec, unsigned int average_number_of_enqueued_requests,
									 bool generate_aligned_addresses, unsigned int alignment_value,
									 int seed, sim_time_type stop_time, double initial_occupancy_ratio, unsigned int total_req_count, HostInterface_Types SSD_device_type, PCIe_Root_Complex *pcie_root_complex, SATA_HBA *sata_hba,
									 bool enabled_logging, sim_time_type logging_period, std::string logging_file_path) : IO_Flow_Base(name, flow_id, start_lsa_on_device, LHA_type(start_lsa_on_device + (end_lsa_on_device - start_lsa_on_device) * working_set_ratio), io_queue_id, nvme_submission_queue_size, nvme_completion_queue_size, priority_class, stop_time, initial_occupancy_ratio, total_req_count, SSD_device_type, pcie_root_complex, sata_hba, enabled_logging, logging_period, logging_file_path),
																														  read_ratio(read_ratio), address_distribution(address_distribution),
																														  working_set_ratio(working_set_ratio), hot_region_ratio(hot_region_ratio),
																														  request_size_distribution(request_size_distribution), average_request_size(average_request_size), variance_request_size(variance_request_size),
																														  generator_type(generator_type), Average_inter_arrival_time_nano_sec(Average_inter_arrival_time_nano_sec), average_number_of_enqueued_requests(average_number_of_enqueued_requests),
																														  seed(seed), generate_aligned_addresses(generate_aligned_addresses), alignment_value(alignment_value)
{
	//If read ratio is 0, then we change its value to a negative one so that in request generation we never generate a read request
	if (read_ratio == 0.0)
	{
		read_ratio = -1.0;
	}
	random_request_type_generator_seed = seed++;
	random_request_type_generator = new Utils::RandomGenerator(random_request_type_generator_seed);
	random_address_generator_seed = seed++;
	random_address_generator = new Utils::RandomGenerator(random_address_generator_seed);
	if (this->start_lsa_on_device > this->end_lsa_on_device)
	{
		throw std::logic_error("Problem in IO Flow Synthetic, the start LBA address is greater than the end LBA address");
	}

	if (address_distribution == Utils::Address_Distribution_Type::RANDOM_HOTCOLD)
	{
		random_hot_address_generator_seed = seed++;
		random_hot_address_generator = new Utils::RandomGenerator(random_hot_address_generator_seed);
		random_hot_cold_generator_seed = seed++;
		random_hot_cold_generator = new Utils::RandomGenerator(random_hot_cold_generator_seed);
		hot_region_end_lsa = this->start_lsa_on_device + (LHA_type)((double)(this->end_lsa_on_device - this->start_lsa_on_device) * hot_region_ratio);
	}

	if (request_size_distribution == Utils::Request_Size_Distribution_Type::NORMAL)
	{
		random_request_size_generator_seed = seed++;
		random_request_size_generator = new Utils::RandomGenerator(random_request_size_generator_seed);
	}

	if (generator_type == Utils::Request_Generator_Type::BANDWIDTH)
	{
		random_time_interval_generator_seed = seed++;
		random_time_interval_generator = new Utils::RandomGenerator(random_time_interval_generator_seed);
	}

	if (this->working_set_ratio == 0)
	{
		PRINT_ERROR("The working set ratio is set to zero for workload " << name)
	}
	}

	IO_Flow_Synthetic::~IO_Flow_Synthetic()
	{
		delete random_request_type_generator;
		delete random_address_generator;
		delete random_hot_cold_generator;
		delete random_hot_address_generator;
		delete random_request_size_generator;
		delete random_time_interval_generator;
	}

	Host_IO_Request* IO_Flow_Synthetic::Generate_next_request()
	{
		if (stop_time > 0) {
			if (Simulator->Time() > stop_time) {
				return NULL;
			}
		} else if (STAT_generated_request_count >= total_requests_to_be_generated) {
			return NULL;
		}
		
		Host_IO_Request* request = new Host_IO_Request;
		if (random_request_type_generator->Uniform(0, 1) <= read_ratio) {
			request->Type = Host_IO_Request_Type::READ;
			STAT_generated_read_request_count++;
		} else {
			request->Type = Host_IO_Request_Type::WRITE;
			STAT_generated_write_request_count++;
		}

		switch (request_size_distribution) {
			case Utils::Request_Size_Distribution_Type::FIXED:
				request->LBA_count = average_request_size;
				break;
			case Utils::Request_Size_Distribution_Type::NORMAL:
			{
				double temp_request_size = random_request_size_generator->Normal(average_request_size, variance_request_size);
				request->LBA_count = (unsigned int)(ceil(temp_request_size));
				if (request->LBA_count <= 0) {
					request->LBA_count = 1;
				}
				break;
			}
			default:
				throw std::invalid_argument("Uknown distribution type for requset size.");
		}

		switch (address_distribution) {
			case Utils::Address_Distribution_Type::STREAMING:
				request->Start_LBA = streaming_next_address;
				if (request->Start_LBA + request->LBA_count > end_lsa_on_device) {
					request->Start_LBA = start_lsa_on_device;
				}
				streaming_next_address += request->LBA_count;
				if (streaming_next_address > end_lsa_on_device) {
					streaming_next_address = start_lsa_on_device;
				}
				if (generate_aligned_addresses) {
					if(streaming_next_address % alignment_value != 0) {
						streaming_next_address += alignment_value - (streaming_next_address % alignment_value);
					}
				}
				if(streaming_next_address == request->Start_LBA) {
					PRINT_MESSAGE("Synthetic Message Generator: The same address is always repeated due to configuration parameters!")
				}
				break;
			case Utils::Address_Distribution_Type::RANDOM_HOTCOLD:
				// (100-hot)% of requests going to hot% of the address space
				if (random_hot_cold_generator->Uniform(0, 1) < hot_region_ratio) {
					request->Start_LBA = random_hot_address_generator->Uniform_ulong(hot_region_end_lsa + 1, end_lsa_on_device);
					if (request->Start_LBA < hot_region_end_lsa + 1 || request->Start_LBA > end_lsa_on_device) {
						PRINT_ERROR("Out of range address is generated in IO_Flow_Synthetic!\n")
						if (request->Start_LBA + request->LBA_count > end_lsa_on_device) {
							request->Start_LBA = hot_region_end_lsa + 1;
						}
					}
				} else {
					request->Start_LBA = random_hot_address_generator->Uniform_ulong(start_lsa_on_device, hot_region_end_lsa);
					if (request->Start_LBA < start_lsa_on_device || request->Start_LBA > hot_region_end_lsa) {
						PRINT_ERROR("Out of range address is generated in IO_Flow_Synthetic!\n")
					}
				}
				break;
			case Utils::Address_Distribution_Type::RANDOM_UNIFORM:
				request->Start_LBA = random_address_generator->Uniform_ulong(start_lsa_on_device, end_lsa_on_device);
				if (request->Start_LBA < start_lsa_on_device || request->Start_LBA > end_lsa_on_device) {
					PRINT_ERROR("Out of range address is generated in IO_Flow_Synthetic!\n")
				}
				if (request->Start_LBA + request->LBA_count > end_lsa_on_device) {
					request->Start_LBA = start_lsa_on_device;
				}
				break;
			default:
				PRINT_ERROR("Unknown address distribution type!\n")
		}

		if (generate_aligned_addresses) {
			request->Start_LBA -= request->Start_LBA % alignment_value;
		}
		STAT_generated_request_count++;
		request->Arrival_time = Simulator->Time();
		DEBUG("* Host: Request generated - " << (request->Type == Host_IO_Request_Type::READ ? "Read, " : "Write, ") << "LBA:" << request->Start_LBA << ", Size_in_bytes:" << request->LBA_count << "")

		return request;
	}

	void IO_Flow_Synthetic::NVMe_consume_io_request(Completion_Queue_Entry* io_request)
	{
		IO_Flow_Base::NVMe_consume_io_request(io_request);
		IO_Flow_Base::NVMe_update_and_submit_completion_queue_tail();
		if (generator_type == Utils::Request_Generator_Type::QUEUE_DEPTH) {
			Host_IO_Request* request = Generate_next_request();
			
			/* In the demand based execution mode, the Generate_next_request() function may return NULL
			* if 1) the simulation stop is met, or 2) the number of generated I/O requests reaches its threshold.*/
			if (request != NULL) {
				Submit_io_request(request);
			}
		}
	}

	void IO_Flow_Synthetic::SATA_consume_io_request(Host_IO_Request* io_request)
	{
		IO_Flow_Base::SATA_consume_io_request(io_request);
		if (generator_type == Utils::Request_Generator_Type::QUEUE_DEPTH) {
			Host_IO_Request* request = Generate_next_request();
			/* In the demand based execution mode, the Generate_next_request() function may return NULL
			* if 1) the simulation stop is met, or 2) the number of generated I/O requests reaches its threshold.*/
			if (request != NULL) {
				Submit_io_request(request);
			}
		}
	}

	void IO_Flow_Synthetic::Start_simulation() 
	{
		IO_Flow_Base::Start_simulation();

		if (address_distribution == Utils::Address_Distribution_Type::STREAMING) {
			streaming_next_address = random_address_generator->Uniform_ulong(start_lsa_on_device, end_lsa_on_device);
			if (generate_aligned_addresses) {
				streaming_next_address -= streaming_next_address % alignment_value;
			}
		}

		if (generator_type == Utils::Request_Generator_Type::BANDWIDTH) {
			Simulator->Register_sim_event((sim_time_type)random_time_interval_generator->Exponential((double)Average_inter_arrival_time_nano_sec), this, 0, 0);
		} else {
			Simulator->Register_sim_event((sim_time_type)1, this, 0, 0);
		}
	}

	void IO_Flow_Synthetic::Validate_simulation_config()
	{
	}

	void IO_Flow_Synthetic::Execute_simulator_event(MQSimEngine::Sim_Event* event)
	{
		if (generator_type == Utils::Request_Generator_Type::BANDWIDTH) {
			Host_IO_Request* req = Generate_next_request();
			if (req != NULL) {
				Submit_io_request(req);
				Simulator->Register_sim_event(Simulator->Time() + (sim_time_type)random_time_interval_generator->Exponential((double)Average_inter_arrival_time_nano_sec), this, 0, 0);
			}
		} else {
			for (unsigned int i = 0; i < average_number_of_enqueued_requests; i++) {
				Submit_io_request(Generate_next_request());
			}
		}
	}

	void IO_Flow_Synthetic::Get_statistics(Utils::Workload_Statistics& stats, LPA_type(*Convert_host_logical_address_to_device_address)(LHA_type lha),
		page_status_type(*Find_NVM_subunit_access_bitmap)(LHA_type lha))
	{
		stats.Type = Utils::Workload_Type::SYNTHETIC;
		stats.generator_type = generator_type;
		stats.Stream_id = io_queue_id - 1;
		stats.Initial_occupancy_ratio = initial_occupancy_ratio;
		stats.Working_set_ratio = working_set_ratio;
		stats.Read_ratio = read_ratio;
		stats.random_request_type_generator_seed = random_request_type_generator_seed;
		stats.Address_distribution_type = address_distribution;
		stats.Ratio_of_hot_addresses_to_whole_working_set = hot_region_ratio;
		stats.Ratio_of_traffic_accessing_hot_region = 1 - hot_region_ratio;
		stats.random_address_generator_seed = random_address_generator_seed;
		stats.random_hot_address_generator_seed = random_hot_address_generator_seed;
		stats.random_hot_cold_generator_seed = random_hot_cold_generator_seed;
		stats.generate_aligned_addresses = generate_aligned_addresses;
		stats.alignment_value = alignment_value;
		stats.Request_size_distribution_type = request_size_distribution;
		stats.Average_request_size_sector = average_request_size;
		stats.STDEV_reuqest_size = variance_request_size;
		stats.random_request_size_generator_seed = random_request_size_generator_seed;
		stats.Request_queue_depth = average_number_of_enqueued_requests;
		stats.random_time_interval_generator_seed = random_time_interval_generator_seed;
		stats.Average_inter_arrival_time_nano_sec = Average_inter_arrival_time_nano_sec;
		stats.Min_LHA = start_lsa_on_device;
		stats.Max_LHA = end_lsa_on_device;
	}
}
