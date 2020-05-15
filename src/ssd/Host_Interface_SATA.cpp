#include "Host_Interface_SATA.h"

namespace SSD_Components
{
	Input_Stream_SATA::~Input_Stream_SATA()
	{
		for (auto &user_request : Waiting_user_requests) {
			delete user_request;
		}
		for (auto &user_request : Completed_user_requests) {
			delete user_request;
		}
	}

	Input_Stream_Manager_SATA::Input_Stream_Manager_SATA(Host_Interface_Base* host_interface, uint16_t ncq_depth,
		LHA_type start_logical_sector_address, LHA_type end_logical_sector_address) :
		ncq_depth(ncq_depth), Input_Stream_Manager_Base(host_interface)
	{
		if (end_logical_sector_address < start_logical_sector_address) {
			PRINT_ERROR("Error in allocating address range to a stream in host interface: the start address should be smaller than the end address.")
		}
		Input_Stream_SATA* input_stream = new Input_Stream_SATA(start_logical_sector_address, end_logical_sector_address, 0, 0);
		this->input_streams.push_back(input_stream);
	}

	void Input_Stream_Manager_SATA::Set_ncq_address(uint64_t submission_queue_base_address, uint64_t completion_queue_base_address)
	{
		((Input_Stream_SATA*)this->input_streams[SATA_STREAM_ID])->Submission_queue_base_address = submission_queue_base_address;
		((Input_Stream_SATA*)this->input_streams[SATA_STREAM_ID])->Completion_queue_base_address = completion_queue_base_address;
	}

	inline void Input_Stream_Manager_SATA::Submission_queue_tail_pointer_update(uint16_t tail_pointer_value)
	{
		((Input_Stream_SATA*)input_streams[SATA_STREAM_ID])->Submission_tail = tail_pointer_value;
		((Host_Interface_SATA*)host_interface)->request_fetch_unit->Fetch_next_request(SATA_STREAM_ID);
		((Input_Stream_SATA*)input_streams[SATA_STREAM_ID])->On_the_fly_requests++;
		((Input_Stream_SATA*)input_streams[SATA_STREAM_ID])->Submission_head++;
		if (((Input_Stream_SATA*)input_streams[SATA_STREAM_ID])->Submission_head == ncq_depth) {
			((Input_Stream_SATA*)input_streams[SATA_STREAM_ID])->Submission_head = 0;
		}
	}

	inline void Input_Stream_Manager_SATA::Completion_queue_head_pointer_update(uint16_t head_pointer_value)
	{
		((Input_Stream_SATA*)input_streams[SATA_STREAM_ID])->Completion_head = head_pointer_value;

		if (((Input_Stream_SATA*)input_streams[SATA_STREAM_ID])->Completed_user_requests.size() > 0) {
			User_Request* request = ((Input_Stream_SATA*)input_streams[SATA_STREAM_ID])->Completed_user_requests.front();
			((Input_Stream_SATA*)input_streams[SATA_STREAM_ID])->Completed_user_requests.pop_front();
			inform_host_request_completed(request);
		}
	}

	inline void Input_Stream_Manager_SATA::Handle_new_arrived_request(User_Request* request)
	{
		((Input_Stream_SATA*)input_streams[SATA_STREAM_ID])->Submission_head_informed_to_host++;
		if (((Input_Stream_SATA*)input_streams[SATA_STREAM_ID])->Submission_head_informed_to_host == ncq_depth) {
			((Input_Stream_SATA*)input_streams[SATA_STREAM_ID])->Submission_head_informed_to_host = 0;
		}
		if (request->Type == UserRequestType::READ) {
			((Input_Stream_SATA*)input_streams[SATA_STREAM_ID])->Waiting_user_requests.push_back(request);
			((Input_Stream_SATA*)input_streams[SATA_STREAM_ID])->STAT_number_of_read_requests++;
			segment_user_request(request);

			((Host_Interface_SATA*)host_interface)->broadcast_user_request_arrival_signal(request);
		} else {//This is a write request
			((Input_Stream_SATA*)input_streams[SATA_STREAM_ID])->Waiting_user_requests.push_back(request);
			((Input_Stream_SATA*)input_streams[SATA_STREAM_ID])->STAT_number_of_write_requests++;
			((Host_Interface_SATA*)host_interface)->request_fetch_unit->Fetch_write_data(request);
		}
	}

