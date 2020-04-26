#include "IO_Flow_Base.h"
#include "../ssd/Host_Interface_Defs.h"
#include "../sim/Engine.h"

namespace Host_Components
{
	//unsigned int InputStreamBase::lastId = 0;
IO_Flow_Base::IO_Flow_Base(const sim_object_id_type &name, uint16_t flow_id, LHA_type start_lsa_on_device, LHA_type end_lsa_on_device, uint16_t io_queue_id,
						   uint16_t nvme_submission_queue_size, uint16_t nvme_completion_queue_size,
						   IO_Flow_Priority_Class::Priority priority_class, sim_time_type stop_time, double initial_occupancy_ratio, unsigned int total_requets_to_be_generated,
						   HostInterface_Types SSD_device_type, PCIe_Root_Complex *pcie_root_complex, SATA_HBA *sata_hba,
						   bool enabled_logging, sim_time_type logging_period, std::string logging_file_path) : MQSimEngine::Sim_Object(name), flow_id(flow_id), start_lsa_on_device(start_lsa_on_device), end_lsa_on_device(end_lsa_on_device), io_queue_id(io_queue_id),
																												priority_class(priority_class), stop_time(stop_time), initial_occupancy_ratio(initial_occupancy_ratio), total_requests_to_be_generated(total_requets_to_be_generated), SSD_device_type(SSD_device_type), pcie_root_complex(pcie_root_complex), sata_hba(sata_hba),
																												STAT_generated_request_count(0), STAT_generated_read_request_count(0), STAT_generated_write_request_count(0),
																												STAT_ignored_request_count(0),
																												STAT_serviced_request_count(0), STAT_serviced_read_request_count(0), STAT_serviced_write_request_count(0),
																												STAT_sum_device_response_time(0), STAT_sum_device_response_time_read(0), STAT_sum_device_response_time_write(0),
																												STAT_min_device_response_time(MAXIMUM_TIME), STAT_min_device_response_time_read(MAXIMUM_TIME), STAT_min_device_response_time_write(MAXIMUM_TIME),
																												STAT_max_device_response_time(0), STAT_max_device_response_time_read(0), STAT_max_device_response_time_write(0),
																												STAT_sum_request_delay(0), STAT_sum_request_delay_read(0), STAT_sum_request_delay_write(0),
																												STAT_min_request_delay(MAXIMUM_TIME), STAT_min_request_delay_read(MAXIMUM_TIME), STAT_min_request_delay_write(MAXIMUM_TIME),
																												STAT_max_request_delay(0), STAT_max_request_delay_read(0), STAT_max_request_delay_write(0),
																												STAT_transferred_bytes_total(0), STAT_transferred_bytes_read(0), STAT_transferred_bytes_write(0), progress(0), next_progress_step(0),
																												enabled_logging(enabled_logging), logging_period(logging_period), logging_file_path(logging_file_path)
{
	Host_IO_Request *t = NULL;

	switch (SSD_device_type)
	{
	case HostInterface_Types::NVME:
		for (uint16_t cmdid = 0; cmdid < (uint16_t)(0xffffffff); cmdid++)
		{
			available_command_ids.insert(cmdid);
		}
		for (uint16_t cmdid = 0; cmdid < nvme_submission_queue_size; cmdid++)
		{
			request_queue_in_memory.push_back(t);
		}
		nvme_queue_pair.Submission_queue_size = nvme_submission_queue_size;
		nvme_queue_pair.Submission_queue_head = 0;
		nvme_queue_pair.Submission_queue_tail = 0;
		nvme_queue_pair.Completion_queue_size = nvme_completion_queue_size;
		nvme_queue_pair.Completion_queue_head = 0;
		nvme_queue_pair.Completion_queue_tail = 0;

		//id = 0: admin queues, id = 1 to 8, normal I/O queues
		switch (io_queue_id)
		{
		case 0:
			throw std::logic_error("I/O queue id 0 is reserved for NVMe admin queues and should not be used for I/O flows");
			/*
						nvme_queue_pair.Submission_queue_memory_base_address = SUBMISSION_QUEUE_MEMORY_0;
						nvme_queue_pair.Submission_tail_register_address_on_device = SUBMISSION_QUEUE_REGISTER_0;
						nvme_queue_pair.Completion_queue_tail = COMPLETION_QUEUE_REGISTER_0;*/
			nvme_queue_pair.Completion_queue_memory_base_address = COMPLETION_QUEUE_REGISTER_0;
			break;
		case 1:
			nvme_queue_pair.Submission_queue_memory_base_address = SUBMISSION_QUEUE_MEMORY_1;
			nvme_queue_pair.Submission_tail_register_address_on_device = SUBMISSION_QUEUE_REGISTER_1;
			nvme_queue_pair.Completion_queue_memory_base_address = COMPLETION_QUEUE_MEMORY_1;
			nvme_queue_pair.Completion_head_register_address_on_device = COMPLETION_QUEUE_REGISTER_1;
			break;
		case 2:
			nvme_queue_pair.Submission_queue_memory_base_address = SUBMISSION_QUEUE_MEMORY_2;
			nvme_queue_pair.Submission_tail_register_address_on_device = SUBMISSION_QUEUE_REGISTER_2;
			nvme_queue_pair.Completion_queue_memory_base_address = COMPLETION_QUEUE_MEMORY_2;
			nvme_queue_pair.Completion_head_register_address_on_device = COMPLETION_QUEUE_REGISTER_2;
			break;
		case 3:
			nvme_queue_pair.Submission_queue_memory_base_address = SUBMISSION_QUEUE_MEMORY_3;
			nvme_queue_pair.Submission_tail_register_address_on_device = SUBMISSION_QUEUE_REGISTER_3;
			nvme_queue_pair.Completion_queue_memory_base_address = COMPLETION_QUEUE_MEMORY_3;
			nvme_queue_pair.Completion_head_register_address_on_device = COMPLETION_QUEUE_REGISTER_3;
			break;
		case 4:
			nvme_queue_pair.Submission_queue_memory_base_address = SUBMISSION_QUEUE_MEMORY_4;
			nvme_queue_pair.Submission_tail_register_address_on_device = SUBMISSION_QUEUE_REGISTER_4;
			nvme_queue_pair.Completion_queue_memory_base_address = COMPLETION_QUEUE_MEMORY_4;
			nvme_queue_pair.Completion_head_register_address_on_device = COMPLETION_QUEUE_REGISTER_4;
			break;
		case 5:
			nvme_queue_pair.Submission_queue_memory_base_address = SUBMISSION_QUEUE_MEMORY_5;
			nvme_queue_pair.Submission_tail_register_address_on_device = SUBMISSION_QUEUE_REGISTER_5;
			nvme_queue_pair.Completion_queue_memory_base_address = COMPLETION_QUEUE_MEMORY_5;
			nvme_queue_pair.Completion_head_register_address_on_device = COMPLETION_QUEUE_REGISTER_5;
			break;
		case 6:
			nvme_queue_pair.Submission_queue_memory_base_address = SUBMISSION_QUEUE_MEMORY_6;
			nvme_queue_pair.Submission_tail_register_address_on_device = SUBMISSION_QUEUE_REGISTER_6;
			nvme_queue_pair.Completion_queue_memory_base_address = COMPLETION_QUEUE_MEMORY_6;
			nvme_queue_pair.Completion_head_register_address_on_device = COMPLETION_QUEUE_REGISTER_6;
			break;
		case 7:
			nvme_queue_pair.Submission_queue_memory_base_address = SUBMISSION_QUEUE_MEMORY_7;
			nvme_queue_pair.Submission_tail_register_address_on_device = SUBMISSION_QUEUE_REGISTER_7;
			nvme_queue_pair.Completion_queue_memory_base_address = COMPLETION_QUEUE_MEMORY_7;
			nvme_queue_pair.Completion_head_register_address_on_device = COMPLETION_QUEUE_REGISTER_7;
			break;
		case 8:
			nvme_queue_pair.Submission_queue_memory_base_address = SUBMISSION_QUEUE_MEMORY_8;
			nvme_queue_pair.Submission_tail_register_address_on_device = SUBMISSION_QUEUE_REGISTER_8;
			nvme_queue_pair.Completion_queue_memory_base_address = COMPLETION_QUEUE_MEMORY_8;
			nvme_queue_pair.Completion_head_register_address_on_device = COMPLETION_QUEUE_REGISTER_8;
			break;
		default:
			break;
		}
		break;
	default:
		break;
	}
	}
	
