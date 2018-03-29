#include <assert.h>
#include <stdexcept>
#include "../nvm_chip/NVM_Types.h"
#include "Data_Cache_Manager_Flash.h"
#include "NVM_Transaction_Flash_RD.h"
#include "NVM_Transaction_Flash_WR.h"
#include "FTL.h"

namespace SSD_Components
{
	Cache::Cache(unsigned int capacity_in_pages) : capacity_in_pages(capacity_in_pages) {}
	inline bool Cache::Exists(const stream_id_type stream_id, const LPA_type lpn)
	{
		LPA_type key = LPN_TO_UNIQUE_KEY(stream_id, lpn);
		auto it = slots.find(key);
		if (it == slots.end())
			return false;
		return true;
	}
	DataCacheSlotType Cache::Get_slot(const stream_id_type stream_id, const LPA_type lpn)
	{
		LPA_type key = LPN_TO_UNIQUE_KEY(stream_id, lpn);
		auto it = slots.find(key);
		assert(it != slots.end());
		if (lru_list.begin()->first != key)
			lru_list.splice(lru_list.begin(), lru_list, it->second->lru_list_ptr);
		return *(it->second);
	}
	inline bool Cache::Check_free_slot_availability()
	{
		return slots.size() < capacity_in_pages;
	}
	inline bool Cache::Check_free_slot_availability(unsigned int no_of_slots)
	{
		return slots.size() + no_of_slots <= capacity_in_pages;
	}
	inline bool Cache::Empty()
	{
		return slots.size() == 0;
	}
	DataCacheSlotType Cache::Evict_one_slot()
	{
		assert(slots.size() > 0);
		slots.erase(lru_list.back().first);
		DataCacheSlotType evicted_item = *lru_list.back().second;
		delete lru_list.back().second;
		lru_list.pop_back();
		return evicted_item;
	}
	void Cache::Insert_read_data(const stream_id_type stream_id, const LPA_type lpn, const data_cache_content_type content,
		const data_timestamp_type timestamp, const page_status_type state_bitmap_of_read_sectors)
	{
		LPA_type key = LPN_TO_UNIQUE_KEY(stream_id, lpn);
		if (slots.find(key) != slots.end())
			throw std::logic_error("Duplicate lpn insertion into data cache!");
		if (slots.size() >= capacity_in_pages)
			throw std::logic_error("Data cache overfull!");

		DataCacheSlotType* cache_slot = new DataCacheSlotType();
		cache_slot->LPA = lpn;
		cache_slot->State_bitmap_of_existing_sectors = state_bitmap_of_read_sectors;
		cache_slot->Content = content;
		cache_slot->Timestamp = timestamp;
		cache_slot->Dirty = false;
		lru_list.push_front(std::pair<LPA_type, DataCacheSlotType*>(key, cache_slot));
		cache_slot->lru_list_ptr = lru_list.begin();
		slots[key] = cache_slot;
	}
	void Cache::Insert_write_data(const stream_id_type stream_id, const LPA_type lpn, const data_cache_content_type content,
		const data_timestamp_type timestamp, const page_status_type state_bitmap_of_write_sectors)
	{
		LPA_type key = LPN_TO_UNIQUE_KEY(stream_id, lpn);
		if (slots.find(key) != slots.end())
			throw std::logic_error("Duplicate lpn insertion into data cache!!");
		if (slots.size() >= capacity_in_pages)
			throw std::logic_error("Data cache overfull!");

		DataCacheSlotType* cache_slot = new DataCacheSlotType();
		cache_slot->LPA = lpn;
		cache_slot->State_bitmap_of_existing_sectors = state_bitmap_of_write_sectors;
		cache_slot->Content = content;
		cache_slot->Timestamp = timestamp;
		cache_slot->Dirty = true;
		lru_list.push_front(std::pair<LPA_type, DataCacheSlotType*>(key, cache_slot));
		cache_slot->lru_list_ptr = lru_list.begin();
		slots[key] = cache_slot;
	}
	void Cache::Update_data(const stream_id_type stream_id, const LPA_type lpn, const data_cache_content_type content,
		const data_timestamp_type timestamp, const page_status_type state_bitmap_of_write_sectors)
	{
		LPA_type key = LPN_TO_UNIQUE_KEY(stream_id, lpn);
		auto it = slots.find(key);
		assert(it != slots.end());

		it->second->LPA = lpn;
		it->second->State_bitmap_of_existing_sectors = state_bitmap_of_write_sectors;
		it->second->Content = content;
		it->second->Timestamp = timestamp;
		it->second->Dirty = true;
		if (lru_list.begin()->first != key)
			lru_list.splice(lru_list.begin(), lru_list, it->second->lru_list_ptr);
	}
	void Cache::Remove_slot(const stream_id_type stream_id, const LPA_type lpn)
	{
		LPA_type key = LPN_TO_UNIQUE_KEY(stream_id, lpn);
		auto it = slots.find(key);
		assert(it != slots.end());
		lru_list.erase(it->second->lru_list_ptr);
		delete it->second;
		slots.erase(it);
	}

