#include "Host_Interface_Base.h"
#include "Data_Cache_Manager_Base.h"

namespace SSD_Components
{
	Input_Stream_Base::Input_Stream_Base() :
		STAT_number_of_read_requests(0), STAT_number_of_write_requests(0), 
		STAT_number_of_read_transactions(0), STAT_number_of_write_transactions(0),
		STAT_sum_of_read_transactions_execution_time(0), STAT_sum_of_read_transactions_transfer_time(0), STAT_sum_of_read_transactions_waiting_time(0),
		STAT_sum_of_write_transactions_execution_time(0), STAT_sum_of_write_transactions_transfer_time(0), STAT_sum_of_write_transactions_waiting_time(0)
	{}
	
	Input_Stream_Manager_Base::~Input_Stream_Manager_Base()
	{
		for (auto &stream : input_streams) {
			delete stream;
		}
	}

	Input_Stream_Base::~Input_Stream_Base()
	{
	}

	Request_Fetch_Unit_Base::~Request_Fetch_Unit_Base()
	{
		for (auto &dma_info : dma_list) {
			delete dma_info;
		}
	}

	Host_Interface_Base* Host_Interface_Base::_my_instance = NULL;

	Host_Interface_Base::Host_Interface_Base(const sim_object_id_type& id, HostInterface_Types type, LHA_type max_logical_sector_address, unsigned int sectors_per_page, 
		Data_Cache_Manager_Base* cache)
		: MQSimEngine::Sim_Object(id), type(type), max_logical_sector_address(max_logical_sector_address), 
		sectors_per_page(sectors_per_page), cache(cache)
	{
		_my_instance = this;
	}
	
	Host_Interface_Base::~Host_Interface_Base()
	{
		delete input_stream_manager;
		delete request_fetch_unit;
	}

	void Host_Interface_Base::Setup_triggers()
	{
		Sim_Object::Setup_triggers();
		cache->Connect_to_user_request_serviced_signal(handle_user_request_serviced_signal_from_cache);
		cache->Connect_to_user_memory_transaction_serviced_signal(handle_user_memory_transaction_serviced_signal_from_cache);
	}

	void Host_Interface_Base::Validate_simulation_config()
	{
	}

	void Host_Interface_Base::Send_read_message_to_host(uint64_t addresss, unsigned int request_read_data_size)
	{
		Host_Components::PCIe_Message* pcie_message = new Host_Components::PCIe_Message;
		pcie_message->Type = Host_Components::PCIe_Message_Type::READ_REQ;
		pcie_message->Destination = Host_Components::PCIe_Destination_Type::HOST;
		pcie_message->Address = addresss;
		pcie_message->Payload = (void*)(intptr_t)request_read_data_size;
		pcie_message->Payload_size = sizeof(request_read_data_size);
		pcie_switch->Send_to_host(pcie_message);
	}

	void Host_Interface_Base::Send_write_message_to_host(uint64_t addresss, void* message, unsigned int message_size)
	{
		Host_Components::PCIe_Message* pcie_message = new Host_Components::PCIe_Message;
		pcie_message->Type = Host_Components::PCIe_Message_Type::WRITE_REQ;
		pcie_message->Destination = Host_Components::PCIe_Destination_Type::HOST;
		pcie_message->Address = addresss;
		COPYDATA(pcie_message->Payload, message, pcie_message->Payload_size);
		pcie_message->Payload_size = message_size;
		pcie_switch->Send_to_host(pcie_message);
	}

	void Host_Interface_Base::Attach_to_device(Host_Components::PCIe_Switch* pcie_switch)
	{
		this->pcie_switch = pcie_switch;
	}

	LHA_type Host_Interface_Base::Get_max_logical_sector_address()
	{
		return max_logical_sector_address;
	}

	unsigned int Host_Interface_Base::Get_no_of_LHAs_in_an_NVM_write_unit()
	{
		return sectors_per_page;
	}

	Input_Stream_Manager_Base::Input_Stream_Manager_Base(Host_Interface_Base* host_interface) :
		host_interface(host_interface)
	{
	}

	void Input_Stream_Manager_Base::Update_transaction_statistics(NVM_Transaction* transaction)
	{
		switch (transaction->Type)
		{
			case Transaction_Type::READ:
				this->input_streams[transaction->Stream_id]->STAT_sum_of_read_transactions_execution_time += transaction->STAT_execution_time;
				this->input_streams[transaction->Stream_id]->STAT_sum_of_read_transactions_transfer_time += transaction->STAT_transfer_time;
				this->input_streams[transaction->Stream_id]->STAT_sum_of_read_transactions_waiting_time += (Simulator->Time() - transaction->Issue_time) - transaction->STAT_execution_time - transaction->STAT_transfer_time;
				break;
			case Transaction_Type::WRITE:
				this->input_streams[transaction->Stream_id]->STAT_sum_of_write_transactions_execution_time += transaction->STAT_execution_time;
				this->input_streams[transaction->Stream_id]->STAT_sum_of_write_transactions_transfer_time += transaction->STAT_transfer_time;
				this->input_streams[transaction->Stream_id]->STAT_sum_of_write_transactions_waiting_time += (Simulator->Time() - transaction->Issue_time) - transaction->STAT_execution_time - transaction->STAT_transfer_time;
				break;
			default:
				break;
		}
	}