	inline void Input_Stream_Manager_SATA::Handle_arrived_write_data(User_Request* request)
	{
		segment_user_request(request);
		((Host_Interface_SATA*)host_interface)->broadcast_user_request_arrival_signal(request);
	}

	inline void Input_Stream_Manager_SATA::Handle_serviced_request(User_Request* request)
	{
		((Input_Stream_SATA*)input_streams[SATA_STREAM_ID])->Waiting_user_requests.remove(request);
		((Input_Stream_SATA*)input_streams[SATA_STREAM_ID])->On_the_fly_requests--;

		DEBUG("** Host Interface: Request #" << request->ID << " is finished")

		//If this is a read request, then the read data should be written to host memory
		if (request->Type == UserRequestType::READ) {
			((Host_Interface_SATA*)host_interface)->request_fetch_unit->Send_read_data(request);
		}

		if (((Input_Stream_SATA*)input_streams[SATA_STREAM_ID])->Submission_head != ((Input_Stream_SATA*)input_streams[SATA_STREAM_ID])->Submission_tail) {
			((Host_Interface_SATA*)host_interface)->request_fetch_unit->Fetch_next_request(SATA_STREAM_ID);
			((Input_Stream_SATA*)input_streams[SATA_STREAM_ID])->On_the_fly_requests++;
			((Input_Stream_SATA*)input_streams[SATA_STREAM_ID])->Submission_head++;//Update submission queue head after starting fetch request
			if (((Input_Stream_SATA*)input_streams[SATA_STREAM_ID])->Submission_head == ncq_depth) {//Circular queue implementation
				((Input_Stream_SATA*)input_streams[SATA_STREAM_ID])->Submission_head = 0;
			}
		}

		//Check if completion queue is full
		if (((Input_Stream_SATA*)input_streams[SATA_STREAM_ID])->Completion_head > ((Input_Stream_SATA*)input_streams[SATA_STREAM_ID])->Completion_tail) {
			//completion queue is full
			if (((Input_Stream_SATA*)input_streams[SATA_STREAM_ID])->Completion_tail + 1 == ((Input_Stream_SATA*)input_streams[SATA_STREAM_ID])->Completion_head) {
				((Input_Stream_SATA*)input_streams[SATA_STREAM_ID])->Completed_user_requests.push_back(request);//Wait while the completion queue is full
				return;
			}
		} else if (((Input_Stream_SATA*)input_streams[SATA_STREAM_ID])->Completion_tail - ((Input_Stream_SATA*)input_streams[SATA_STREAM_ID])->Completion_head
			== ncq_depth - 1) {
			((Input_Stream_SATA*)input_streams[SATA_STREAM_ID])->Completed_user_requests.push_back(request);//Wait while the completion queue is full
			return;
		}

		inform_host_request_completed(request);//Completion queue is not full, so the device can DMA the completion queue entry to the host
		DELETE_REQUEST_NVME(request);
	}

	inline void Input_Stream_Manager_SATA::inform_host_request_completed(User_Request* request)
	{
		((Request_Fetch_Unit_SATA*)((Host_Interface_SATA*)host_interface)->request_fetch_unit)->Send_completion_queue_element(request, ((Input_Stream_SATA*)input_streams[SATA_STREAM_ID])->Submission_head_informed_to_host);
		((Input_Stream_SATA*)input_streams[SATA_STREAM_ID])->Completion_tail++;//Next free slot in the completion queue
		
		//Circular queue implementation
		if (((Input_Stream_SATA*)input_streams[SATA_STREAM_ID])->Completion_tail == ncq_depth) {
			((Input_Stream_SATA*)input_streams[SATA_STREAM_ID])->Completion_tail = 0;
		}
	}

