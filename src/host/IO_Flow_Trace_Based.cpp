#include "IO_Flow_Trace_Based.h"
#include "../utils/StringTools.h"
#include "ASCII_Trace_Definition.h"
#include "../utils/DistributionTypes.h"

namespace Host_Components
{
	IO_Flow_Trace_Based::IO_Flow_Trace_Based(const sim_object_id_type& name, LSA_type start_lsa_on_device, LSA_type end_lsa_on_device, uint16_t io_queue_id,
		uint16_t nvme_submission_queue_size, uint16_t nvme_completion_queue_size, IO_Flow_Priority_Class priority_class,
		std::string trace_file_path, Trace_Time_Unit time_unit, unsigned int total_replay_count, unsigned int percentage_to_be_simulated,
		HostInterfaceType SSD_device_type, PCIe_Root_Complex* pcie_root_complex) :
		IO_Flow_Base(name, start_lsa_on_device, end_lsa_on_device, io_queue_id, nvme_submission_queue_size, nvme_completion_queue_size, priority_class, 0, 0, SSD_device_type, pcie_root_complex),
		trace_file_path(trace_file_path), time_unit(time_unit), total_replay_no(total_replay_count), percentage_to_be_simulated(percentage_to_be_simulated),
		total_requests_in_file(0), time_offset(0)
	{
		if (percentage_to_be_simulated > 100)
		{
			percentage_to_be_simulated = 100;
			PRINT_MESSAGE("Bad value for percentage of trace file! It is set to 100 % ");
		}
	}

	Host_IO_Reqeust* IO_Flow_Trace_Based::Generate_next_request()
	{
		if (current_trace_line.size() == 0 || STAT_generated_request_count >= total_requests_to_be_generated)
			return NULL;

		Host_IO_Reqeust* request = new Host_IO_Reqeust;
		if (current_trace_line[ASCIITraceTypeColumn].compare(ASCIITraceWriteCode) == 0)
		{
			request->Type = Host_IO_Request_Type::WRITE;
			STAT_generated_write_request_count++;
		}
		else
		{
			request->Type = Host_IO_Request_Type::READ;
			STAT_generated_read_request_count++;
		}

		char* pEnd;
		request->LBA_count = std::strtoul(current_trace_line[ASCIITraceSizeColumn].c_str(), &pEnd, 0);

		request->Start_LBA = std::strtoull(current_trace_line[ASCIITraceAddressColumn].c_str(), &pEnd, 0);
		if (request->Start_LBA <= (end_lsa_on_device - start_lsa_on_device))
			request->Start_LBA += start_lsa_on_device;
		else
			request->Start_LBA = start_lsa_on_device + request->Start_LBA % (end_lsa_on_device - start_lsa_on_device);

		request->Arrival_time = time_offset + Simulator->Time();
		STAT_generated_request_count++;
		return request;
	}

	void IO_Flow_Trace_Based::NVMe_consume_io_request(Completion_Queue_Entry* io_request)
	{
		IO_Flow_Base::NVMe_consume_io_request(io_request);
		IO_Flow_Base::NVMe_update_and_submit_completion_queue_tail();
	}