	IO_Flow_Base::~IO_Flow_Base()
	{
		log_file.close();
		for(auto &req : waiting_requests) {
			if (req) {
				delete req;
			}
		}

		switch (SSD_device_type) {
			case HostInterface_Types::NVME:
				for (auto &req : nvme_software_request_queue) {
					if (req.second) {
						delete req.second;
					}
				}
				break;
			case HostInterface_Types::SATA:
				break;
			default:
				PRINT_ERROR("Unsupported host interface type in IO_Flow_Base!")
		}

	}

	void IO_Flow_Base::Start_simulation()
	{
		next_logging_milestone = logging_period;
		if (enabled_logging) {
			log_file.open(logging_file_path, std::ofstream::out);
		}
		log_file << "SimulationTime(us)\t" << "ReponseTime(us)\t" << "EndToEndDelay(us)"<< std::endl;
		STAT_sum_device_response_time_short_term = 0;
		STAT_serviced_request_count_short_term = 0;
	}

	void IO_Flow_Base::SATA_consume_io_request(Host_IO_Request* request)
	{
		sim_time_type device_response_time = Simulator->Time() - request->Enqueue_time;
		sim_time_type request_delay = Simulator->Time() - request->Arrival_time;
		
		STAT_serviced_request_count++;
		STAT_serviced_request_count_short_term++;
		STAT_sum_device_response_time += device_response_time;
		STAT_sum_device_response_time_short_term += device_response_time;
		STAT_sum_request_delay += request_delay;
		STAT_sum_request_delay_short_term += request_delay;
		if (device_response_time > STAT_max_device_response_time) {
			STAT_max_device_response_time = device_response_time;
		}
		if (device_response_time < STAT_min_device_response_time) {
			STAT_min_device_response_time = device_response_time;
		}
		if (request_delay > STAT_max_request_delay) {
			STAT_max_request_delay = request_delay;
		}
		if (request_delay < STAT_min_request_delay) {
			STAT_min_request_delay = request_delay;
		}
		STAT_transferred_bytes_total += request->LBA_count * SECTOR_SIZE_IN_BYTE;

		if (request->Type == Host_IO_Request_Type::READ) {
			STAT_serviced_read_request_count++;
			STAT_sum_device_response_time_read += device_response_time;
			STAT_sum_request_delay_read += request_delay;
			if (device_response_time > STAT_max_device_response_time_read) {
				STAT_max_device_response_time_read = device_response_time;
			}
			if (device_response_time < STAT_min_device_response_time_read) {
				STAT_min_device_response_time_read = device_response_time;
			}
			if (request_delay > STAT_max_request_delay_read) {
				STAT_max_request_delay_read = request_delay;
			}
			if (request_delay < STAT_min_request_delay_read) {
				STAT_min_request_delay_read = request_delay;
			}
			STAT_transferred_bytes_read += request->LBA_count * SECTOR_SIZE_IN_BYTE;
		} else {
			STAT_serviced_write_request_count++;
			STAT_sum_device_response_time_write += device_response_time;
			STAT_sum_request_delay_write += request_delay;
			if (device_response_time > STAT_max_device_response_time_write) {
				STAT_max_device_response_time_write = device_response_time;
			}
			if (device_response_time < STAT_min_device_response_time_write) {
				STAT_min_device_response_time_write = device_response_time;
			}
			if (request_delay > STAT_max_request_delay_write) {
				STAT_max_request_delay_write = request_delay;
			}
			if (request_delay < STAT_min_request_delay_write) {
				STAT_min_request_delay_write = request_delay;
			}
			STAT_transferred_bytes_write += request->LBA_count * SECTOR_SIZE_IN_BYTE;
		}

		delete request;

		//Announce simulation progress
		if (stop_time > 0) {
			progress = int(Simulator->Time() / (double)stop_time * 100);
		} else {
			progress = int(STAT_serviced_request_count / (double)total_requests_to_be_generated * 100);
		}
		if (progress >= next_progress_step) {
			std::string progress_bar;
			int barWidth = 100;
			progress_bar += "[";
			int pos = progress;
			for (int i = 0; i < barWidth; i += 5) {
				if (i < pos) {
					progress_bar += "=";
				} else if (i == pos) {
					progress_bar += ">";
				} else {
					progress_bar += " ";
				}
			}
			progress_bar += "] ";
			PRINT_MESSAGE(progress_bar << " " << progress << "% progress in " << ID() << std::endl)
				next_progress_step += 5;
		}

		if (Simulator->Time() > next_logging_milestone) {
			log_file << Simulator->Time() / SIM_TIME_TO_MICROSECONDS_COEFF << "\t" << Get_device_response_time_short_term() << "\t" << Get_end_to_end_request_delay_short_term() << std::endl;
			STAT_sum_device_response_time_short_term = 0;
			STAT_sum_request_delay_short_term = 0;
			STAT_serviced_request_count_short_term = 0;
			next_logging_milestone = Simulator->Time() + logging_period;
		}
	}