	void Input_Stream_Manager_SATA::segment_user_request(User_Request* user_request)
	{
		LHA_type lsa = user_request->Start_LBA;
		LHA_type lsa2 = user_request->Start_LBA;
		unsigned int req_size = user_request->SizeInSectors;

		page_status_type access_status_bitmap = 0;
		unsigned int handled_sectors_count = 0;
		unsigned int transaction_size = 0;
		while (handled_sectors_count < req_size) {
			//Check if LSA is in the correct range allocted to the stream
			if (lsa < ((Input_Stream_SATA*)input_streams[SATA_STREAM_ID])->Start_logical_sector_address || lsa >((Input_Stream_SATA*)input_streams[SATA_STREAM_ID])->End_logical_sector_address) {
				lsa = ((Input_Stream_SATA*)input_streams[SATA_STREAM_ID])->Start_logical_sector_address
				+ (lsa % (((Input_Stream_SATA*)input_streams[SATA_STREAM_ID])->End_logical_sector_address - (((Input_Stream_SATA*)input_streams[SATA_STREAM_ID])->Start_logical_sector_address)));
			}
			LHA_type internal_lsa = lsa - ((Input_Stream_SATA*)input_streams[SATA_STREAM_ID])->Start_logical_sector_address;//For each flow, all lsa's should be translated into a range starting from zero

			transaction_size = host_interface->sectors_per_page - (unsigned int)(lsa % host_interface->sectors_per_page);
			if (handled_sectors_count + transaction_size >= req_size) {
				transaction_size = req_size - handled_sectors_count;
			}
			LPA_type lpa = internal_lsa / host_interface->sectors_per_page;

			page_status_type temp = ~(0xffffffffffffffff << (int)transaction_size);
			access_status_bitmap = temp << (int)(internal_lsa % host_interface->sectors_per_page);

			if (user_request->Type == UserRequestType::READ) {
				NVM_Transaction_Flash_RD* transaction = new NVM_Transaction_Flash_RD(Transaction_Source_Type::USERIO, SATA_STREAM_ID,
					transaction_size * SECTOR_SIZE_IN_BYTE, lpa, NO_PPA, user_request, 0, access_status_bitmap, CurrentTimeStamp);
				user_request->Transaction_list.push_back(transaction);
				input_streams[SATA_STREAM_ID]->STAT_number_of_read_transactions++;
			} else {//user_request->Type == UserRequestType::WRITE
				NVM_Transaction_Flash_WR* transaction = new NVM_Transaction_Flash_WR(Transaction_Source_Type::USERIO, SATA_STREAM_ID,
					transaction_size * SECTOR_SIZE_IN_BYTE, lpa, user_request, 0, access_status_bitmap, CurrentTimeStamp);
				user_request->Transaction_list.push_back(transaction);
				input_streams[SATA_STREAM_ID]->STAT_number_of_write_transactions++;
			}

			lsa = lsa + transaction_size;
			handled_sectors_count += transaction_size;
		}
	}

	Request_Fetch_Unit_SATA::Request_Fetch_Unit_SATA(Host_Interface_Base* host_interface, uint16_t ncq_depth) :
		Request_Fetch_Unit_Base(host_interface), current_phase(0xffff), number_of_sent_cqe(0), ncq_depth(ncq_depth) {}

	void Request_Fetch_Unit_SATA::Process_pcie_write_message(uint64_t address, void * payload, unsigned int payload_size)
	{
		Host_Interface_SATA* hi = (Host_Interface_SATA*)host_interface;
		uint64_t val = (uint64_t)payload;
		switch (address)
		{
			case NCQ_SUBMISSION_REGISTER:
				((Input_Stream_Manager_SATA*)(hi->input_stream_manager))->Submission_queue_tail_pointer_update((uint16_t)val);
				break;
			case NCQ_COMPLETION_REGISTER:
				((Input_Stream_Manager_SATA*)(hi->input_stream_manager))->Completion_queue_head_pointer_update((uint16_t)val);
				break;
			default:
				throw std::invalid_argument("Unknown register is written in Request_Fetch_Unit_SATA!");
		}
	}

