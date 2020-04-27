#include <stdexcept>
#include "../nvm_chip/NVM_Types.h"
#include "Data_Cache_Manager_Flash_Simple.h"
#include "NVM_Transaction_Flash_RD.h"
#include "NVM_Transaction_Flash_WR.h"
#include "FTL.h"

namespace SSD_Components
{
	Data_Cache_Manager_Flash_Simple::Data_Cache_Manager_Flash_Simple(const sim_object_id_type& id, Host_Interface_Base* host_interface, NVM_Firmware* firmware, NVM_PHY_ONFI* flash_controller,
		unsigned int total_capacity_in_bytes,
		unsigned int dram_row_size, unsigned int dram_data_rate, unsigned int dram_busrt_size, sim_time_type dram_tRCD, sim_time_type dram_tCL, sim_time_type dram_tRP,
		Caching_Mode* caching_mode_per_input_stream, unsigned int stream_count, unsigned int sector_no_per_page, unsigned int back_pressure_buffer_max_depth)
		: Data_Cache_Manager_Base(id, host_interface, firmware, dram_row_size, dram_data_rate, dram_busrt_size, dram_tRCD, dram_tCL, dram_tRP, caching_mode_per_input_stream, Cache_Sharing_Mode::SHARED, stream_count),
		flash_controller(flash_controller), capacity_in_bytes(total_capacity_in_bytes), sector_no_per_page(sector_no_per_page),	request_queue_turn(0), back_pressure_buffer_max_depth(back_pressure_buffer_max_depth)
	{
		capacity_in_pages = capacity_in_bytes / (SECTOR_SIZE_IN_BYTE * sector_no_per_page);
		data_cache = new Data_Cache_Flash(capacity_in_pages);
		dram_execution_queue = new std::queue<Memory_Transfer_Info*>[stream_count];
		waiting_user_requests_queue_for_dram_free_slot = new std::list<User_Request*>[stream_count];
		this->back_pressure_buffer_depth = 0;
		bloom_filter = new std::set<LPA_type>[stream_count];
	}

	Data_Cache_Manager_Flash_Simple::~Data_Cache_Manager_Flash_Simple()
	{
		for (unsigned int i = 0; i < stream_count; i++) {
			while (dram_execution_queue[i].size()) {
				delete dram_execution_queue[i].front();
				dram_execution_queue[i].pop();
			}
			for (auto &req : waiting_user_requests_queue_for_dram_free_slot[i]) {
				delete req;
			}
		}

		delete data_cache;
		delete[] dram_execution_queue;
		delete[] waiting_user_requests_queue_for_dram_free_slot;
		delete[] bloom_filter;
	}

	void Data_Cache_Manager_Flash_Simple::Setup_triggers()
	{
		Data_Cache_Manager_Base::Setup_triggers();
		flash_controller->ConnectToTransactionServicedSignal(handle_transaction_serviced_signal_from_PHY);
	}

	void Data_Cache_Manager_Flash_Simple::Do_warmup(std::vector<Utils::Workload_Statistics*> workload_stats)
	{
	}