	void IO_Flow_Base::NVMe_consume_io_request(Completion_Queue_Entry* cqe)
	{
		//Find the request and update statistics
		Host_IO_Request* request = nvme_software_request_queue[cqe->Command_Identifier];
		nvme_software_request_queue.erase(cqe->Command_Identifier);
		available_command_ids.insert(cqe->Command_Identifier);
		sim_time_type device_response_time = Simulator->Time() - request->Enqueue_time;
		sim_time_type request_delay = Simulator->Time() - request->Arrival_time;
		STAT_serviced_request_count++;
		STAT_serviced_request_count_short_term++;

		STAT_sum_device_response_time += device_response_time;
		STAT_sum_device_response_time_short_term += device_response_time;
		STAT_sum_request_delay += request_delay;
		STAT_sum_request_delay_short_term += request_delay;
		if (device_response_time > STAT_max_device_response_time) {
			STAT_max_device_response_time = device_response_time;
		}
		if (device_response_time < STAT_min_device_response_time) {
			STAT_min_device_response_time = device_response_time;
		}
		if (request_delay > STAT_max_request_delay) {
			STAT_max_request_delay = request_delay;
		}
		if (request_delay < STAT_min_request_delay) {
			STAT_min_request_delay = request_delay;
		}
		STAT_transferred_bytes_total += request->LBA_count * SECTOR_SIZE_IN_BYTE;
		
		if (request->Type == Host_IO_Request_Type::READ) {
			STAT_serviced_read_request_count++;
			STAT_sum_device_response_time_read += device_response_time;
			STAT_sum_request_delay_read += request_delay;
			if (device_response_time > STAT_max_device_response_time_read) {
				STAT_max_device_response_time_read = device_response_time;
			}
			if (device_response_time < STAT_min_device_response_time_read) {
				STAT_min_device_response_time_read = device_response_time;
			}
			if (request_delay > STAT_max_request_delay_read) {
				STAT_max_request_delay_read = request_delay;
			}
			if (request_delay < STAT_min_request_delay_read) {
				STAT_min_request_delay_read = request_delay;
			}
			STAT_transferred_bytes_read += request->LBA_count * SECTOR_SIZE_IN_BYTE;
		} else {
			STAT_serviced_write_request_count++;
			STAT_sum_device_response_time_write += device_response_time;
			STAT_sum_request_delay_write += request_delay;
			if (device_response_time > STAT_max_device_response_time_write) {
				STAT_max_device_response_time_write = device_response_time;
			}
			if (device_response_time < STAT_min_device_response_time_write) {
				STAT_min_device_response_time_write = device_response_time;
			}
			if (request_delay > STAT_max_request_delay_write) {
				STAT_max_request_delay_write = request_delay;
			}
			if (request_delay < STAT_min_request_delay_write) {
				STAT_min_request_delay_write = request_delay;
			}
			STAT_transferred_bytes_write += request->LBA_count * SECTOR_SIZE_IN_BYTE;
		}

		delete request;

		nvme_queue_pair.Submission_queue_head = cqe->SQ_Head;
		
		//MQSim always assumes that the request is processed correctly, so no need to check cqe->SF_P

		//If the submission queue is not full anymore, then enqueue waiting requests
		while(waiting_requests.size() > 0) {
			if (!NVME_SQ_FULL(nvme_queue_pair) && available_command_ids.size() > 0) {
				Host_IO_Request* new_req = waiting_requests.front();
				waiting_requests.pop_front();
				if (nvme_software_request_queue[*available_command_ids.begin()] != NULL) {
					PRINT_ERROR("Unexpteced situation in IO_Flow_Base! Overwriting a waiting I/O request in the queue!")
				} else {
					new_req->IO_queue_info = *available_command_ids.begin();
					nvme_software_request_queue[*available_command_ids.begin()] = new_req;
					available_command_ids.erase(available_command_ids.begin());
					request_queue_in_memory[nvme_queue_pair.Submission_queue_tail] = new_req;
					NVME_UPDATE_SQ_TAIL(nvme_queue_pair);
				}
				new_req->Enqueue_time = Simulator->Time();
				pcie_root_complex->Write_to_device(nvme_queue_pair.Submission_tail_register_address_on_device, nvme_queue_pair.Submission_queue_tail);//Based on NVMe protocol definition, the updated tail pointer should be informed to the device
			} else {
				break;
			}
		}

		delete cqe;

		//Announce simulation progress
		if (stop_time > 0) {
			progress = int(Simulator->Time() / (double)stop_time * 100);
		} else {
			progress = int(STAT_serviced_request_count / (double)total_requests_to_be_generated * 100);
		}

		if (progress >= next_progress_step) {
			std::string progress_bar;
			int barWidth = 100;
			progress_bar += "[";
			int pos = progress;
			for (int i = 0; i < barWidth; i += 5) {
				if (i < pos) {
					progress_bar += "=";
				} else if (i == pos) {
					progress_bar += ">";
				} else {
					progress_bar += " ";
				}
			}
			progress_bar += "] ";
			PRINT_MESSAGE(progress_bar << " " << progress << "% progress in " << ID() << std::endl)
			next_progress_step += 5;
		}

		if (Simulator->Time() > next_logging_milestone) {
			log_file << Simulator->Time() / SIM_TIME_TO_MICROSECONDS_COEFF << "\t" << Get_device_response_time_short_term() << "\t" << Get_end_to_end_request_delay_short_term() << std::endl;
			STAT_sum_device_response_time_short_term = 0;
			STAT_sum_request_delay_short_term = 0;
			STAT_serviced_request_count_short_term = 0;
			next_logging_milestone = Simulator->Time() + logging_period;
		}
	}
	