	void Request_Fetch_Unit_SATA::Process_pcie_read_message(uint64_t address, void * payload, unsigned int payload_size)
	{
		Host_Interface_SATA* hi = (Host_Interface_SATA*)host_interface;
		DMA_Req_Item* dma_req_item = dma_list.front();
		dma_list.pop_front();

		switch (dma_req_item->Type)
		{
			case DMA_Req_Type::REQUEST_INFO:
			{
				User_Request* new_request = new User_Request;
				new_request->IO_command_info = payload;
				new_request->Stream_id = (stream_id_type)((uint64_t)(dma_req_item->object));
				new_request->STAT_InitiationTime = Simulator->Time();
				Submission_Queue_Entry* sqe = (Submission_Queue_Entry*)payload;
				switch (sqe->Opcode)
				{
					case SATA_READ_OPCODE:
						new_request->Type = UserRequestType::READ;
						new_request->Start_LBA = ((LHA_type)sqe->Command_specific[1]) << 31 | (LHA_type)sqe->Command_specific[0];//Command Dword 10 and Command Dword 11
						new_request->SizeInSectors = sqe->Command_specific[2] & (LHA_type)(0x0000ffff);
						new_request->Size_in_byte = new_request->SizeInSectors * SECTOR_SIZE_IN_BYTE;
						break;
					case SATA_WRITE_OPCODE:
						new_request->Type = UserRequestType::WRITE;
						new_request->Start_LBA = ((LHA_type)sqe->Command_specific[1]) << 31 | (LHA_type)sqe->Command_specific[0];//Command Dword 10 and Command Dword 11
						new_request->SizeInSectors = sqe->Command_specific[2] & (LHA_type)(0x0000ffff);
						new_request->Size_in_byte = new_request->SizeInSectors * SECTOR_SIZE_IN_BYTE;
						break;
					default:
						throw std::invalid_argument("SATA command is not supported!");
				}
				((Input_Stream_Manager_SATA*)(hi->input_stream_manager))->Handle_new_arrived_request(new_request);
				break;
			}
			case DMA_Req_Type::WRITE_DATA:
				COPYDATA(((User_Request*)dma_req_item->object)->Data, payload, payload_size);
				((Input_Stream_Manager_SATA*)(hi->input_stream_manager))->Handle_arrived_write_data((User_Request*)dma_req_item->object);
				break;
			default:
				break;
		}

		delete dma_req_item;
	}

	void Request_Fetch_Unit_SATA::Fetch_next_request(stream_id_type stream_id)
	{
		DMA_Req_Item* dma_req_item = new DMA_Req_Item;
		dma_req_item->Type = DMA_Req_Type::REQUEST_INFO;
		dma_req_item->object = (void *)SATA_STREAM_ID;
		dma_list.push_back(dma_req_item);

		Host_Interface_SATA* hi = (Host_Interface_SATA*)host_interface;
		Input_Stream_SATA* im = ((Input_Stream_SATA*)hi->input_stream_manager->input_streams[SATA_STREAM_ID]);
		host_interface->Send_read_message_to_host(im->Submission_queue_base_address + im->Submission_head * sizeof(Submission_Queue_Entry), sizeof(Submission_Queue_Entry));
	}

	void Request_Fetch_Unit_SATA::Fetch_write_data(User_Request* request)
	{
		DMA_Req_Item* dma_req_item = new DMA_Req_Item;
		dma_req_item->Type = DMA_Req_Type::WRITE_DATA;
		dma_req_item->object = (void *)request;
		dma_list.push_back(dma_req_item);

		Submission_Queue_Entry* sqe = (Submission_Queue_Entry*)request->IO_command_info;
		host_interface->Send_read_message_to_host((sqe->PRP_entry_2 << 31) | sqe->PRP_entry_1, request->Size_in_byte);
	}

	void Request_Fetch_Unit_SATA::Send_completion_queue_element(User_Request* request, uint16_t sq_head_value)
	{
		Host_Interface_SATA* hi = (Host_Interface_SATA*)host_interface;
		Completion_Queue_Entry* cqe = new Completion_Queue_Entry;
		cqe->SQ_Head = sq_head_value;
		cqe->SQ_ID = FLOW_ID_TO_Q_ID(SATA_STREAM_ID);
		cqe->SF_P = 0x0001 & current_phase;
		cqe->Command_Identifier = ((Submission_Queue_Entry*)request->IO_command_info)->Command_Identifier;
		Input_Stream_SATA* im = ((Input_Stream_SATA*)hi->input_stream_manager->input_streams[SATA_STREAM_ID]);
		host_interface->Send_write_message_to_host(im->Completion_queue_base_address + im->Completion_tail * sizeof(Completion_Queue_Entry), cqe, sizeof(Completion_Queue_Entry));
		number_of_sent_cqe++;
		if (number_of_sent_cqe % ncq_depth == 0) {
			if (current_phase == 0xffff) {//According to protocol specification, the value of the Phase Tag is inverted each pass through the Completion Queue
				current_phase = 0xfffe;
			} else {
				current_phase = 0xffff;
			}
		}
	}