	Data_Cache_Manager_Flash::Data_Cache_Manager_Flash(const sim_object_id_type& id, Host_Interface_Base* host_interface, NVM_Firmware* firmware, NVM_PHY_ONFI* flash_controller,
		unsigned int total_capacity_in_bytes,
		unsigned int dram_row_size, unsigned int dram_data_rate, unsigned int dram_busrt_size, sim_time_type dram_tRCD, sim_time_type dram_tCL, sim_time_type dram_tRP,
		Caching_Mode* caching_mode_per_input_stream, Cache_Sharing_Mode sharing_mode, unsigned int stream_count,
		unsigned int sector_no_per_page, unsigned int back_pressure_buffer_max_depth)
		: Data_Cache_Manager_Base(id, host_interface, firmware, dram_row_size, dram_data_rate, dram_busrt_size, dram_tRCD, dram_tCL, dram_tRP, caching_mode_per_input_stream, sharing_mode, stream_count, back_pressure_buffer_max_depth),
		flash_controller(flash_controller), capacity_in_bytes(total_capacity_in_bytes), sector_no_per_page(sector_no_per_page),	memory_channel_is_busy(false)
	{
		capacity_in_pages = capacity_in_bytes / (SECTOR_SIZE_IN_BYTE * sector_no_per_page);
		switch (sharing_mode)
		{
		case SSD_Components::Cache_Sharing_Mode::SHARED:
		{
			Cache* sharedCache = new Cache(capacity_in_pages);
			per_stream_cache = new Cache*[stream_count];
			for (unsigned int i = 0; i < stream_count; i++)
				per_stream_cache[i] = sharedCache;
			break; 
		}
		case SSD_Components::Cache_Sharing_Mode::EQUAL_PARTITIONING:
			per_stream_cache = new Cache*[stream_count];
			for (unsigned int i = 0; i < stream_count; i++)
				per_stream_cache[i] = new Cache(capacity_in_pages / stream_count);
			break;
		default:
			break;
		}
	}

	void Data_Cache_Manager_Flash::Setup_triggers()
	{
		Data_Cache_Manager_Base::Setup_triggers();
		flash_controller->ConnectToTransactionServicedSignal(handle_transaction_serviced_signal_from_PHY);
	}

	void Data_Cache_Manager_Flash::Make_warmup()
	{}

