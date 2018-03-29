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
	struct DataCacheSlotType
	{
		unsigned long long State_bitmap_of_existing_sectors;
		LPA_type LPA;
		data_cache_content_type Content;
		data_timestamp_type Timestamp;
		bool Dirty;
		std::list<std::pair<LPA_type, DataCacheSlotType*>>::iterator lru_list_ptr;//used for fast implementation of LRU
	};

	class Cache
	{
	public:
		Cache(unsigned int capacity_in_pages = 0);
		bool Exists(const stream_id_type streamID, const LPA_type lpn);
		bool Check_free_slot_availability();
		bool Check_free_slot_availability(unsigned int no_of_slots);
		bool Empty();
		DataCacheSlotType Get_slot(const stream_id_type stream_id, const LPA_type lpn);
		DataCacheSlotType Evict_one_slot();
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
		unsigned int Size;
		void* Related_request;
		Data_Cache_Simulation_Event_Type next_event_type;
	};

	class Data_Cache_Manager_Flash : public Data_Cache_Manager_Base
	{
	public:
		Data_Cache_Manager_Flash(const sim_object_id_type& id, Host_Interface_Base* host_interface, NVM_Firmware* firmware, NVM_PHY_ONFI* flash_controller,
			unsigned int total_capacity_in_bytes,
			unsigned int dram_row_size, unsigned int dram_data_rate, unsigned int dram_busrt_size, sim_time_type dram_tRCD, sim_time_type dram_tCL, sim_time_type dram_tRP,
			Caching_Mode* caching_mode_per_input_stream, Cache_Sharing_Mode sharing_mode, unsigned int stream_count,
			unsigned int sector_no_per_page, unsigned int back_pressure_buffer_max_depth);
		void Execute_simulator_event(MQSimEngine::Sim_Event* ev);
		void Setup_triggers();
		void Make_warmup();
	private:
		NVM_PHY_ONFI * flash_controller;
		unsigned int capacity_in_bytes, capacity_in_pages;
		unsigned int sector_no_per_page;
		Cache** per_stream_cache;
		bool memory_channel_is_busy;
		
		void process_new_user_request(User_Request* user_request);
		void write_to_dram_cache(User_Request* user_request);
		std::queue<Memory_Transfer_Info*> dram_access_request_queue;//The list of DRAM transfers that are waiting to be executed
		std::queue<Memory_Transfer_Info*> waiting_access_request_queue;//The list of user writes that are waiting for the DRAM space to be free
		std::list<User_Request*> waiting_user_requests_queue;//The list of user requests that are waiting for the free DRAM space

		static void handle_transaction_serviced_signal_from_PHY(NVM_Transaction_Flash* transaction);
		void service_dram_access_request(Memory_Transfer_Info* request_info);
	};
}
#endif // !CACHE_MANAGER_H
