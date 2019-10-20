#include "Data_Cache_Manager_Base.h"
#include "FTL.h"

namespace SSD_Components
{
	Data_Cache_Manager_Base* Data_Cache_Manager_Base::_my_instance = NULL;
	Caching_Mode* Data_Cache_Manager_Base::caching_mode_per_input_stream;

	Data_Cache_Manager_Base::Data_Cache_Manager_Base(const sim_object_id_type& id, Host_Interface_Base* host_interface, NVM_Firmware* nvm_firmware,
		unsigned int dram_row_size, unsigned int dram_data_rate, unsigned int dram_busrt_size, sim_time_type dram_tRCD, sim_time_type dram_tCL, sim_time_type dram_tRP,
		Caching_Mode* caching_mode_per_input_stream, Cache_Sharing_Mode sharing_mode, unsigned int stream_count)
		: MQSimEngine::Sim_Object(id), host_interface(host_interface), nvm_firmware(nvm_firmware),
		dram_row_size(dram_row_size), dram_data_rate(dram_data_rate), dram_busrt_size(dram_busrt_size), dram_tRCD(dram_tRCD), dram_tCL(dram_tCL), dram_tRP(dram_tRP),
		sharing_mode(sharing_mode), stream_count(stream_count)
	{
		_my_instance = this;
		dram_burst_transfer_time_ddr = (double) ONE_SECOND / (dram_data_rate * 1000 * 1000);
		this->caching_mode_per_input_stream = new Caching_Mode[stream_count];
		for (unsigned int i = 0; i < stream_count; i++) {
			this->caching_mode_per_input_stream[i] = caching_mode_per_input_stream[i];
		}
	}

	Data_Cache_Manager_Base::~Data_Cache_Manager_Base() {}

	void Data_Cache_Manager_Base::Setup_triggers()
	{
		Sim_Object::Setup_triggers();
		host_interface->Connect_to_user_request_arrived_signal(handle_user_request_arrived_signal);
	}

	void Data_Cache_Manager_Base::Start_simulation() {}
	
	void Data_Cache_Manager_Base::Validate_simulation_config() {}

	void Data_Cache_Manager_Base::Connect_to_user_request_serviced_signal(UserRequestServicedSignalHanderType function)
	{
		connected_user_request_serviced_signal_handlers.push_back(function);
	}
	
	void Data_Cache_Manager_Base::broadcast_user_request_serviced_signal(User_Request* nvm_transaction)
	{
		for (std::vector<UserRequestServicedSignalHanderType>::iterator it = connected_user_request_serviced_signal_handlers.begin();
			it != connected_user_request_serviced_signal_handlers.end(); it++) {
			(*it)(nvm_transaction);
		}
	}

	void Data_Cache_Manager_Base::Connect_to_user_memory_transaction_serviced_signal(MemoryTransactionServicedSignalHanderType function)
	{
		connected_user_memory_transaction_serviced_signal_handlers.push_back(function);
	}

	void Data_Cache_Manager_Base::broadcast_user_memory_transaction_serviced_signal(NVM_Transaction* transaction)
	{
		for (std::vector<MemoryTransactionServicedSignalHanderType>::iterator it = connected_user_memory_transaction_serviced_signal_handlers.begin();
			it != connected_user_memory_transaction_serviced_signal_handlers.end(); it++) {
			(*it)(transaction);
		}
	}

	void Data_Cache_Manager_Base::handle_user_request_arrived_signal(User_Request* user_request)
	{
		_my_instance->process_new_user_request(user_request);
	}

	void Data_Cache_Manager_Base::Set_host_interface(Host_Interface_Base* host_interface)
	{
		this->host_interface = host_interface;
	}
}