	void IO_Flow_Trace_Based::Start_simulation()
	{
		std::string trace_line;
		char* pEnd;
		if (!input_file_processed)
		{
			trace_file.open(trace_file_path, std::ios::in);
			if (!trace_file.is_open())
				PRINT_ERROR("Error while opening input trace file!")

				PRINT_MESSAGE("Investigating input trace file: " << trace_file_path)
			sim_time_type last_request_arrival_time = 0;
			while (std::getline(trace_file, trace_line))
			{
				Utils::Helper_Functions::Remove_cr(trace_line);
				current_trace_line.clear();
				Utils::Helper_Functions::Tokenize(trace_line, ASCIILineDelimiter, current_trace_line);
				if (current_trace_line.size() != ASCIIItemsPerLine)
					break;
				total_requests_in_file++;
				sim_time_type prev_time = last_request_arrival_time;
				last_request_arrival_time = std::strtoll(current_trace_line[ASCIITraceTimeColumn].c_str(), &pEnd, 10);
				if (last_request_arrival_time < prev_time)
					PRINT_ERROR("Unexpected request arrival time: " << last_request_arrival_time << "\nMQSim expects request arrival times to be monotonic increasing in the input trace!")
			}
			trace_file.close();
			PRINT_MESSAGE("Trace file: " << trace_file_path << " seems healthy")

				if (total_replay_no == 1)
					total_requests_to_be_generated = (int)(((double)percentage_to_be_simulated / 100) * total_requests_in_file);
				else
					total_requests_to_be_generated = total_requests_in_file * total_replay_no;
		}

		trace_file.open(trace_file_path);
		current_trace_line.clear();
		std::getline(trace_file, trace_line);
		Utils::Helper_Functions::Remove_cr(trace_line);
		Utils::Helper_Functions::Tokenize(trace_line, ASCIILineDelimiter, current_trace_line);
		Simulator->Register_sim_event(std::strtoll(current_trace_line[ASCIITraceTimeColumn].c_str(), &pEnd, 10), this);
	}

	void IO_Flow_Trace_Based::Validate_simulation_config()
	{
	}

	void IO_Flow_Trace_Based::Execute_simulator_event(MQSimEngine::Sim_Event*)
	{
		Host_IO_Reqeust* request = Generate_next_request();
		if (request != NULL)
			submit_io_request(request);

		if (STAT_generated_request_count < total_requests_to_be_generated)
		{
			std::string trace_line;
			if (std::getline(trace_file, trace_line))
			{
				Utils::Helper_Functions::Remove_cr(trace_line);
				current_trace_line.clear();
				Utils::Helper_Functions::Tokenize(trace_line, ASCIILineDelimiter, current_trace_line);
			}
			else
			{
				trace_file.close();
				trace_file.open(trace_file_path);
				replay_counter++;
				time_offset = Simulator->Time();
				std::getline(trace_file, trace_line);
				Utils::Helper_Functions::Remove_cr(trace_line);
				current_trace_line.clear();
				Utils::Helper_Functions::Tokenize(trace_line, ASCIILineDelimiter, current_trace_line);
				PRINT_MESSAGE("* Replay ound "<< replay_counter << "of "<< total_replay_no << " started  for" << ID())
			}
			char* pEnd;
			Simulator->Register_sim_event(time_offset + std::strtoll(current_trace_line[ASCIITraceTimeColumn].c_str(), &pEnd, 10), this);
		}
	}