	Submission_Queue_Entry* IO_Flow_Base::NVMe_read_sqe(uint64_t address)
	{
		Submission_Queue_Entry* sqe = new Submission_Queue_Entry;
		Host_IO_Request* request = request_queue_in_memory[(uint16_t)((address - nvme_queue_pair.Submission_queue_memory_base_address) / sizeof(Submission_Queue_Entry))];
		
		if (request == NULL) {
			throw std::invalid_argument(this->ID() + ": Request to access a submission queue entry that does not exist.");
		}

		sqe->Command_Identifier = request->IO_queue_info;
		if (request->Type == Host_IO_Request_Type::READ) {
			sqe->Opcode = NVME_READ_OPCODE;
			sqe->Command_specific[0] = (uint32_t) request->Start_LBA;
			sqe->Command_specific[1] = (uint32_t)(request->Start_LBA >> 32);
			sqe->Command_specific[2] = ((uint32_t)((uint16_t)request->LBA_count)) & (uint32_t)(0x0000ffff);
			sqe->PRP_entry_1 = (DATA_MEMORY_REGION);//Dummy addresses, just to emulate data read/write access
			sqe->PRP_entry_2 = (DATA_MEMORY_REGION + 0x1000);//Dummy addresses
		} else {
			sqe->Opcode = NVME_WRITE_OPCODE;
			sqe->Command_specific[0] = (uint32_t)request->Start_LBA;
			sqe->Command_specific[1] = (uint32_t)(request->Start_LBA >> 32);
			sqe->Command_specific[2] = ((uint32_t)((uint16_t)request->LBA_count)) & (uint32_t)(0x0000ffff);
			sqe->PRP_entry_1 = (DATA_MEMORY_REGION);//Dummy addresses, just to emulate data read/write access
			sqe->PRP_entry_2 = (DATA_MEMORY_REGION + 0x1000);//Dummy addresses
		}

		return sqe;
	}