	void Data_Cache_Manager_Flash::write_to_dram_cache(User_Request* user_request)
	{
		for (std::list<NVM_Transaction*>::iterator tr = user_request->Transaction_list.begin();
			tr != user_request->Transaction_list.end(); tr++)
		{
			if (per_stream_cache[(*tr)->Stream_id]->Exists((*tr)->Stream_id, ((NVM_Transaction_Flash_WR*)(*tr))->LPA))
			{
				/*MQSim should get rid of writting stale data to the cache.
				* This situation may result from out-of-order transaction execution*/
				DataCacheSlotType slot = per_stream_cache[(*tr)->Stream_id]->Get_slot((*tr)->Stream_id, ((NVM_Transaction_Flash_WR*)(*tr))->LPA);
				sim_time_type timestamp = slot.Timestamp;
				NVM::memory_content_type content = slot.Content;
				if (((NVM_Transaction_Flash_WR*)(*tr))->DataTimeStamp > timestamp)
				{
					timestamp = ((NVM_Transaction_Flash_WR*)(*tr))->DataTimeStamp;
					content = ((NVM_Transaction_Flash_WR*)(*tr))->Content;
					per_stream_cache[(*tr)->Stream_id]->Update_data((*tr)->Stream_id, ((NVM_Transaction_Flash_WR*)(*tr))->LPA,
						content, timestamp, ((NVM_Transaction_Flash_WR*)(*tr))->write_sectors_bitmap | slot.State_bitmap_of_existing_sectors);
				}
				else//escape writing stale data
				{
					user_request->Sectors_serviced_from_cache -= count_sector_no_from_status_bitmap(((NVM_Transaction_Flash_WR*)(*tr))->write_sectors_bitmap);
				}
			}
			else
			{
				per_stream_cache[user_request->Stream_id]->Insert_write_data((*tr)->Stream_id,
					((NVM_Transaction_Flash_WR*)(*tr))->LPA, ((NVM_Transaction_Flash_WR*)(*tr))->Content,
					((NVM_Transaction_Flash_WR*)(*tr))->DataTimeStamp, ((NVM_Transaction_Flash_WR*)(*tr))->write_sectors_bitmap);
			}
		}

		Memory_Transfer_Info* write_transfer_info = new Memory_Transfer_Info;
		write_transfer_info->Size = user_request->Sectors_serviced_from_cache * SECTOR_SIZE_IN_BYTE;
		write_transfer_info->Related_request = user_request;
		write_transfer_info->next_event_type = Data_Cache_Simulation_Event_Type::MEMORY_WRITE_FOR_USERIO_FINISHED;
		service_dram_access_request(write_transfer_info);

		if (back_pressure_buffer_depth < back_pressure_buffer_max_depth)
		{
			back_pressure_buffer_depth += user_request->Sectors_serviced_from_cache;
			static_cast<FTL*>(nvm_firmware)->Address_Mapping_Unit->Translate_lpa_to_ppa_and_dispatch(user_request->Transaction_list);
		}
		user_request->Transaction_list.clear();//All flash transactions are serviced from DRAM
	}


