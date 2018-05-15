#ifndef DATA_CACHE_MANAGER_H
#define DATA_CACHE_MANAGER_H

#include <list>
#include <queue>
#include <unordered_map>
#include "../nvm_chip/flash_memory/FlashTypes.h"
#include "SSD_Defs.h"
#include "Data_Cache_Manager_Base.h"
#include "NVM_Transaction_Flash.h"

namespace SSD_Components
{
	enum class CacheSlotStatus {EMPTY, CLEAN, DIRTY_NO_FLASH_WRITEBACK, DIRTY_FLASH_WRITEBACK};
	struct DataCacheSlotType
	{
		unsigned long long State_bitmap_of_existing_sectors;
		LPA_type LPA;
		data_cache_content_type Content;
		data_timestamp_type Timestamp;
		CacheSlotStatus Status;
		std::list<std::pair<LPA_type, DataCacheSlotType*>>::iterator lru_list_ptr;//used for fast implementation of LRU
	};

	class Cache
	{
	public:
		Cache(unsigned int capacity_in_pages = 0);
		~Cache();
		bool Exists(const stream_id_type streamID, const LPA_type lpn);
		bool Check_free_slot_availability();
		bool Check_free_slot_availability(unsigned int no_of_slots);
		bool Empty();
		bool Full();
		DataCacheSlotType Get_slot(const stream_id_type stream_id, const LPA_type lpn);
		DataCacheSlotType Evict_one_dirty_slot();
		DataCacheSlotType Evict_one_slot_lru();
		void Change_slot_status_to_writeback(const stream_id_type stream_id, const LPA_type lpn);
		void Remove_slot(const stream_id_type stream_id, const LPA_type lpn);
		void Insert_read_data(const stream_id_type stream_id, const LPA_type lpn, const data_cache_content_type content, const data_timestamp_type timestamp, const page_status_type state_bitmap_of_read_sectors);
		void Insert_write_data(const stream_id_type stream_id, const LPA_type lpn, const data_cache_content_type content, const data_timestamp_type timestamp, const page_status_type state_bitmap_of_write_sectors);
		void Update_data(const stream_id_type stream_id, const LPA_type lpn, const data_cache_content_type content, const data_timestamp_type timestamp, const page_status_type state_bitmap_of_write_sectors);
	private:
		std::unordered_map<LPA_type, DataCacheSlotType*> slots;
		std::list<std::pair<LPA_type, DataCacheSlotType*>> lru_list;
		unsigned int capacity_in_pages;
	};

	enum class Data_Cache_Simulation_Event_Type {MEMORY_READ_FOR_CACHE_FINISHED,
		MEMORY_WRITE_FOR_CACHE_FINISHED,
		MEMORY_READ_FOR_USERIO_FINISHED,
		MEMORY_WRITE_FOR_USERIO_FINISHED};
	struct Memory_Transfer_Info
	{
		unsigned int Size_in_bytes;
		void* Related_request;
		Data_Cache_Simulation_Event_Type next_event_type;
		stream_id_type Stream_id;
	};

	/*
	Assumed hardware structure:
			dram_free_slot_waiting_queue (a write transfer request enqueued here if DRAM is full. for reads, there is no need for DRAM queue)
					 |
					 |
					\|/
			dram_execution_queue (the transfer request goes here if DRAM can service it but the memory channel is busy)
					 |
					 |
					\|/
			 ---------------------------------------------------------
			|     DRAM Main Data Space     DRAM-back_pressure_buffer  | ---------->To the flash backend
			 ---------------------------------------------------------
	*/
	class Data_Cache_Manager_Flash : public Data_Cache_Manager_Base
	{
	public:
		Data_Cache_Manager_Flash(const sim_object_id_type& id, Host_Interface_Base* host_interface, NVM_Firmware* firmware, NVM_PHY_ONFI* flash_controller,
			unsigned int total_capacity_in_bytes,
			unsigned int dram_row_size, unsigned int dram_data_rate, unsigned int dram_busrt_size, sim_time_type dram_tRCD, sim_time_type dram_tCL, sim_time_type dram_tRP,
			Caching_Mode* caching_mode_per_input_stream, Cache_Sharing_Mode sharing_mode, 
			unsigned int stream_count, unsigned int sector_no_per_page, unsigned int back_pressure_buffer_max_depth, bool shared_dram_request_queue = false);
		~Data_Cache_Manager_Flash();
		void Execute_simulator_event(MQSimEngine::Sim_Event* ev);
		void Setup_triggers();
		void Do_warmup(std::vector<Utils::Workload_Statistics*> workload_stats);
	private:
		NVM_PHY_ONFI * flash_controller;
		unsigned int capacity_in_bytes, capacity_in_pages;
		unsigned int sector_no_per_page;
		Cache** per_stream_cache;
		bool memory_channel_is_busy;
		
		void process_new_user_request(User_Request* user_request);
		std::queue<Memory_Transfer_Info*>* dram_execution_queue;//The list of DRAM transfers that are waiting to be executed
		std::list<Memory_Transfer_Info*>* dram_free_slot_waiting_queue;//The list of DRAM transfers that are waiting for free space in DRAM and are not in the execution list
		//std::list<User_Request*>* waiting_user_requests_queue;//The list of user requests that are waiting for slots in the DRAM space
		bool shared_dram_request_queue;
		int dram_execution_list_turn;
		std::set<LPA_type>* bloom_filter;
		sim_time_type bloom_filter_reset_step = 1000000000;
		sim_time_type next_bloom_filter_reset_milestone = 0;

		static void handle_transaction_serviced_signal_from_PHY(NVM_Transaction_Flash* transaction);
		void service_dram_access_request(Memory_Transfer_Info* request_info);
	};
}
#endif // !CACHE_MANAGER_H