	void IO_Flow_Base::Submit_io_request(Host_IO_Request* request)
	{
		switch (SSD_device_type) {
			case HostInterface_Types::NVME:
				//If either of software or hardware queue is full
				if (NVME_SQ_FULL(nvme_queue_pair) || available_command_ids.size() == 0) {
					waiting_requests.push_back(request);
				} else {
					if (nvme_software_request_queue[*available_command_ids.begin()] != NULL) {
						PRINT_ERROR("Unexpteced situation in IO_Flow_Base! Overwriting an unhandled I/O request in the queue!")
					} else {
						request->IO_queue_info = *available_command_ids.begin();
						nvme_software_request_queue[*available_command_ids.begin()] = request;
						available_command_ids.erase(available_command_ids.begin());
						request_queue_in_memory[nvme_queue_pair.Submission_queue_tail] = request;
						NVME_UPDATE_SQ_TAIL(nvme_queue_pair);
					}
					request->Enqueue_time = Simulator->Time();
					pcie_root_complex->Write_to_device(nvme_queue_pair.Submission_tail_register_address_on_device, nvme_queue_pair.Submission_queue_tail);//Based on NVMe protocol definition, the updated tail pointer should be informed to the device
				}
				break;
			case HostInterface_Types::SATA:
				request->Source_flow_id = flow_id;
				sata_hba->Submit_io_request(request);
				break;
		}
	}

