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

	Host_Interface_Base* Host_Interface_Base::_my_instance = NULL;

	Host_Interface_Base::Host_Interface_Base(const sim_object_id_type& id, HostInterfaceType type, LSA_type max_logical_sector_address, unsigned int sectors_per_page, Data_Cache_Manager_Base* cache)
		: MQSimEngine::Sim_Object(id), type(type), max_logical_sector_address(max_logical_sector_address), sectors_per_page(sectors_per_page), cache(cache)
	{
		_my_instance = this;
	}

	void Host_Interface_Base::Setup_triggers()
	{
		Sim_Object::Setup_triggers();
		cache->Connect_to_user_request_serviced_signal(handle_user_request_serviced_signal_from_cache);
	}

	void Host_Interface_Base::Validate_simulation_config()
	{}


	Input_Stream_Manager_Base::Input_Stream_Manager_Base(Host_Interface_Base* host_interface) :
		host_interface(host_interface)
	{}
	

	Request_Fetch_Unit_Base::Request_Fetch_Unit_Base(Host_Interface_Base* host_interface) :
		host_interface(host_interface)
	{}

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

	LSA_type Host_Interface_Base::Get_max_logical_sector_address()
	{
		return max_logical_sector_address;
	}

	unsigned int Host_Interface_Base::Get_sector_no_per_NVM_write_unit()
	{
		return sectors_per_page;
	}
}