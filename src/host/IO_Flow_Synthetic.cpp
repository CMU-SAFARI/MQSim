#include <math.h>
#include <stdexcept>
#include "../sim/Engine.h"
#include "IO_Flow_Synthetic.h"

namespace Host_Components
{

	IO_Flow_Synthetic::IO_Flow_Synthetic(const sim_object_id_type& name, 
		LSA_type start_lsa_on_device, LSA_type end_lsa_on_device, double working_set_ratio,
		uint16_t io_queue_id,
		uint16_t nvme_submission_queue_size, uint16_t nvme_completion_queue_size, IO_Flow_Priority_Class priority_class,
		double read_ratio, Preconditioning::Address_Distribution_Type address_distribution,
		double hot_address_ratio,
		Preconditioning::Request_Size_Distribution_Type request_size_distribution, unsigned int average_request_size, unsigned int variance_request_size,
		Request_Generator_Type generator_type, sim_time_type average_inter_arrival_time, unsigned int average_number_of_enqueued_requests,
		int seed, sim_time_type stop_time, unsigned int total_req_count, HostInterfaceType SSD_device_type, PCIe_Root_Complex* pcie_root_complex) :
		IO_Flow_Base(name, start_lsa_on_device * working_set_ratio, end_lsa_on_device * working_set_ratio, io_queue_id, nvme_submission_queue_size, nvme_completion_queue_size, priority_class, stop_time, total_req_count, SSD_device_type, pcie_root_complex), read_ratio(read_ratio), address_distribution(address_distribution),
		working_set_ratio(working_set_ratio), hot_address_ratio(hot_address_ratio),
		request_size_distribution(request_size_distribution), average_request_size(average_request_size), variance_request_size(variance_request_size),
		generator_type(generator_type), average_inter_arrival_time(average_inter_arrival_time), average_number_of_enqueued_requests(average_number_of_enqueued_requests),
		seed(seed)
	{
		if (read_ratio == 0.0)//If read ratio is 0, then we change its value to a negative one so that in request generation we never generate a read request
			read_ratio = -1.0;
		random_request_type_generator = new Utils::RandomGenerator(seed++);
		random_address_generator = new Utils::RandomGenerator(seed++);
		if (start_lsa_on_device > end_lsa_on_device)
			throw std::logic_error("Problem in IO Flow Synthetic, the start LBA address is greater than the end LBA address");
		if (address_distribution == Preconditioning::Address_Distribution_Type::HOTCOLD_RANDOM)
		{
			random_hot_address_generator = new Utils::RandomGenerator(seed++);
			random_hot_cold_generator = new Utils::RandomGenerator(seed++);
			hot_region_end_address = start_lsa_on_device + (LSA_type)((double)(end_lsa_on_device - start_lsa_on_device) * hot_address_ratio);
		}
		if (request_size_distribution == Preconditioning::Request_Size_Distribution_Type::NORMAL)
			random_request_size_generator = new Utils::RandomGenerator(seed++);
		if (generator_type == Request_Generator_Type::TIMED)
			random_time_interval_generator = new Utils::RandomGenerator(seed++);
	}