	void Data_Cache_Manager_Flash_Simple::process_new_user_request(User_Request* user_request)
	{
		//This condition shouldn't happen, but we check it
		if (user_request->Transaction_list.size() == 0) {
			return;
		}
		
		if (user_request->Type == UserRequestType::READ) {
			switch (caching_mode_per_input_stream[user_request->Stream_id])
			{
				case Caching_Mode::TURNED_OFF:
					static_cast<FTL*>(nvm_firmware)->Address_Mapping_Unit->Translate_lpa_to_ppa_and_dispatch(user_request->Transaction_list);
					return;
				case Caching_Mode::WRITE_CACHE:
				{
					std::list<NVM_Transaction*>::iterator it = user_request->Transaction_list.begin();
					while (it != user_request->Transaction_list.end()) {
						NVM_Transaction_Flash_RD* tr = (NVM_Transaction_Flash_RD*)(*it);
						if (data_cache->Exists(tr->Stream_id, tr->LPA)) {
							page_status_type available_sectors_bitmap = data_cache->Get_slot(tr->Stream_id, tr->LPA).State_bitmap_of_existing_sectors & tr->read_sectors_bitmap;
							if (available_sectors_bitmap == tr->read_sectors_bitmap) {
								user_request->Sectors_serviced_from_cache += count_sector_no_from_status_bitmap(tr->read_sectors_bitmap);
								user_request->Transaction_list.erase(it++);//the ++ operation should happen here, otherwise the iterator will be part of the list after erasing it from the list
							} else if (available_sectors_bitmap != 0) {
								user_request->Sectors_serviced_from_cache += count_sector_no_from_status_bitmap(available_sectors_bitmap);
								tr->read_sectors_bitmap = (tr->read_sectors_bitmap & ~available_sectors_bitmap);
								tr->Data_and_metadata_size_in_byte -= count_sector_no_from_status_bitmap(available_sectors_bitmap) * SECTOR_SIZE_IN_BYTE;
								it++;
							} else {
								it++;
							}
						} else {
							it++;
						}
					}
					if (user_request->Sectors_serviced_from_cache > 0) {
						Memory_Transfer_Info* transfer_info = new Memory_Transfer_Info;
						transfer_info->Size_in_bytes = user_request->Sectors_serviced_from_cache * SECTOR_SIZE_IN_BYTE;
						transfer_info->Related_request = user_request;
						transfer_info->next_event_type = Data_Cache_Simulation_Event_Type::MEMORY_READ_FOR_USERIO_FINISHED;
						transfer_info->Stream_id = user_request->Stream_id;
						service_dram_access_request(transfer_info);
					}
					if (user_request->Transaction_list.size() > 0) {
						static_cast<FTL*>(nvm_firmware)->Address_Mapping_Unit->Translate_lpa_to_ppa_and_dispatch(user_request->Transaction_list);
					}
				}
				default:
					PRINT_ERROR("The specified caching mode is not not support in simple cache manager!")
			}
		} else {//This is a write request
			switch (caching_mode_per_input_stream[user_request->Stream_id])
			{
				case Caching_Mode::TURNED_OFF:
					static_cast<FTL*>(nvm_firmware)->Address_Mapping_Unit->Translate_lpa_to_ppa_and_dispatch(user_request->Transaction_list);
					return;
				case Caching_Mode::WRITE_CACHE://The data cache manger unit performs like a destage buffer
				{
					write_to_destage_buffer(user_request);

					if (user_request->Transaction_list.size() > 0) {
						waiting_user_requests_queue_for_dram_free_slot[user_request->Stream_id].push_back(user_request);
					}
				}
				default:
					PRINT_ERROR("The specified caching mode is not not support in simple cache manager!")
			}
		}
	}