	void IO_Flow_Base::NVMe_update_and_submit_completion_queue_tail()
	{
		nvme_queue_pair.Completion_queue_head++;
		if (nvme_queue_pair.Completion_queue_head == nvme_queue_pair.Completion_queue_size) {
			nvme_queue_pair.Completion_queue_head = 0;
		}
		pcie_root_complex->Write_to_device(nvme_queue_pair.Completion_head_register_address_on_device, nvme_queue_pair.Completion_queue_head);//Based on NVMe protocol definition, the updated head pointer should be informed to the device
	}

	const NVMe_Queue_Pair* IO_Flow_Base::Get_nvme_queue_pair_info()
	{
		return &nvme_queue_pair;
	}

	LHA_type IO_Flow_Base::Get_start_lsa_on_device()
	{
		return start_lsa_on_device;
	}

	LHA_type IO_Flow_Base::Get_end_lsa_address_on_device()
	{
		return end_lsa_on_device;
	}

	uint32_t IO_Flow_Base::Get_generated_request_count()
	{
		return STAT_generated_request_count;
	}

	uint32_t IO_Flow_Base::Get_serviced_request_count()
	{
		return STAT_serviced_request_count;
	}

	uint32_t IO_Flow_Base::Get_device_response_time()
	{
		if (STAT_serviced_request_count == 0) {
			return 0;
		}

		return (uint32_t)(STAT_sum_device_response_time / STAT_serviced_request_count / SIM_TIME_TO_MICROSECONDS_COEFF);
	}

	uint32_t IO_Flow_Base::Get_min_device_response_time()
	{
		return (uint32_t)(STAT_min_device_response_time / SIM_TIME_TO_MICROSECONDS_COEFF);
	}

	uint32_t IO_Flow_Base::Get_max_device_response_time()
	{
		return (uint32_t)(STAT_max_device_response_time / SIM_TIME_TO_MICROSECONDS_COEFF);
	}

	uint32_t IO_Flow_Base::Get_end_to_end_request_delay()
	{
		if (STAT_serviced_request_count == 0) {
			return 0;
		}

		return (uint32_t)(STAT_sum_request_delay / STAT_serviced_request_count / SIM_TIME_TO_MICROSECONDS_COEFF);
	}

	uint32_t IO_Flow_Base::Get_min_end_to_end_request_delay()
	{
		return (uint32_t)(STAT_min_request_delay / SIM_TIME_TO_MICROSECONDS_COEFF);
	}

	uint32_t IO_Flow_Base::Get_max_end_to_end_request_delay()
	{
		return (uint32_t)(STAT_max_request_delay / SIM_TIME_TO_MICROSECONDS_COEFF);
	}

	uint32_t IO_Flow_Base::Get_device_response_time_short_term()
	{
		if (STAT_serviced_request_count_short_term == 0) {
			return 0;
		}

		return (uint32_t)(STAT_sum_device_response_time_short_term / STAT_serviced_request_count_short_term / SIM_TIME_TO_MICROSECONDS_COEFF);
	}