	uint32_t Input_Stream_Manager_Base::Get_average_read_transaction_turnaround_time(stream_id_type stream_id)//in microseconds
	{
		if (input_streams[stream_id]->STAT_number_of_read_transactions == 0) {
			return 0;
		}
		return (uint32_t)((input_streams[stream_id]->STAT_sum_of_read_transactions_execution_time + input_streams[stream_id]->STAT_sum_of_read_transactions_transfer_time + input_streams[stream_id]->STAT_sum_of_read_transactions_waiting_time)
			/ input_streams[stream_id]->STAT_number_of_read_transactions / SIM_TIME_TO_MICROSECONDS_COEFF);
	}

	uint32_t Input_Stream_Manager_Base::Get_average_read_transaction_execution_time(stream_id_type stream_id)//in microseconds
	{
		if (input_streams[stream_id]->STAT_number_of_read_transactions == 0) {
			return 0;
		}
		return (uint32_t)(input_streams[stream_id]->STAT_sum_of_read_transactions_execution_time / input_streams[stream_id]->STAT_number_of_read_transactions / SIM_TIME_TO_MICROSECONDS_COEFF);
	}

	uint32_t Input_Stream_Manager_Base::Get_average_read_transaction_transfer_time(stream_id_type stream_id)//in microseconds
	{
		if (input_streams[stream_id]->STAT_number_of_read_transactions == 0) {
			return 0;
		}
		return (uint32_t)(input_streams[stream_id]->STAT_sum_of_read_transactions_transfer_time / input_streams[stream_id]->STAT_number_of_read_transactions / SIM_TIME_TO_MICROSECONDS_COEFF);
	}

	uint32_t Input_Stream_Manager_Base::Get_average_read_transaction_waiting_time(stream_id_type stream_id)//in microseconds
	{
		if (input_streams[stream_id]->STAT_number_of_read_transactions == 0) {
			return 0;
		}
		return (uint32_t)(input_streams[stream_id]->STAT_sum_of_read_transactions_waiting_time / input_streams[stream_id]->STAT_number_of_read_transactions / SIM_TIME_TO_MICROSECONDS_COEFF);
	}

	uint32_t Input_Stream_Manager_Base::Get_average_write_transaction_turnaround_time(stream_id_type stream_id)//in microseconds
	{
		if (input_streams[stream_id]->STAT_number_of_write_transactions == 0) {
			return 0;
		}
		return (uint32_t)((input_streams[stream_id]->STAT_sum_of_write_transactions_execution_time + input_streams[stream_id]->STAT_sum_of_write_transactions_transfer_time + input_streams[stream_id]->STAT_sum_of_write_transactions_waiting_time)
			/ input_streams[stream_id]->STAT_number_of_write_transactions / SIM_TIME_TO_MICROSECONDS_COEFF);
	}

	uint32_t Input_Stream_Manager_Base::Get_average_write_transaction_execution_time(stream_id_type stream_id)//in microseconds
	{
		if (input_streams[stream_id]->STAT_number_of_write_transactions == 0) {
			return 0;
		}
		return (uint32_t)(input_streams[stream_id]->STAT_sum_of_write_transactions_execution_time / input_streams[stream_id]->STAT_number_of_write_transactions / SIM_TIME_TO_MICROSECONDS_COEFF);
	}

	uint32_t Input_Stream_Manager_Base::Get_average_write_transaction_transfer_time(stream_id_type stream_id)//in microseconds
	{
		if (input_streams[stream_id]->STAT_number_of_write_transactions == 0) {
			return 0;
		}
		return (uint32_t)(input_streams[stream_id]->STAT_sum_of_write_transactions_transfer_time / input_streams[stream_id]->STAT_number_of_write_transactions / SIM_TIME_TO_MICROSECONDS_COEFF);
	}

	uint32_t Input_Stream_Manager_Base::Get_average_write_transaction_waiting_time(stream_id_type stream_id)//in microseconds
	{
		if (input_streams[stream_id]->STAT_number_of_write_transactions == 0) {
			return 0;
		}
		return (uint32_t)(input_streams[stream_id]->STAT_sum_of_write_transactions_waiting_time / input_streams[stream_id]->STAT_number_of_write_transactions / SIM_TIME_TO_MICROSECONDS_COEFF);
	}
	
	Request_Fetch_Unit_Base::Request_Fetch_Unit_Base(Host_Interface_Base* host_interface) :
		host_interface(host_interface)
	{
	}
}