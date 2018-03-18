#include "IO_Flow_Base.h"
#include "../ssd/Host_Interface_Defs.h"
#include "../sim/Engine.h"

namespace Host_Components
{
	//unsigned int InputStreamBase::lastId = 0;
	IO_Flow_Base::IO_Flow_Base(const sim_object_id_type& name, LSA_type start_lsa_on_device, LSA_type end_lsa_on_device, uint16_t io_queue_id,
		uint16_t nvme_submission_queue_size, uint16_t nvme_completion_queue_size,
		IO_Flow_Priority_Class priority_class, sim_time_type stop_time, unsigned int total_requets_to_be_generated,
		HostInterfaceType SSD_device_type, PCIe_Root_Complex* pcie_root_complex) :
		MQSimEngine::Sim_Object(name), start_lsa_on_device(start_lsa_on_device), end_lsa_on_device(end_lsa_on_device), io_queue_id(io_queue_id),
		priority_class(priority_class), stop_time(stop_time), total_requests_to_be_generated(total_requets_to_be_generated), SSD_device_type(SSD_device_type), pcie_root_complex(pcie_root_complex),
		STAT_generated_request_count(0), STAT_generated_read_request_count(0), STAT_generated_write_request_count(0),
		STAT_ignored_request_count(0),
		STAT_serviced_request_count(0), STAT_serviced_read_request_count(0), STAT_serviced_write_request_count(0),
		STAT_sum_device_response_time(0), STAT_sum_device_response_time_read(0), STAT_sum_device_response_time_write(0),
		STAT_min_device_response_time(MAXIMUM_TIME), STAT_min_device_response_time_read(MAXIMUM_TIME), STAT_min_device_response_time_write(MAXIMUM_TIME),
		STAT_max_device_response_time(0), STAT_max_device_response_time_read(0), STAT_max_device_response_time_write(0),
		STAT_sum_request_delay(0), STAT_sum_request_delay_read(0), STAT_sum_request_delay_write(0),
		STAT_min_request_delay(MAXIMUM_TIME), STAT_min_request_delay_read(MAXIMUM_TIME), STAT_min_request_delay_write(MAXIMUM_TIME),
		STAT_max_request_delay(0), STAT_max_request_delay_read(0), STAT_max_request_delay_write(0),
		STAT_transferred_bytes_total(0), STAT_transferred_bytes_read(0), STAT_transferred_bytes_write(0), progress(0), next_progress_step(0)
	{
		switch (SSD_device_type)
		{
		case HostInterfaceType::SATA:
			break;
		case HostInterfaceType::NVME:
				nvme_queue_pair.Submission_queue_size = nvme_submission_queue_size;
				nvme_queue_pair.Submission_queue_head = 0;
				nvme_queue_pair.Submission_queue_tail = 0;
				nvme_queue_pair.Completion_queue_size = nvme_completion_queue_size;
				nvme_queue_pair.Completion_queue_head = 0;
				nvme_queue_pair.Completion_queue_tail = 0;
				switch (io_queue_id)//id = 0: admin queues, id = 1 to 8, normal I/O queues
				{
				case 0:
					throw "I/O queue id 0 is reserved for NVMe admin queues and should not be used for I/O flows";
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

	void IO_Flow_Base::NVMe_consume_io_request(Completion_Queue_Entry* cqe)
	{
		//Find the request and update statistics
		Host_IO_Reqeust* request = enqueued_requests[cqe->Command_Identifier];
		enqueued_requests.erase(cqe->Command_Identifier);
		sim_time_type device_response_time = Simulator->Time() - request->Enqueue_time;
		sim_time_type request_delay = Simulator->Time() - request->Arrival_time;
		DEBUG("* Host: Request #" << cqe->Command_Identifier << " finished: end-to-end delay:" << (request_delay / 1000) << "(us) device response time:" << (device_response_time / 1000) << "(us)")
		STAT_serviced_request_count++;
		STAT_sum_device_response_time += device_response_time;
		STAT_sum_request_delay += request_delay;
		if (device_response_time > STAT_max_device_response_time)
			STAT_max_device_response_time = device_response_time;
		if (device_response_time < STAT_min_device_response_time)
			STAT_min_device_response_time = device_response_time;
		if (request_delay > STAT_max_request_delay)
			STAT_max_request_delay = request_delay;
		if (request_delay < STAT_min_request_delay)
			STAT_min_request_delay = request_delay;
		STAT_transferred_bytes_total = request->LBA_count * SECTOR_SIZE_IN_BYTE;

		if (request->Type == Host_IO_Request_Type::READ)
		{
			STAT_serviced_read_request_count++;
			STAT_sum_device_response_time_read += device_response_time;
			STAT_sum_request_delay_read += request_delay;
			if (device_response_time > STAT_max_device_response_time_read)
				STAT_max_device_response_time_read = device_response_time;
			if (device_response_time < STAT_min_device_response_time_read)
				STAT_min_device_response_time_read = device_response_time;
			if (request_delay > STAT_max_request_delay_read)
				STAT_max_request_delay_read = request_delay;
			if (request_delay < STAT_min_request_delay_read)
				STAT_min_request_delay_read = request_delay;
			STAT_transferred_bytes_read = request->LBA_count * SECTOR_SIZE_IN_BYTE;
		}
		else
		{
			STAT_serviced_write_request_count++;
			STAT_sum_device_response_time_write += device_response_time;
			STAT_sum_request_delay_write += request_delay;
			if (device_response_time > STAT_max_device_response_time_write)
				STAT_max_device_response_time_write = device_response_time;
			if (device_response_time < STAT_min_device_response_time_write)
				STAT_min_device_response_time_write = device_response_time;
			if (request_delay > STAT_max_request_delay_write)
				STAT_max_request_delay_write = request_delay;
			if (request_delay < STAT_min_request_delay_write)
				STAT_min_request_delay_write = request_delay;
			STAT_transferred_bytes_write = request->LBA_count * SECTOR_SIZE_IN_BYTE;
		}

		delete request;

		nvme_queue_pair.Submission_queue_head = cqe->SQ_Head;
		
		//MQSim always assumes that the request is processed correctly, so no need to check cqe->SF_P

		//If the submission queue is not full anymore, then enqueue waiting requests
		if(waiting_requests.size() > 0)
			if (!NVME_SQ_FULL(nvme_queue_pair))
			{
				Host_IO_Reqeust* new_req = waiting_requests.front();
				waiting_requests.pop_front();
				new_req->IO_queue_info = nvme_queue_pair.Submission_queue_tail++;
				if (nvme_queue_pair.Submission_queue_tail == nvme_queue_pair.Submission_queue_size)
					nvme_queue_pair.Submission_queue_tail = 0;
				enqueued_requests[new_req->IO_queue_info] = new_req;
				new_req->Enqueue_time = Simulator->Time();
				pcie_root_complex->Write_to_device(nvme_queue_pair.Submission_tail_register_address_on_device, nvme_queue_pair.Submission_queue_tail);//Based on NVMe protocol definition, the updated tail pointer should be informed to the device
			}

		delete cqe;

		//Announce simulation progress
		if (stop_time > 0)
		{
			progress = int(Simulator->Time() / (double)stop_time * 100);
		}
		else
		{
			progress = int(STAT_serviced_request_count / (double)total_requests_to_be_generated * 100);
		}
		if (progress == next_progress_step)
		{
			std::string progress_bar;
			int barWidth = 100;
			progress_bar += "[";
			int pos = progress;
			for (int i = 0; i < barWidth; i += 5) {
				if (i < pos) progress_bar += "=";
				else if (i == pos) progress_bar += ">";
				else progress_bar += " ";
			}
			progress_bar += "] ";
			PRINT_MESSAGE(progress_bar << " " << progress << "% progress in " << ID() << std::endl)
			next_progress_step += 5;
		}
	}

	Submission_Queue_Entry* IO_Flow_Base::NVMe_read_sqe(uint64_t address)
	{
		Submission_Queue_Entry* sqe = new Submission_Queue_Entry;
		Host_IO_Reqeust* request = enqueued_requests[(uint16_t)((address - nvme_queue_pair.Submission_queue_memory_base_address) / sizeof(Submission_Queue_Entry))];
		if (request == NULL)
			throw this->ID() + std::string(": Request to access a submission queue entry that does not exist.\n");
		sqe->Command_Identifier = request->IO_queue_info;
		if (request->Type == Host_IO_Request_Type::READ)
		{
			sqe->Opcode = NVME_READ_OPCODE;
			sqe->Command_specific[0] = (uint32_t) request->Start_LBA;
			sqe->Command_specific[1] = (uint32_t)(request->Start_LBA >> 32);
			sqe->Command_specific[2] = ((uint32_t)((uint16_t)request->LBA_count)) & (uint32_t)(0x0000ffff);
			sqe->PRP_entry_1 = (DATA_MEMORY_REGION);//Dummy addresses, just to emulate data read/write access
			sqe->PRP_entry_2 = (DATA_MEMORY_REGION + 0x1000);//Dummy addresses
		}
		else
		{
			sqe->Opcode = NVME_WRITE_OPCODE;
			sqe->Command_specific[0] = (uint32_t)request->Start_LBA;
			sqe->Command_specific[1] = (uint32_t)(request->Start_LBA >> 32);
			sqe->Command_specific[2] = ((uint32_t)((uint16_t)request->LBA_count)) & (uint32_t)(0x0000ffff);
			sqe->PRP_entry_1 = (DATA_MEMORY_REGION);//Dummy addresses, just to emulate data read/write access
			sqe->PRP_entry_2 = (DATA_MEMORY_REGION + 0x1000);//Dummy addresses
		}
		return sqe;
	}

	void IO_Flow_Base::submit_io_request(Host_IO_Reqeust* request)
	{
		switch (SSD_device_type)
		{
		case HostInterfaceType::NVME:
			if (NVME_SQ_FULL(nvme_queue_pair))
				waiting_requests.push_back(request);
			else
			{
				request->IO_queue_info = nvme_queue_pair.Submission_queue_tail++;
				if (nvme_queue_pair.Submission_queue_tail == nvme_queue_pair.Submission_queue_size)
					nvme_queue_pair.Submission_queue_tail = 0;
				enqueued_requests[request->IO_queue_info] = request;
				request->Enqueue_time = Simulator->Time();
				pcie_root_complex->Write_to_device(nvme_queue_pair.Submission_tail_register_address_on_device, nvme_queue_pair.Submission_queue_tail);//Based on NVMe protocol definition, the updated tail pointer should be informed to the device
			}
			break;
		case HostInterfaceType::SATA:
			break;
		}
	}

	void IO_Flow_Base::NVMe_update_and_submit_completion_queue_tail()
	{
		nvme_queue_pair.Completion_queue_head++;
		if (nvme_queue_pair.Completion_queue_head == nvme_queue_pair.Completion_queue_size)
			nvme_queue_pair.Completion_queue_head = 0;
		pcie_root_complex->Write_to_device(nvme_queue_pair.Completion_head_register_address_on_device, nvme_queue_pair.Completion_queue_head);//Based on NVMe protocol definition, the updated head pointer should be informed to the device
	}
	
	const NVMe_Queue_Pair* IO_Flow_Base::Get_nvme_queue_pair_info()
	{
		return &nvme_queue_pair;
	}

	LSA_type IO_Flow_Base::Get_start_lsa_on_device() 
	{
		return start_lsa_on_device;
	}
	
	LSA_type IO_Flow_Base::Get_end_lsa_address_on_device()
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

	void IO_Flow_Base::Report_results_in_XML(Utils::XmlWriter& xmlwriter)
	{
		std::string tmp;
		tmp = "IO_Flow";
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