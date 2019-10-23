#ifndef DATA_CACHE_MANAGER_FLASH_SIMPLE_H
#define DATA_CACHE_MANAGER_FLASH_SIMPLE_H

#include <list>
#include <queue>
#include <unordered_map>
#include "../nvm_chip/flash_memory/FlashTypes.h"
#include "SSD_Defs.h"
#include "Data_Cache_Manager_Base.h"
#include "Data_Cache_Flash.h"
#include "NVM_Transaction_Flash.h"

namespace SSD_Components
{
	class Data_Cache_Manager_Flash_Simple : public Data_Cache_Manager_Base
	{
	public:
		Data_Cache_Manager_Flash_Simple(const sim_object_id_type& id, Host_Interface_Base* host_interface, NVM_Firmware* firmware, NVM_PHY_ONFI* flash_controller,
			unsigned int total_capacity_in_bytes,
			unsigned int dram_row_size, unsigned int dram_data_rate, unsigned int dram_busrt_size, sim_time_type dram_tRCD, sim_time_type dram_tCL, sim_time_type dram_tRP,
			Caching_Mode* caching_mode_per_input_stream, unsigned int stream_count, unsigned int sector_no_per_page, unsigned int back_pressure_buffer_max_depth);
		~Data_Cache_Manager_Flash_Simple();
		void Execute_simulator_event(MQSimEngine::Sim_Event* ev);
		void Setup_triggers();
		void Do_warmup(std::vector<Utils::Workload_Statistics*> workload_stats);
	private:
		NVM_PHY_ONFI * flash_controller;
		unsigned int capacity_in_bytes, capacity_in_pages;
		unsigned int sector_no_per_page;
		Data_Cache_Flash* data_cache;

		void process_new_user_request(User_Request* user_request);
		void write_to_destage_buffer(User_Request* user_request);//Used in the WRITE_CACHE and WRITE_READ_CACHE modes in which the DRAM space is used as a destage buffer
		std::queue<Memory_Transfer_Info*>* dram_execution_queue;//The list of DRAM transfers that are waiting to be executed
		std::list<User_Request*>* waiting_user_requests_queue_for_dram_free_slot;//The list of user requests that are waiting for free space in DRAM
		int request_queue_turn;
		unsigned int back_pressure_buffer_max_depth;
		unsigned int back_pressure_buffer_depth;
		std::set<LPA_type>* bloom_filter;
		sim_time_type bloom_filter_reset_step = 1000000000;
		sim_time_type next_bloom_filter_reset_milestone = 0;

		static void handle_transaction_serviced_signal_from_PHY(NVM_Transaction_Flash* transaction);
		void service_dram_access_request(Memory_Transfer_Info* request_info);
	};
}

#endif // !DATA_CACHE_MANAGER_FLASH_SIMPLE_H