	Host_IO_Reqeust* IO_Flow_Synthetic::Generate_next_request()
	{
		if (stop_time > 0)
		{
			if (Simulator->Time() > stop_time)
				return NULL;
		}
		else if (STAT_generated_request_count >= total_requests_to_be_generated)
			return NULL;
		
		Host_IO_Reqeust* request = new Host_IO_Reqeust;
		if (random_request_type_generator->Uniform(0, 1) <= read_ratio)
		{
			request->Type = Host_IO_Request_Type::READ;
			STAT_generated_read_request_count++;
		}
		else
		{
			request->Type = Host_IO_Request_Type::WRITE;
			STAT_generated_write_request_count++;
		}

		switch (request_size_distribution)
		{
		case Preconditioning::Request_Size_Distribution_Type::FIXED:
			request->LBA_count = average_request_size;
			break;
		case Preconditioning::Request_Size_Distribution_Type::NORMAL:
		{
			double temp_request_size = random_request_size_generator->Normal(average_request_size, variance_request_size);
			request->LBA_count = (unsigned int)(ceil(temp_request_size));
			if (request->LBA_count < 0)
				request->LBA_count = 1;
			break;
		}
		default:
			throw std::invalid_argument("Uknown distribution type for requset size.");
		}

		switch (address_distribution)
		{
		case Preconditioning::Address_Distribution_Type::STREAMING:
			request->Start_LBA = streaming_next_address;
			if (request->Start_LBA + request->LBA_count > end_lsa_on_device)
				request->Start_LBA = start_lsa_on_device;
			streaming_next_address += request->LBA_count;
			if (streaming_next_address > end_lsa_on_device)
				streaming_next_address = start_lsa_on_device;
			break;
		case Preconditioning::Address_Distribution_Type::HOTCOLD_RANDOM:
			if (random_hot_cold_generator->Uniform(0, 1) < hot_address_ratio)
			{
				request->Start_LBA = start_lsa_on_device + random_hot_address_generator->Uniform_ulong(start_lsa_on_device, hot_region_end_address);
				if (request->Start_LBA < start_lsa_on_device || request->Start_LBA > hot_region_end_address)
					PRINT_ERROR("Out of range address is generated in IO_Flow_Synthetic!\n")
			}
			else
			{
				request->Start_LBA = random_hot_address_generator->Uniform_ulong(hot_region_end_address + 1, end_lsa_on_device);
				if (request->Start_LBA < hot_region_end_address + 1 || request->Start_LBA > end_lsa_on_device)
					PRINT_ERROR("Out of range address is generated in IO_Flow_Synthetic!\n")
				if (request->Start_LBA + request->LBA_count > end_lsa_on_device)
					request->Start_LBA = hot_region_end_address + 1;
			}
			break;
		case Preconditioning::Address_Distribution_Type::UNIFORM_RANDOM:
			request->Start_LBA = random_address_generator->Uniform_ulong(start_lsa_on_device, end_lsa_on_device);
			if (request->Start_LBA < start_lsa_on_device || request->Start_LBA > end_lsa_on_device)
				PRINT_ERROR("Out of range address is generated in IO_Flow_Synthetic!\n")
			if (request->Start_LBA + request->LBA_count > end_lsa_on_device)
				request->Start_LBA = start_lsa_on_device;
			break;
		default:
			PRINT_ERROR("Unknown distribution type for address.\n")
		}
		STAT_generated_request_count++;
		request->Arrival_time = Simulator->Time();
		DEBUG("* Host: Request generated - " << (request->Type == Host_IO_Request_Type::READ ? "Read, " : "Write, ") << "LBA:" << request->Start_LBA << ", Size:" << request->LBA_count << "")

		return request;
	}

	void IO_Flow_Synthetic::NVMe_consume_io_request(Completion_Queue_Entry* io_request)
	{
		IO_Flow_Base::NVMe_consume_io_request(io_request);
		IO_Flow_Base::NVMe_update_and_submit_completion_queue_tail();
		if (generator_type == Request_Generator_Type::DEMAND_BASED)
		{
			Host_IO_Reqeust* request = Generate_next_request();
			/* In the demand based execution mode, the Generate_next_request() function may return NULL
			* if 1) the simulation stop is met, or 2) the number of generated I/O requests reaches its threshold.*/
			if (request != NULL)
				submit_io_request(request);
		}
	}

	void IO_Flow_Synthetic::Start_simulation() 
	{
		if (address_distribution == Preconditioning::Address_Distribution_Type::STREAMING)
			streaming_next_address = random_address_generator->Uniform_ulong(start_lsa_on_device, end_lsa_on_device);
		if (generator_type == Request_Generator_Type::TIMED)
			Simulator->Register_sim_event((sim_time_type)random_time_interval_generator->Exponential((double)average_inter_arrival_time), this, 0, 0);
		else
			Simulator->Register_sim_event((sim_time_type)1, this, 0, 0);
	}

	void IO_Flow_Synthetic::Validate_simulation_config() {}

	void IO_Flow_Synthetic::Execute_simulator_event(MQSimEngine::Sim_Event* event)
	{
		if (generator_type == Request_Generator_Type::TIMED)
		{
			submit_io_request(Generate_next_request());
			Simulator->Register_sim_event(Simulator->Time() + (sim_time_type)random_time_interval_generator->Exponential((double)average_inter_arrival_time), this, 0, 0);
		}
		else for (unsigned int i = 0; i < average_number_of_enqueued_requests; i++)
			submit_io_request(Generate_next_request());
	}

	void IO_Flow_Synthetic::Get_statistics(Preconditioning::Workload_Statistics& stats)
	{
		stats.Type = Preconditioning::Workload_Type::SYNTHETIC;
		stats.Stream_id = io_queue_id;
		stats.Occupancy = working_set_ratio;
		stats.Read_ratio = read_ratio;
		stats.Request_queue_depth = average_number_of_enqueued_requests;
		stats.Address_distribution_type = address_distribution;
		stats.Hot_region_ratio = hot_address_ratio;
		stats.Request_size_distribution_type = request_size_distribution;
		stats.Average_request_size = average_request_size;
		stats.STDEV_reuqest_size = variance_request_size;
	}
}