	void IO_Flow_Trace_Based::Get_statistics(Preconditioning::Workload_Statistics& stats)
	{
		stats.Type = Utils::Workload_Type::TRACE_BASED;
		stats.Stream_id = io_queue_id;
		for (int i = 0; i < MAX_ARRIVAL_TIME_HISTOGRAM + 1; i++)
		{
			stats.Write_arrival_time.push_back(0);
			stats.Read_arrival_time.push_back(0);
		}
		for (int i = 0; i < MAX_SIZE_HISTOGRAM + 1; i++)
		{
			stats.Write_size_histogram.push_back(0);
			stats.Read_size_histogram.push_back(0);
		}
		stats.Total_generated_reqeusts = 0;
		stats.Total_accessed_lbas = 0;

		trace_file.open(trace_file_path, std::ios::in);
		if (!trace_file.is_open())
			PRINT_ERROR("Error while opening input trace file!")

		PRINT_MESSAGE("Investigating input trace file: " << trace_file_path);
		
		std::string trace_line;
		char* pEnd;
		sim_time_type last_request_arrival_time = 0;
		while (std::getline(trace_file, trace_line))
		{
			Utils::Helper_Functions::Remove_cr(trace_line);
			current_trace_line.clear();
			Utils::Helper_Functions::Tokenize(trace_line, ASCIILineDelimiter, current_trace_line);
			if (current_trace_line.size() != ASCIIItemsPerLine)
				break;
			total_requests_in_file++;
			sim_time_type prev_time = last_request_arrival_time;
			last_request_arrival_time = std::strtoull(current_trace_line[ASCIITraceTimeColumn].c_str(), &pEnd, 10);
			if (last_request_arrival_time < prev_time)
				PRINT_ERROR("Unexpected request arrival time: " << last_request_arrival_time << "\nMQSim expects request arrival times to be monotonic increasing in the input trace!")
			sim_time_type diff = (last_request_arrival_time - prev_time) / 1000;


			unsigned int LBA_count = std::strtoul(current_trace_line[ASCIITraceSizeColumn].c_str(), &pEnd, 0);
			LSA_type start_LBA = std::strtoull(current_trace_line[ASCIITraceAddressColumn].c_str(), &pEnd, 0);
			if (start_LBA <= (end_lsa_on_device - start_lsa_on_device))
				start_LBA += start_lsa_on_device;
			else
				start_LBA = start_lsa_on_device + start_LBA % (end_lsa_on_device - start_lsa_on_device);
			LSA_type end_LBA = start_LBA + LBA_count - 1;
			if (end_LBA > end_lsa_on_device)
				end_LBA = start_lsa_on_device + (end_LBA - end_lsa_on_device) - 1;

			//Address access pattern statistics
			while (start_LBA <= end_LBA)
			{
				if (current_trace_line[ASCIITraceTypeColumn].compare(ASCIITraceWriteCode) == 0)
				{
					if (stats.Write_address_access_pattern.find(start_LBA) == stats.Write_address_access_pattern.end())
						stats.Write_address_access_pattern[start_LBA] = (unsigned int)1;
					else
						stats.Write_address_access_pattern[start_LBA] =	stats.Write_address_access_pattern[start_LBA] + 1;
				}
				else
				{
					if (stats.Read_address_access_pattern.find(start_LBA) == stats.Read_address_access_pattern.end())
						stats.Read_address_access_pattern[start_LBA] = (unsigned int)1;
					else
						stats.Read_address_access_pattern[start_LBA] = stats.Read_address_access_pattern[start_LBA] + 1;

				}
				stats.Write_read_shared_addresses.insert(start_LBA);
				stats.Total_accessed_lbas++;
				start_LBA++;
				if (start_LBA > end_lsa_on_device)
					start_LBA = start_lsa_on_device;
			}

			//Request size statistics
			if (current_trace_line[ASCIITraceTypeColumn].compare(ASCIITraceWriteCode) == 0)
			{
				if (diff < MAX_ARRIVAL_TIME_HISTOGRAM)
					stats.Write_arrival_time[diff]++;
				else
					stats.Write_arrival_time[MAX_ARRIVAL_TIME_HISTOGRAM]++;
				if (LBA_count < MAX_SIZE_HISTOGRAM)
					stats.Write_size_histogram[LBA_count]++;
				else stats.Write_size_histogram[MAX_SIZE_HISTOGRAM]++;
			}
			else
			{
				if (diff < MAX_ARRIVAL_TIME_HISTOGRAM)
					stats.Read_arrival_time[diff]++;
				else
					stats.Read_arrival_time[MAX_ARRIVAL_TIME_HISTOGRAM]++;
				if (LBA_count < MAX_SIZE_HISTOGRAM)
					stats.Read_size_histogram[LBA_count]++;
				else stats.Read_size_histogram[(unsigned int)MAX_SIZE_HISTOGRAM]++;
			}
			stats.Total_generated_reqeusts++;
		}
		trace_file.close();
		PRINT_MESSAGE("Trace file: " << trace_file_path << " seems healthy");

		stats.Occupancy = stats.Write_read_shared_addresses.size() / (double)stats.Total_accessed_lbas;
		if (stats.Occupancy > MAX_OCCUPANCY)
			stats.Occupancy = MAX_OCCUPANCY;

		stats.Replay_no = total_replay_no;

		if (total_replay_no == 1)
			total_requests_to_be_generated = (int)(((double)percentage_to_be_simulated / 100) * total_requests_in_file);
		else
			total_requests_to_be_generated = total_requests_in_file * total_replay_no;


		input_file_processed = true;
	}
}