	void Data_Cache_Manager_Flash_Simple::write_to_destage_buffer(User_Request* user_request)
	{
		//To eliminate race condition, MQSim assumes the management information and user data are stored in separate DRAM modules
		unsigned int cache_eviction_read_size_in_sectors = 0;//The size of data evicted from cache
		unsigned int flash_written_back_write_size_in_sectors = 0;//The size of data that is both written back to flash and written to DRAM
		unsigned int dram_write_size_in_sectors = 0;//The size of data written to DRAM (must be >= flash_written_back_write_size_in_sectors)
		std::list<NVM_Transaction*>* evicted_cache_slots = new std::list<NVM_Transaction*>;
		std::list<NVM_Transaction*> writeback_transactions;
		auto it = user_request->Transaction_list.begin();

		while (it != user_request->Transaction_list.end()
			&& (back_pressure_buffer_depth + cache_eviction_read_size_in_sectors + flash_written_back_write_size_in_sectors) < back_pressure_buffer_max_depth) {
			NVM_Transaction_Flash_WR* tr = (NVM_Transaction_Flash_WR*)(*it);
			if (data_cache->Exists(tr->Stream_id, tr->LPA))//If the logical address already exists in the cache
			{
				/*MQSim should get rid of writting stale data to the cache.
				* This situation may result from out-of-order transaction execution*/
				Data_Cache_Slot_Type slot = data_cache->Get_slot(tr->Stream_id, tr->LPA);
				sim_time_type timestamp = slot.Timestamp;
				NVM::memory_content_type content = slot.Content;
				if (tr->DataTimeStamp > timestamp) {
					timestamp = tr->DataTimeStamp;
					content = tr->Content;
				}
				data_cache->Update_data(tr->Stream_id, tr->LPA, content, timestamp, tr->write_sectors_bitmap | slot.State_bitmap_of_existing_sectors);
			} else { //the logical address is not in the cache
				if (!data_cache->Check_free_slot_availability()) {
					Data_Cache_Slot_Type evicted_slot = data_cache->Evict_one_slot_lru();
					if (evicted_slot.Status == Cache_Slot_Status::DIRTY_NO_FLASH_WRITEBACK) {
						evicted_cache_slots->push_back(new NVM_Transaction_Flash_WR(Transaction_Source_Type::CACHE,
							tr->Stream_id, count_sector_no_from_status_bitmap(evicted_slot.State_bitmap_of_existing_sectors) * SECTOR_SIZE_IN_BYTE,
							evicted_slot.LPA, NULL, IO_Flow_Priority_Class::URGENT, evicted_slot.Content, evicted_slot.State_bitmap_of_existing_sectors, evicted_slot.Timestamp));
						cache_eviction_read_size_in_sectors += count_sector_no_from_status_bitmap(evicted_slot.State_bitmap_of_existing_sectors);
						//DEBUG2("Evicting page" << evicted_slot.LPA << " from write buffer ")
					}
				}
				data_cache->Insert_write_data(tr->Stream_id, tr->LPA, tr->Content, tr->DataTimeStamp, tr->write_sectors_bitmap);
			}
			dram_write_size_in_sectors += count_sector_no_from_status_bitmap(tr->write_sectors_bitmap);

			//hot/cold data separation
			if (bloom_filter[0].find(tr->LPA) == bloom_filter[0].end()) {
				data_cache->Change_slot_status_to_writeback(tr->Stream_id, tr->LPA); //Eagerly write back cold data
				flash_written_back_write_size_in_sectors += count_sector_no_from_status_bitmap(tr->write_sectors_bitmap);
				bloom_filter[0].insert(tr->LPA);
				writeback_transactions.push_back(tr);
			}
			user_request->Transaction_list.erase(it++);
		}

		user_request->Sectors_serviced_from_cache += dram_write_size_in_sectors;//This is very important update. It is used to decide when all data sectors of a user request are serviced
		back_pressure_buffer_depth += cache_eviction_read_size_in_sectors + flash_written_back_write_size_in_sectors;

		//Issue memory read for cache evictions
		if (evicted_cache_slots->size() > 0) {
			Memory_Transfer_Info* read_transfer_info = new Memory_Transfer_Info;
			read_transfer_info->Size_in_bytes = cache_eviction_read_size_in_sectors * SECTOR_SIZE_IN_BYTE;
			read_transfer_info->Related_request = evicted_cache_slots;
			read_transfer_info->next_event_type = Data_Cache_Simulation_Event_Type::MEMORY_READ_FOR_CACHE_EVICTION_FINISHED;
			read_transfer_info->Stream_id = user_request->Stream_id;
			service_dram_access_request(read_transfer_info);
		}

		//Issue memory write to write data to DRAM
		if (dram_write_size_in_sectors) {
			Memory_Transfer_Info* write_transfer_info = new Memory_Transfer_Info;
			write_transfer_info->Size_in_bytes = dram_write_size_in_sectors * SECTOR_SIZE_IN_BYTE;
			write_transfer_info->Related_request = user_request;
			write_transfer_info->next_event_type = Data_Cache_Simulation_Event_Type::MEMORY_WRITE_FOR_USERIO_FINISHED;
			write_transfer_info->Stream_id = user_request->Stream_id;
			service_dram_access_request(write_transfer_info);
		}

		//If any writeback should be performed, then issue flash write transactions
		if (writeback_transactions.size() > 0) {
			static_cast<FTL*>(nvm_firmware)->Address_Mapping_Unit->Translate_lpa_to_ppa_and_dispatch(writeback_transactions);
		}

		//Reset control data structures used for hot/cold separation 
		if (Simulator->Time() > next_bloom_filter_reset_milestone) {
			bloom_filter[0].clear();
			next_bloom_filter_reset_milestone = Simulator->Time() + bloom_filter_reset_step;
		}
	}