	uint32_t IO_Flow_Base::Get_end_to_end_request_delay_short_term()
	{
		if (STAT_serviced_request_count == 0) {
			return 0;
		}

		return (uint32_t)(STAT_sum_request_delay_short_term / STAT_serviced_request_count_short_term / SIM_TIME_TO_MICROSECONDS_COEFF);
	}
	
	void IO_Flow_Base::Report_results_in_XML(std::string name_prefix, Utils::XmlWriter& xmlwriter)
	{
		std::string tmp = name_prefix + ".IO_Flow";
		xmlwriter.Write_open_tag(tmp);


		std::string attr = "Name";
		std::string val = ID();
		xmlwriter.Write_attribute_string(attr, val);

		attr = "Request_Count";
		val = std::to_string(STAT_generated_request_count);
		xmlwriter.Write_attribute_string(attr, val);

		attr = "Read_Request_Count";
		val = std::to_string(STAT_generated_read_request_count);
		xmlwriter.Write_attribute_string(attr, val);

		attr = "Write_Request_Count";
		val = std::to_string(STAT_generated_write_request_count);
		xmlwriter.Write_attribute_string(attr, val);

		attr = "IOPS";
		val = std::to_string((double)STAT_generated_request_count / (Simulator->Time() / SIM_TIME_TO_SECONDS_COEFF));
		xmlwriter.Write_attribute_string(attr, val);

		attr = "IOPS_Read";
		val = std::to_string((double)STAT_generated_read_request_count / (Simulator->Time() / SIM_TIME_TO_SECONDS_COEFF));
		xmlwriter.Write_attribute_string(attr, val);

		attr = "IOPS_Write";
		val = std::to_string((double)STAT_generated_write_request_count / (Simulator->Time() / SIM_TIME_TO_SECONDS_COEFF));
		xmlwriter.Write_attribute_string(attr, val);

		attr = "Bytes_Transferred";
		val = std::to_string((double)STAT_transferred_bytes_total);
		xmlwriter.Write_attribute_string(attr, val);

		attr = "Bytes_Transferred_Read";
		val = std::to_string((double)STAT_transferred_bytes_read);
		xmlwriter.Write_attribute_string(attr, val);

		attr = "Bytes_Transferred_Write";
		val = std::to_string((double)STAT_transferred_bytes_write);
		xmlwriter.Write_attribute_string(attr, val);

		attr = "Bandwidth";
		val = std::to_string((double)STAT_transferred_bytes_total / (Simulator->Time() / SIM_TIME_TO_SECONDS_COEFF));
		xmlwriter.Write_attribute_string(attr, val);

		attr = "Bandwidth_Read";
		val = std::to_string((double)STAT_transferred_bytes_read / (Simulator->Time() / SIM_TIME_TO_SECONDS_COEFF));
		xmlwriter.Write_attribute_string(attr, val);

		attr = "Bandwidth_Write";
		val = std::to_string((double)STAT_transferred_bytes_write / (Simulator->Time() / SIM_TIME_TO_SECONDS_COEFF));
		xmlwriter.Write_attribute_string(attr, val);


		attr = "Device_Response_Time";
		val = std::to_string(Get_device_response_time());
		xmlwriter.Write_attribute_string(attr, val);

		attr = "Min_Device_Response_Time";
		val = std::to_string(Get_min_device_response_time());
		xmlwriter.Write_attribute_string(attr, val);

		attr = "Max_Device_Response_Time";
		val = std::to_string(Get_max_device_response_time());
		xmlwriter.Write_attribute_string(attr, val);

		attr = "End_to_End_Request_Delay";
		val = std::to_string(Get_end_to_end_request_delay());
		xmlwriter.Write_attribute_string(attr, val);

		attr = "Min_End_to_End_Request_Delay";
		val = std::to_string(Get_min_end_to_end_request_delay());
		xmlwriter.Write_attribute_string(attr, val);

		attr = "Max_End_to_End_Request_Delay";
		val = std::to_string(Get_max_end_to_end_request_delay());
		xmlwriter.Write_attribute_string(attr, val);

		xmlwriter.Write_close_tag();
	}
}