	void Data_Cache_Manager_Flash::process_new_user_request(User_Request* user_request)
	{
		if (user_request->Transaction_list.size() == 0)//This condition shouldn't happen, but we check it
			return;

		if (user_request->Type == UserRequestType::READ)
		{
			switch (caching_mode_per_input_stream[user_request->Stream_id])
			{
			case Caching_Mode::TURNED_OFF:
				static_cast<FTL*>(nvm_firmware)->Address_Mapping_Unit->Translate_lpa_to_ppa_and_dispatch(user_request->Transaction_list);
				return;
			case Caching_Mode::WRITE_CACHE:
			case Caching_Mode::READ_CACHE:
			case Caching_Mode::WRITE_READ_CACHE:
			{
				std::list<NVM_Transaction*>::iterator it = user_request->Transaction_list.begin();
				while (it != user_request->Transaction_list.end())
				{
					NVM_Transaction_Flash_RD* tr = (NVM_Transaction_Flash_RD*)(*it);
					if (per_stream_cache[tr->Stream_id]->Exists(tr->Stream_id, tr->LPA))
					{
						page_status_type available_sectors_bitmap = per_stream_cache[tr->Stream_id]->Get_slot(tr->Stream_id, tr->LPA).State_bitmap_of_existing_sectors & tr->read_sectors_bitmap;
						if (available_sectors_bitmap == tr->read_sectors_bitmap)
						{
							user_request->Sectors_serviced_from_cache += count_sector_no_from_status_bitmap(tr->read_sectors_bitmap);
							user_request->Transaction_list.erase(it++);//the ++ operation should happen here, otherwise the iterator will be part of the list after erasing it from the list
						}
						else if (available_sectors_bitmap != 0)
						{
							user_request->Sectors_serviced_from_cache += count_sector_no_from_status_bitmap(available_sectors_bitmap);
							tr->read_sectors_bitmap = (tr->read_sectors_bitmap & ~available_sectors_bitmap);
							tr->Data_and_metadata_size_in_byte -= count_sector_no_from_status_bitmap(available_sectors_bitmap) * SECTOR_SIZE_IN_BYTE;
							it++;
						}
						else it++;
					}
					else it++;
				}
				if (user_request->Sectors_serviced_from_cache > 0)
				{
					Memory_Transfer_Info* transfer_info = new Memory_Transfer_Info;
					transfer_info->Size = user_request->Sectors_serviced_from_cache * SECTOR_SIZE_IN_BYTE;
					transfer_info->Related_request = user_request;
					transfer_info->next_event_type = Data_Cache_Simulation_Event_Type::MEMORY_READ_FOR_USERIO_FINISHED;
					service_dram_access_request(transfer_info);
				}
				if (user_request->Transaction_list.size() > 0)
					static_cast<FTL*>(nvm_firmware)->Address_Mapping_Unit->Translate_lpa_to_ppa_and_dispatch(user_request->Transaction_list);

				return;
			}
			}
		}
		else//This is a write request
		{
			switch (caching_mode_per_input_stream[user_request->Stream_id])
			{
				case Caching_Mode::TURNED_OFF:
				case Caching_Mode::READ_CACHE:
					static_cast<FTL*>(nvm_firmware)->Address_Mapping_Unit->Translate_lpa_to_ppa_and_dispatch(user_request->Transaction_list);
					return;
				case Caching_Mode::WRITE_CACHE://The data cache manger unit performs like a destage buffer
					for (auto tr : user_request->Transaction_list)
					{
						user_request->Sectors_serviced_from_cache += count_sector_no_from_status_bitmap(((NVM_Transaction_Flash_WR*)tr)->write_sectors_bitmap);
						((NVM_Transaction_Flash_WR*)tr)->Source = Transaction_Source_Type::CACHE;
					}
					
					if (per_stream_cache[user_request->Stream_id]->Check_free_slot_availability(user_request->Sectors_serviced_from_cache))
						write_to_dram_cache(user_request);
					else
						waiting_user_requests_queue.push_back(user_request);
					break;
				case Caching_Mode::WRITE_READ_CACHE:
				{
					unsigned int cache_eviction_read_size_in_sectors = 0;
					std::list<NVM_Transaction*>* evicted_cache_slots = new std::list<NVM_Transaction*>;
					std::list<NVM_Transaction*>::iterator it = user_request->Transaction_list.begin();
					while (it != user_request->Transaction_list.end())
					{
						NVM_Transaction_Flash_WR* tr = (NVM_Transaction_Flash_WR*)(*it);
						if (per_stream_cache[tr->Stream_id]->Exists(tr->Stream_id, tr->LPA))//If the logical address already exists in the cache
						{
							/*MQSim should get rid of writting stale data to the cache.
							* This situation may result from out-of-order transaction execution*/
							DataCacheSlotType slot = per_stream_cache[tr->Stream_id]->Get_slot(tr->Stream_id, tr->LPA);
							sim_time_type timestamp = slot.Timestamp;
							NVM::memory_content_type content = slot.Content;
							if (tr->DataTimeStamp > timestamp)
							{
								timestamp = tr->DataTimeStamp;
								content = tr->Content;
							}

							per_stream_cache[tr->Stream_id]->Update_data(tr->Stream_id, tr->LPA, content, timestamp, tr->write_sectors_bitmap | slot.State_bitmap_of_existing_sectors);
							user_request->Sectors_serviced_from_cache += count_sector_no_from_status_bitmap(tr->write_sectors_bitmap);
							//DEBUG2("Updating page" << tr->LPA << " in write buffer ")
						}
						else//the logical address is not in the cache
						{
							if (!per_stream_cache[tr->Stream_id]->Check_free_slot_availability())
							{
								DataCacheSlotType evicted_slot = per_stream_cache[tr->Stream_id]->Evict_one_slot();
								if (evicted_slot.Dirty)
								{
									evicted_cache_slots->push_back(new NVM_Transaction_Flash_WR(Transaction_Source_Type::CACHE,
										tr->Stream_id, count_sector_no_from_status_bitmap(evicted_slot.State_bitmap_of_existing_sectors) * SECTOR_SIZE_IN_BYTE,
										evicted_slot.LPA, NULL, evicted_slot.Content, evicted_slot.State_bitmap_of_existing_sectors, evicted_slot.Timestamp));
									cache_eviction_read_size_in_sectors += count_sector_no_from_status_bitmap(evicted_slot.State_bitmap_of_existing_sectors);
									//DEBUG2("Evicting page" << evicted_slot.LPA << " from write buffer ")
								}
							}
							per_stream_cache[tr->Stream_id]->Insert_write_data(tr->Stream_id, tr->LPA, tr->Content, tr->DataTimeStamp, tr->write_sectors_bitmap);
							user_request->Sectors_serviced_from_cache += count_sector_no_from_status_bitmap(tr->write_sectors_bitmap);
						}
						user_request->Transaction_list.erase(it++);
					}

					if (evicted_cache_slots->size() > 0)
					{
						back_pressure_buffer_depth += cache_eviction_read_size_in_sectors;
						Memory_Transfer_Info* read_transfer_info = new Memory_Transfer_Info;
						read_transfer_info->Size = cache_eviction_read_size_in_sectors * SECTOR_SIZE_IN_BYTE;
						read_transfer_info->Related_request = evicted_cache_slots;
						read_transfer_info->next_event_type = Data_Cache_Simulation_Event_Type::MEMORY_READ_FOR_CACHE_FINISHED;
						service_dram_access_request(read_transfer_info);
						//DEBUG2("Starting memory transfer for cache eviction!")
					}

					Memory_Transfer_Info* write_transfer_info = new Memory_Transfer_Info;
					write_transfer_info->Size = user_request->Sectors_serviced_from_cache * SECTOR_SIZE_IN_BYTE;
					write_transfer_info->Related_request = user_request;
					write_transfer_info->next_event_type = Data_Cache_Simulation_Event_Type::MEMORY_WRITE_FOR_USERIO_FINISHED;
					if (back_pressure_buffer_depth >= back_pressure_buffer_max_depth)
					{
						waiting_access_request_queue.push(write_transfer_info);
					}
					else
					{
						service_dram_access_request(write_transfer_info);
						//DEBUG2("Starting memory transfer for cache write!")
					}
					return;
				}
			}
		}
	}