	void Data_Cache_Manager_Flash_Simple::handle_transaction_serviced_signal_from_PHY(NVM_Transaction_Flash* transaction)
	{
		//First check if the transaction source is a user request or the cache itself
		if (transaction->Source != Transaction_Source_Type::USERIO && transaction->Source != Transaction_Source_Type::CACHE) {
			return;
		}

		if (transaction->Source == Transaction_Source_Type::USERIO)
			_my_instance->broadcast_user_memory_transaction_serviced_signal(transaction);
		/* This is an update read (a read that is generated for a write request that partially updates page data).
		*  An update read transaction is issued in Address Mapping Unit, but is consumed in data cache manager.*/
		if (transaction->Type == Transaction_Type::READ) {
			if (((NVM_Transaction_Flash_RD*)transaction)->RelatedWrite != NULL) {
				((NVM_Transaction_Flash_RD*)transaction)->RelatedWrite->RelatedRead = NULL;
				return;
			}
			switch (Data_Cache_Manager_Flash_Simple::caching_mode_per_input_stream[transaction->Stream_id])
			{
				case Caching_Mode::TURNED_OFF:
				case Caching_Mode::WRITE_CACHE:
					transaction->UserIORequest->Transaction_list.remove(transaction);
					if (_my_instance->is_user_request_finished(transaction->UserIORequest)) {
						_my_instance->broadcast_user_request_serviced_signal(transaction->UserIORequest);
					}
					break;
				default:
					PRINT_ERROR("The specified caching mode is not not support in simple cache manager!")
			}
		} else { //This is a write request
			switch (Data_Cache_Manager_Flash_Simple::caching_mode_per_input_stream[transaction->Stream_id])
			{
				case Caching_Mode::TURNED_OFF:
					transaction->UserIORequest->Transaction_list.remove(transaction);
					if (_my_instance->is_user_request_finished(transaction->UserIORequest)) {
						_my_instance->broadcast_user_request_serviced_signal(transaction->UserIORequest);
					}
					break;
				case Caching_Mode::WRITE_CACHE:
				{
					((Data_Cache_Manager_Flash_Simple*)_my_instance)->back_pressure_buffer_depth -= transaction->Data_and_metadata_size_in_byte / SECTOR_SIZE_IN_BYTE + (transaction->Data_and_metadata_size_in_byte % SECTOR_SIZE_IN_BYTE == 0 ? 0 : 1);

					if (((Data_Cache_Manager_Flash_Simple*)_my_instance)->data_cache->Exists(transaction->Stream_id, ((NVM_Transaction_Flash_WR*)transaction)->LPA)) {
						Data_Cache_Slot_Type slot = ((Data_Cache_Manager_Flash_Simple*)_my_instance)->data_cache->Get_slot(transaction->Stream_id, ((NVM_Transaction_Flash_WR*)transaction)->LPA);
						sim_time_type timestamp = slot.Timestamp;
						NVM::memory_content_type content = slot.Content;
						if (((NVM_Transaction_Flash_WR*)transaction)->DataTimeStamp >= timestamp) {
							((Data_Cache_Manager_Flash_Simple*)_my_instance)->data_cache->Remove_slot(transaction->Stream_id, ((NVM_Transaction_Flash_WR*)transaction)->LPA);
						}
					}

					for (unsigned int i = 0; i < _my_instance->stream_count; i++) {
						((Data_Cache_Manager_Flash_Simple*)_my_instance)->request_queue_turn++;
						((Data_Cache_Manager_Flash_Simple*)_my_instance)->request_queue_turn %= ((Data_Cache_Manager_Flash_Simple*)_my_instance)->stream_count;
						if (((Data_Cache_Manager_Flash_Simple*)_my_instance)->waiting_user_requests_queue_for_dram_free_slot[((Data_Cache_Manager_Flash_Simple*)_my_instance)->request_queue_turn].size() > 0) {
							auto user_request = ((Data_Cache_Manager_Flash_Simple*)_my_instance)->waiting_user_requests_queue_for_dram_free_slot[((Data_Cache_Manager_Flash_Simple*)_my_instance)->request_queue_turn].begin();
							((Data_Cache_Manager_Flash_Simple*)_my_instance)->write_to_destage_buffer(*user_request);
							if ((*user_request)->Transaction_list.size() == 0) {
								((Data_Cache_Manager_Flash_Simple*)_my_instance)->waiting_user_requests_queue_for_dram_free_slot[((Data_Cache_Manager_Flash_Simple*)_my_instance)->request_queue_turn].remove(*user_request);
							}
							//The traffic load on the backend is high and the waiting requests cannot be serviced
							if (((Data_Cache_Manager_Flash_Simple*)_my_instance)->back_pressure_buffer_depth >= ((Data_Cache_Manager_Flash_Simple*)_my_instance)->back_pressure_buffer_max_depth) {
								break;
							}
						}
					}
					break;
				}
				default:
					PRINT_ERROR("The specified caching mode is not not support in simple cache manager!")
			}
		}
	}

