#ifndef DATA_CACHE_MANAGER_BASE_H
#define DATA_CACHE_MANAGER_BASE_H

#include <vector>
#include "../sim/Sim_Object.h"
#include "Host_Interface_Base.h"
#include "User_Request.h"
#include "NVM_Firmware.h"
#include "NVM_PHY_ONFI.h"
#include "../utils/Workload_Statistics.h"

namespace SSD_Components
{
	class NVM_Firmware;
	class Host_Interface_Base;
	enum class Caching_Mode {WRITE_CACHE, READ_CACHE, WRITE_READ_CACHE, TURNED_OFF};
	enum class Caching_Mechanism { SIMPLE, ADVANCED };
	//How the cache space is shared among the concurrently running I/O flows/streams
	enum class Cache_Sharing_Mode { SHARED,//each application has access to the entire cache space
		EQUAL_PARTITIONING}; 
	class Data_Cache_Manager_Base: public MQSimEngine::Sim_Object
	{
		friend class Data_Cache_Manager_Flash_Advanced;
		friend class Data_Cache_Manager_Flash_Simple;
	public:
		Data_Cache_Manager_Base(const sim_object_id_type& id, Host_Interface_Base* host_interface, NVM_Firmware* nvm_firmware,
			unsigned int dram_row_size, unsigned int dram_data_rate, unsigned int dram_busrt_size, sim_time_type dram_tRCD, sim_time_type dram_tCL, sim_time_type dram_tRP,
			Caching_Mode* caching_mode_per_input_stream, Cache_Sharing_Mode sharing_mode, unsigned int stream_count);
		virtual ~Data_Cache_Manager_Base();
		void Setup_triggers();
		void Start_simulation();
		void Validate_simulation_config();

		typedef void(*UserRequestServicedSignalHanderType) (User_Request*);
		void Connect_to_user_request_serviced_signal(UserRequestServicedSignalHanderType);
		typedef void(*MemoryTransactionServicedSignalHanderType) (NVM_Transaction*);
		void Connect_to_user_memory_transaction_serviced_signal(MemoryTransactionServicedSignalHanderType);
		void Set_host_interface(Host_Interface_Base* host_interface);
		virtual void Do_warmup(std::vector<Utils::Workload_Statistics*> workload_stats) = 0;
	protected:
		static Data_Cache_Manager_Base* _my_instance;
		Host_Interface_Base* host_interface;
		NVM_Firmware* nvm_firmware;
		unsigned int dram_row_size;//The size of the DRAM rows in bytes
		unsigned int dram_data_rate;//in MT/s
		unsigned int dram_busrt_size;
		double dram_burst_transfer_time_ddr;//The transfer time of two bursts, changed from sim_time_type to double to increase precision
		sim_time_type dram_tRCD, dram_tCL, dram_tRP;//DRAM access parameters in nano-seconds
		Cache_Sharing_Mode sharing_mode;
		static Caching_Mode* caching_mode_per_input_stream;
		unsigned int stream_count;

		std::vector<UserRequestServicedSignalHanderType> connected_user_request_serviced_signal_handlers;
		void broadcast_user_request_serviced_signal(User_Request* user_request);

		std::vector<MemoryTransactionServicedSignalHanderType> connected_user_memory_transaction_serviced_signal_handlers;
		void broadcast_user_memory_transaction_serviced_signal(NVM_Transaction* transaction);

		static void handle_user_request_arrived_signal(User_Request* user_request);
		virtual void process_new_user_request(User_Request* user_request) = 0;

		bool is_user_request_finished(const User_Request* user_request) { return (user_request->Transaction_list.size() == 0 && user_request->Sectors_serviced_from_cache == 0); }
	};


	inline sim_time_type estimate_dram_access_time(const unsigned int memory_access_size_in_byte,
		const unsigned int dram_row_size, const unsigned int dram_burst_size_in_bytes, const double dram_burst_transfer_time_ddr,
		const sim_time_type tRCD, const sim_time_type tCL, const sim_time_type tRP)
	{
		if (memory_access_size_in_byte <= dram_row_size) {
			return (sim_time_type)(tRCD + tCL + sim_time_type((double)(memory_access_size_in_byte / dram_burst_size_in_bytes / 2) * dram_burst_transfer_time_ddr));
		}
		else {
			return (sim_time_type)((tRCD + tCL + (sim_time_type)((double)(dram_row_size / dram_burst_size_in_bytes / 2 * dram_burst_transfer_time_ddr) + tRP) * (double)(memory_access_size_in_byte / dram_row_size / 2)) + tRCD + tCL + (sim_time_type)((double)(memory_access_size_in_byte % dram_row_size) / ((double)dram_burst_size_in_bytes * dram_burst_transfer_time_ddr)));
		}
	}
}

#endif // !DATA_CACHE_MANAGER_BASE_H