	void Data_Cache_Manager_Flash::handle_transaction_serviced_signal_from_PHY(NVM_Transaction_Flash* transaction)
	{
		//First check if the transaction source is a user request or the cache itself
		if (transaction->Source != Transaction_Source_Type::USERIO && transaction->Source != Transaction_Source_Type::CACHE)
			return;

		/* This is an update read (a read that is generated for a write request that partially updates page data).
		*  An update read transaction is issued in Address Mapping Unit, but is consumed in data cache manager.*/
		if (transaction->Type == Transaction_Type::READ)
		{
			if (((NVM_Transaction_Flash_RD*)transaction)->RelatedWrite != NULL)
			{
				((NVM_Transaction_Flash_RD*)transaction)->RelatedWrite->RelatedRead = NULL;
				return;
			}
			switch (Data_Cache_Manager_Flash::caching_mode_per_input_stream[transaction->Stream_id])
			{
			case Caching_Mode::TURNED_OFF:
			case Caching_Mode::WRITE_CACHE:
				transaction->UserIORequest->Transaction_list.remove(transaction);
				if (transaction->UserIORequest->Transaction_list.size() == 0)
					_my_instance->broadcast_user_request_serviced_signal(transaction->UserIORequest);
				break;
			case Caching_Mode::READ_CACHE:
			case Caching_Mode::WRITE_READ_CACHE:
			{
				Memory_Transfer_Info* transfer_info = new Memory_Transfer_Info;
				transfer_info->Size = count_sector_no_from_status_bitmap(((NVM_Transaction_Flash_RD*)transaction)->read_sectors_bitmap) * SECTOR_SIZE_IN_BYTE;
				transfer_info->next_event_type = Data_Cache_Simulation_Event_Type::MEMORY_WRITE_FOR_CACHE_FINISHED;
				((Data_Cache_Manager_Flash*)_my_instance)->service_dram_access_request(transfer_info);

				if (((Data_Cache_Manager_Flash*)_my_instance)->per_stream_cache[transaction->Stream_id]->Exists(transaction->Stream_id, transaction->LPA))
				{
					/*MQSim should get rid of writting stale data to the cache.
					* This situation may result from out-of-order transaction execution*/
					DataCacheSlotType slot = ((Data_Cache_Manager_Flash*)_my_instance)->per_stream_cache[transaction->Stream_id]->Get_slot(transaction->Stream_id, transaction->LPA);
					sim_time_type timestamp = slot.Timestamp;
					NVM::memory_content_type content = slot.Content;
					if (((NVM_Transaction_Flash_RD*)transaction)->DataTimeStamp > timestamp)
					{
						timestamp = ((NVM_Transaction_Flash_RD*)transaction)->DataTimeStamp;
						content = ((NVM_Transaction_Flash_RD*)transaction)->Content;
					}

					((Data_Cache_Manager_Flash*)_my_instance)->per_stream_cache[transaction->Stream_id]->Update_data(transaction->Stream_id, transaction->LPA, content,
						timestamp, ((NVM_Transaction_Flash_RD*)transaction)->read_sectors_bitmap | slot.State_bitmap_of_existing_sectors);
				}
				else 
				{
					if (!((Data_Cache_Manager_Flash*)_my_instance)->per_stream_cache[transaction->Stream_id]->Check_free_slot_availability())
					{
						std::list<NVM_Transaction*>* evicted_cache_slots = new std::list<NVM_Transaction*>;
						DataCacheSlotType evicted_slot = ((Data_Cache_Manager_Flash*)_my_instance)->per_stream_cache[transaction->Stream_id]->Evict_one_slot();
						if (evicted_slot.Dirty)
						{
							Memory_Transfer_Info* transfer_info = new Memory_Transfer_Info;
							transfer_info->Size = count_sector_no_from_status_bitmap(evicted_slot.State_bitmap_of_existing_sectors) * SECTOR_SIZE_IN_BYTE;
							evicted_cache_slots->push_back(new NVM_Transaction_Flash_WR(Transaction_Source_Type::USERIO,
								transaction->Stream_id, transfer_info->Size, evicted_slot.LPA, NULL, evicted_slot.Content,
								evicted_slot.State_bitmap_of_existing_sectors, evicted_slot.Timestamp));
							transfer_info->Related_request = evicted_cache_slots;
							transfer_info->next_event_type = Data_Cache_Simulation_Event_Type::MEMORY_READ_FOR_CACHE_FINISHED;
							((Data_Cache_Manager_Flash*)_my_instance)->service_dram_access_request(transfer_info);
						}
					}
					((Data_Cache_Manager_Flash*)_my_instance)->per_stream_cache[transaction->Stream_id]->Insert_write_data(transaction->Stream_id, transaction->LPA,
						((NVM_Transaction_Flash_RD*)transaction)->Content, ((NVM_Transaction_Flash_RD*)transaction)->DataTimeStamp, ((NVM_Transaction_Flash_RD*)transaction)->read_sectors_bitmap);

					Memory_Transfer_Info* transfer_info = new Memory_Transfer_Info;
					transfer_info->Size = count_sector_no_from_status_bitmap(((NVM_Transaction_Flash_RD*)transaction)->read_sectors_bitmap) * SECTOR_SIZE_IN_BYTE;
					transfer_info->next_event_type = Data_Cache_Simulation_Event_Type::MEMORY_WRITE_FOR_CACHE_FINISHED;
					((Data_Cache_Manager_Flash*)_my_instance)->service_dram_access_request(transfer_info);
				}

				transaction->UserIORequest->Transaction_list.remove(transaction);
				if (transaction->UserIORequest->Transaction_list.size() == 0)
					_my_instance->broadcast_user_request_serviced_signal(transaction->UserIORequest);
				break;
			}
			}
		}
		else//This is a write request
		{
			switch (Data_Cache_Manager_Flash::caching_mode_per_input_stream[transaction->Stream_id])
			{
				case Caching_Mode::TURNED_OFF:
				case Caching_Mode::READ_CACHE:
					transaction->UserIORequest->Transaction_list.remove(transaction);
					if (transaction->UserIORequest->Transaction_list.size() == 0)
						_my_instance->broadcast_user_request_serviced_signal(transaction->UserIORequest);
					break;
				case Caching_Mode::WRITE_CACHE:
				{
					if (((Data_Cache_Manager_Flash*)_my_instance)->per_stream_cache[transaction->Stream_id]->Exists(transaction->Stream_id, ((NVM_Transaction_Flash_WR*)transaction)->LPA))
					{
						DataCacheSlotType slot = ((Data_Cache_Manager_Flash*)_my_instance)->per_stream_cache[transaction->Stream_id]->Get_slot(transaction->Stream_id, ((NVM_Transaction_Flash_WR*)transaction)->LPA);
						sim_time_type timestamp = slot.Timestamp;
						NVM::memory_content_type content = slot.Content;
						if (((NVM_Transaction_Flash_WR*)transaction)->DataTimeStamp >= timestamp)
							((Data_Cache_Manager_Flash*)_my_instance)->per_stream_cache[transaction->Stream_id]->Remove_slot(transaction->Stream_id, ((NVM_Transaction_Flash_WR*)transaction)->LPA);
					}
					_my_instance->back_pressure_buffer_depth -= transaction->Data_and_metadata_size_in_byte / SECTOR_SIZE_IN_BYTE + (transaction->Data_and_metadata_size_in_byte % SECTOR_SIZE_IN_BYTE == 0 ? 0 : 1);
					unsigned int cache_eviction_read_size_in_sectors = 0;
					if (_my_instance->back_pressure_buffer_depth < _my_instance->back_pressure_buffer_max_depth)
					{
						std::list<NVM_Transaction*>* evicted_cache_slots = new std::list<NVM_Transaction*>;
						while (!((Data_Cache_Manager_Flash*)_my_instance)->per_stream_cache[transaction->Stream_id]->Empty())
						{
							DataCacheSlotType evicted_slot = ((Data_Cache_Manager_Flash*)_my_instance)->per_stream_cache[transaction->Stream_id]->Evict_one_slot();
							if (evicted_slot.Dirty)
							{
								evicted_cache_slots->push_back(new NVM_Transaction_Flash_WR(Transaction_Source_Type::CACHE,
									transaction->Stream_id, count_sector_no_from_status_bitmap(evicted_slot.State_bitmap_of_existing_sectors) * SECTOR_SIZE_IN_BYTE,
									evicted_slot.LPA, NULL, evicted_slot.Content, evicted_slot.State_bitmap_of_existing_sectors, evicted_slot.Timestamp));
								_my_instance->back_pressure_buffer_depth += count_sector_no_from_status_bitmap(evicted_slot.State_bitmap_of_existing_sectors);
								cache_eviction_read_size_in_sectors += count_sector_no_from_status_bitmap(evicted_slot.State_bitmap_of_existing_sectors);
							}
							if (_my_instance->back_pressure_buffer_depth >= _my_instance->back_pressure_buffer_max_depth)
								break;
						}
						
						Memory_Transfer_Info* read_transfer_info = new Memory_Transfer_Info;
						read_transfer_info->Size = cache_eviction_read_size_in_sectors * SECTOR_SIZE_IN_BYTE;
						read_transfer_info->Related_request = evicted_cache_slots;
						read_transfer_info->next_event_type = Data_Cache_Simulation_Event_Type::MEMORY_READ_FOR_CACHE_FINISHED;
						((Data_Cache_Manager_Flash*)_my_instance)->service_dram_access_request(read_transfer_info);

						for (auto user_request = ((Data_Cache_Manager_Flash*)_my_instance)->waiting_user_requests_queue.begin();
							user_request != ((Data_Cache_Manager_Flash*)_my_instance)->waiting_user_requests_queue.end();)
						{
							if (((Data_Cache_Manager_Flash*)_my_instance)->per_stream_cache[(*user_request)->Stream_id]->Check_free_slot_availability((*user_request)->Sectors_serviced_from_cache))
							{
								((Data_Cache_Manager_Flash*)_my_instance)->waiting_user_requests_queue.erase(user_request);
								((Data_Cache_Manager_Flash*)_my_instance)->write_to_dram_cache(*user_request);
							}
						}
					}
					break;
				}
				case Caching_Mode::WRITE_READ_CACHE:
				{
					_my_instance->back_pressure_buffer_depth -= transaction->Data_and_metadata_size_in_byte / SECTOR_SIZE_IN_BYTE;
					if (_my_instance->back_pressure_buffer_depth < _my_instance->back_pressure_buffer_max_depth && ((Data_Cache_Manager_Flash*)_my_instance)->waiting_access_request_queue.size() > 0)
					{
						((Data_Cache_Manager_Flash*)_my_instance)->service_dram_access_request(((Data_Cache_Manager_Flash*)_my_instance)->waiting_access_request_queue.front());
						((Data_Cache_Manager_Flash*)_my_instance)->waiting_access_request_queue.pop();
					}
					break;
				}
			}
		}
	}