	void Data_Cache_Manager_Flash_Simple::service_dram_access_request(Memory_Transfer_Info* request_info)
	{
		dram_execution_queue[request_info->Stream_id].push(request_info);
		if(dram_execution_queue[request_info->Stream_id].size() == 1) {
			Simulator->Register_sim_event(Simulator->Time() + estimate_dram_access_time(request_info->Size_in_bytes, dram_row_size,
				dram_busrt_size, dram_burst_transfer_time_ddr, dram_tRCD, dram_tCL, dram_tRP),
				this, request_info, static_cast<int>(request_info->next_event_type));
		}
	}

	void Data_Cache_Manager_Flash_Simple::Execute_simulator_event(MQSimEngine::Sim_Event* ev)
	{
		Data_Cache_Simulation_Event_Type eventType = (Data_Cache_Simulation_Event_Type)ev->Type;
		Memory_Transfer_Info* transfer_inf = (Memory_Transfer_Info*)ev->Parameters;

		switch (eventType)
			{
			case Data_Cache_Simulation_Event_Type::MEMORY_READ_FOR_USERIO_FINISHED://A user read is service from DRAM cache
			case Data_Cache_Simulation_Event_Type::MEMORY_WRITE_FOR_USERIO_FINISHED:
				((User_Request*)(transfer_inf)->Related_request)->Sectors_serviced_from_cache -= transfer_inf->Size_in_bytes / SECTOR_SIZE_IN_BYTE;
				if (is_user_request_finished((User_Request*)(transfer_inf)->Related_request)) {
					broadcast_user_request_serviced_signal(((User_Request*)(transfer_inf)->Related_request));
				}
				break;
			case Data_Cache_Simulation_Event_Type::MEMORY_READ_FOR_CACHE_EVICTION_FINISHED://Reading data from DRAM and writing it back to the flash storage
				static_cast<FTL*>(nvm_firmware)->Address_Mapping_Unit->Translate_lpa_to_ppa_and_dispatch(*((std::list<NVM_Transaction*>*)(transfer_inf->Related_request)));
				delete (std::list<NVM_Transaction*>*)transfer_inf->Related_request;
				break;
			case Data_Cache_Simulation_Event_Type::MEMORY_WRITE_FOR_CACHE_FINISHED://The recently read data from flash is written back to memory to support future user read requests
				break;
		}

		dram_execution_queue[transfer_inf->Stream_id].pop();
		if (dram_execution_queue[transfer_inf->Stream_id].size() > 0) {
			Memory_Transfer_Info* new_transfer_info = dram_execution_queue[transfer_inf->Stream_id].front();
			Simulator->Register_sim_event(Simulator->Time() + estimate_dram_access_time(new_transfer_info->Size_in_bytes, dram_row_size, dram_busrt_size,
				dram_burst_transfer_time_ddr, dram_tRCD, dram_tCL, dram_tRP),
				this, new_transfer_info, static_cast<int>(new_transfer_info->next_event_type));
		}

		delete transfer_inf;
	}
}