	void Request_Fetch_Unit_SATA::Send_read_data(User_Request* request)
	{
		Submission_Queue_Entry* sqe = (Submission_Queue_Entry*)request->IO_command_info;
		host_interface->Send_write_message_to_host(sqe->PRP_entry_1, request->Data, request->Size_in_byte);
	}

	Host_Interface_SATA::Host_Interface_SATA(const sim_object_id_type& id,
		const uint16_t ncq_depth, const LHA_type max_logical_sector_address, const unsigned int sectors_per_page, Data_Cache_Manager_Base* cache) :
		Host_Interface_Base(id, HostInterface_Types::SATA, max_logical_sector_address, sectors_per_page, cache), ncq_depth(ncq_depth)
	{
		this->input_stream_manager = new Input_Stream_Manager_SATA(this, ncq_depth, 0, max_logical_sector_address);
		this->request_fetch_unit = new Request_Fetch_Unit_SATA(this, ncq_depth);
	}

	void Host_Interface_SATA::Set_ncq_address(const uint64_t submission_queue_base_address, const uint64_t completion_queue_base_address)
	{
		((Input_Stream_Manager_SATA*)this->input_stream_manager)->Set_ncq_address(submission_queue_base_address, completion_queue_base_address);
	}

	void Host_Interface_SATA::Validate_simulation_config()
	{
		Host_Interface_Base::Validate_simulation_config();
		if (this->input_stream_manager == NULL) {
			throw std::logic_error("Input stream manager is not set for Host Interface");
		}
		if (this->request_fetch_unit == NULL) {
			throw std::logic_error("Request fetch unit is not set for Host Interface");
		}
	}

	void Host_Interface_SATA::Start_simulation() {}

	void Host_Interface_SATA::Execute_simulator_event(MQSimEngine::Sim_Event* event) {}

	void Host_Interface_SATA::Report_results_in_XML(std::string name_prefix, Utils::XmlWriter& xmlwriter)
	{
		std::string tmp = name_prefix + ".HostInterface";
		xmlwriter.Write_open_tag(tmp);

		std::string attr = "Name";
		std::string val = ID();
		xmlwriter.Write_attribute_string(attr, val);

		attr = "Average_Read_Transaction_Turnaround_Time";
		val = std::to_string(input_stream_manager->Get_average_read_transaction_turnaround_time(SATA_STREAM_ID));
		xmlwriter.Write_attribute_string(attr, val);

		attr = "Average_Read_Transaction_Execution_Time";
		val = std::to_string(input_stream_manager->Get_average_read_transaction_execution_time(SATA_STREAM_ID));
		xmlwriter.Write_attribute_string(attr, val);

		attr = "Average_Read_Transaction_Transfer_Time";
		val = std::to_string(input_stream_manager->Get_average_read_transaction_transfer_time(SATA_STREAM_ID));
		xmlwriter.Write_attribute_string(attr, val);

		attr = "Average_Read_Transaction_Waiting_Time";
		val = std::to_string(input_stream_manager->Get_average_read_transaction_waiting_time(SATA_STREAM_ID));
		xmlwriter.Write_attribute_string(attr, val);

		attr = "Average_Write_Transaction_Turnaround_Time";
		val = std::to_string(input_stream_manager->Get_average_write_transaction_turnaround_time(SATA_STREAM_ID));
		xmlwriter.Write_attribute_string(attr, val);

		attr = "Average_Write_Transaction_Execution_Time";
		val = std::to_string(input_stream_manager->Get_average_write_transaction_execution_time(SATA_STREAM_ID));
		xmlwriter.Write_attribute_string(attr, val);

		attr = "Average_Write_Transaction_Transfer_Time";
		val = std::to_string(input_stream_manager->Get_average_write_transaction_transfer_time(SATA_STREAM_ID));
		xmlwriter.Write_attribute_string(attr, val);

		attr = "Average_Write_Transaction_Waiting_Time";
		val = std::to_string(input_stream_manager->Get_average_write_transaction_waiting_time(SATA_STREAM_ID));
		xmlwriter.Write_attribute_string(attr, val);

		xmlwriter.Write_close_tag();
	}

	uint16_t Host_Interface_SATA::Get_ncq_depth()
	{
		return ncq_depth;
	}
}