	void Data_Cache_Manager_Flash::service_dram_access_request(Memory_Transfer_Info* request_info)
	{
		if (memory_channel_is_busy)
			dram_access_request_queue.push(request_info);
		else
		{
			Simulator->Register_sim_event(Simulator->Time() + estimate_dram_access_time(request_info->Size, dram_row_size,
				dram_busrt_size, dram_burst_transfer_time_ddr, dram_tRCD, dram_tCL, dram_tRP),
				this, request_info, static_cast<int>(request_info->next_event_type));
			memory_channel_is_busy = true;
		}
	}

	void Data_Cache_Manager_Flash::Execute_simulator_event(MQSimEngine::Sim_Event* ev)
	{
		Data_Cache_Simulation_Event_Type eventType = (Data_Cache_Simulation_Event_Type)ev->Type;
		Memory_Transfer_Info* transfer_info = (Memory_Transfer_Info*)ev->Parameters;

		switch (eventType)
		{
		case Data_Cache_Simulation_Event_Type::MEMORY_READ_FOR_USERIO_FINISHED:
		case Data_Cache_Simulation_Event_Type::MEMORY_WRITE_FOR_USERIO_FINISHED:
			((User_Request*)(transfer_info)->Related_request)->Sectors_serviced_from_cache = 0;
			if (((User_Request*)(transfer_info)->Related_request)->Transaction_list.size() == 0)
				broadcast_user_request_serviced_signal(((User_Request*)(transfer_info)->Related_request));
			break;
		case Data_Cache_Simulation_Event_Type::MEMORY_READ_FOR_CACHE_FINISHED:
			static_cast<FTL*>(nvm_firmware)->Address_Mapping_Unit->Translate_lpa_to_ppa_and_dispatch(*((std::list<NVM_Transaction*>*)(transfer_info->Related_request)));
			delete (std::list<NVM_Transaction*>*)transfer_info->Related_request;
			break;
		case Data_Cache_Simulation_Event_Type::MEMORY_WRITE_FOR_CACHE_FINISHED://The recently read data from flash is written back to memory to support future user read requests
			break;
		}
		delete transfer_info;

		memory_channel_is_busy = false;
		if (dram_access_request_queue.size() > 0)
		{
			Memory_Transfer_Info* transfer_info = dram_access_request_queue.front();
			dram_access_request_queue.pop();
			Simulator->Register_sim_event(Simulator->Time() + estimate_dram_access_time(transfer_info->Size, dram_row_size, dram_busrt_size,
				dram_burst_transfer_time_ddr, dram_tRCD, dram_tCL, dram_tRP),
				this, transfer_info, static_cast<int>(transfer_info->next_event_type));
			memory_channel_is_busy = true;
		}
	}

}