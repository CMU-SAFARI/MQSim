#include "GC_and_WL_Unit_Base.h"

namespace SSD_Components
{
	GC_and_WL_Unit_Base* GC_and_WL_Unit_Base::_my_instance;
	
	GC_and_WL_Unit_Base::GC_and_WL_Unit_Base(const sim_object_id_type& id,
		Address_Mapping_Unit_Base* address_mapping_unit, Flash_Block_Manager_Base* block_manager, TSU_Base* tsu, NVM_PHY_ONFI* flash_controller,
		GC_Block_Selection_Policy_Type block_selection_policy, double gc_threshold, bool preemptible_gc_enabled, double gc_hard_threshold,
		unsigned int channel_count, unsigned int chip_no_per_channel, unsigned int die_no_per_chip, unsigned int plane_no_per_die,
		unsigned int block_no_per_plane, unsigned int page_no_per_block, unsigned int sector_no_per_page, 
		bool use_copyback, double rho, unsigned int max_ongoing_gc_reqs_per_plane, bool dynamic_wearleveling_enabled, bool static_wearleveling_enabled, unsigned int static_wearleveling_threshold, int seed) :
		Sim_Object(id), address_mapping_unit(address_mapping_unit), block_manager(block_manager), tsu(tsu), flash_controller(flash_controller), force_gc(false),
		block_selection_policy(block_selection_policy), gc_threshold(gc_threshold),	use_copyback(use_copyback), 
		preemptible_gc_enabled(preemptible_gc_enabled), gc_hard_threshold(gc_hard_threshold),
		random_generator(seed), max_ongoing_gc_reqs_per_plane(max_ongoing_gc_reqs_per_plane),
		channel_count(channel_count), chip_no_per_channel(chip_no_per_channel), die_no_per_chip(die_no_per_chip), plane_no_per_die(plane_no_per_die),
		block_no_per_plane(block_no_per_plane), pages_no_per_block(page_no_per_block), sector_no_per_page(sector_no_per_page),
		dynamic_wearleveling_enabled(dynamic_wearleveling_enabled), static_wearleveling_enabled(static_wearleveling_enabled), static_wearleveling_threshold(static_wearleveling_threshold)
	{
		_my_instance = this;
		block_pool_gc_threshold = (unsigned int)(gc_threshold * (double)block_no_per_plane);
		if (block_pool_gc_threshold < 1)
			block_pool_gc_threshold = 1;
		block_pool_gc_hard_threshold = (unsigned int)(gc_hard_threshold * (double)block_no_per_plane);
		if (block_pool_gc_hard_threshold < 1)
			block_pool_gc_hard_threshold = 1;
		random_pp_threshold = (unsigned int)(rho * pages_no_per_block);
	}

	void GC_and_WL_Unit_Base::Setup_triggers()
	{
		Sim_Object::Setup_triggers();
		flash_controller->ConnectToTransactionServicedSignal(handle_transaction_serviced_signal_from_PHY);
	}

	void GC_and_WL_Unit_Base::handle_transaction_serviced_signal_from_PHY(NVM_Transaction_Flash* transaction)
	{
		PlaneBookKeepingType* pbke = &(_my_instance->block_manager->plane_manager[transaction->Address.ChannelID][transaction->Address.ChipID][transaction->Address.DieID][transaction->Address.PlaneID]);
		
		switch (transaction->Source)
		{
		case Transaction_Source_Type::USERIO:
		case Transaction_Source_Type::MAPPING:
		case Transaction_Source_Type::CACHE:
			switch (transaction->Type)
			{
			case Transaction_Type::READ:
				_my_instance->block_manager->Read_transaction_serviced(transaction->Address);
				break;
			case Transaction_Type::WRITE:
				_my_instance->block_manager->Program_transaction_serviced(transaction->Address);
				break;
			default:
				PRINT_ERROR("Unexpected situation occured in GC_and_WL_Unit_Base")
			}
			if(_my_instance->block_manager->Has_ongoing_gc(transaction->Address))
				if (_my_instance->block_manager->Can_execute_gc(transaction->Address))
				{
					NVM::FlashMemory::Physical_Page_Address gc_candidate_address(transaction->Address);
					Block_Pool_Slot_Type* block = &pbke->Blocks[transaction->Address.BlockID];
					_my_instance->tsu->Prepare_for_transaction_submit();
					NVM_Transaction_Flash_ER* gc_erase_tr = new NVM_Transaction_Flash_ER(Transaction_Source_Type::GC, block->Stream_id, gc_candidate_address);
					if (block->Current_page_write_index - block->Invalid_page_count > 0)//If there are some valid pages in block, then prepare flash transactions for page movement
					{
						//address_mapping_unit->Lock_physical_block_for_gc(gc_candidate_address);//Lock the block, so no user request can intervene while the GC is progressing
						NVM_Transaction_Flash_RD* gc_read = NULL;
						NVM_Transaction_Flash_WR* gc_write = NULL;
						for (flash_page_ID_type pageID = 0; pageID < block->Current_page_write_index; pageID++)
						{
							if (_my_instance->block_manager->Is_page_valid(block, pageID))
							{
								gc_candidate_address.PageID = pageID;
								if (_my_instance->use_copyback)
								{
									gc_write = new NVM_Transaction_Flash_WR(Transaction_Source_Type::GC, block->Stream_id, _my_instance->sector_no_per_page * SECTOR_SIZE_IN_BYTE,
										NO_LPA, _my_instance->address_mapping_unit->Convert_address_to_ppa(gc_candidate_address), NULL, 0, NULL, 0, INVALID_TIME_STAMP);
									gc_write->ExecutionMode = WriteExecutionModeType::COPYBACK;
									_my_instance->tsu->Submit_transaction(gc_write);
								}
								else
								{
									gc_read = new NVM_Transaction_Flash_RD(Transaction_Source_Type::GC, block->Stream_id, _my_instance->sector_no_per_page * SECTOR_SIZE_IN_BYTE,
										NO_LPA, _my_instance->address_mapping_unit->Convert_address_to_ppa(gc_candidate_address), gc_candidate_address, NULL, 0, NULL, 0, INVALID_TIME_STAMP);
									gc_write = new NVM_Transaction_Flash_WR(Transaction_Source_Type::GC, block->Stream_id, _my_instance->sector_no_per_page * SECTOR_SIZE_IN_BYTE,
										NO_LPA, NO_PPA, gc_candidate_address, NULL, 0, gc_read, 0, INVALID_TIME_STAMP);
									gc_write->ExecutionMode = WriteExecutionModeType::SIMPLE;
									gc_write->RelatedErase = gc_erase_tr;
									gc_read->RelatedWrite = gc_write;
									_my_instance->tsu->Submit_transaction(gc_read);//Only the read transaction would be submitted. The Write transaction is submitted when the read transaction is finished and the LPA of the target page is determined
								}
								gc_erase_tr->Page_movement_activities.push_back(gc_write);
							}
						}
					}
					block->Erase_transaction = gc_erase_tr;
					_my_instance->tsu->Schedule();
				}
			return;
		}

		switch (transaction->Type)
		{
		case Transaction_Type::READ:
		{
			PPA_type ppa;
			MPPN_type mppa;
			page_status_type page_status_bitmap;
			if (pbke->Blocks[transaction->Address.BlockID].Holds_mapping_data)
			{
				_my_instance->address_mapping_unit->Get_translation_mapping_info_for_gc(transaction->Stream_id, (MVPN_type)transaction->LPA, mppa, page_status_bitmap);
				if (mppa == transaction->PPA)//There has been no write on the page since GC start, and it is still valid
				{
					_my_instance->tsu->Prepare_for_transaction_submit();
					((NVM_Transaction_Flash_RD*)transaction)->RelatedWrite->write_sectors_bitmap = FULL_PROGRAMMED_PAGE;
					((NVM_Transaction_Flash_RD*)transaction)->RelatedWrite->LPA = transaction->LPA;
					((NVM_Transaction_Flash_RD*)transaction)->RelatedWrite->RelatedRead = NULL;
					_my_instance->address_mapping_unit->Allocate_new_page_for_gc(((NVM_Transaction_Flash_RD*)transaction)->RelatedWrite, pbke->Blocks[transaction->Address.BlockID].Holds_mapping_data);
					_my_instance->tsu->Submit_transaction(((NVM_Transaction_Flash_RD*)transaction)->RelatedWrite);
					_my_instance->tsu->Schedule();
				}
				else
					PRINT_ERROR("Inconsistent situation occured in GC page movement!")
			}
			else
			{
				_my_instance->address_mapping_unit->Get_data_mapping_info_for_gc(transaction->Stream_id, transaction->LPA, ppa, page_status_bitmap);
				if (ppa == transaction->PPA)//There has been no write on the page since GC start, and it is still valid
				{
					_my_instance->tsu->Prepare_for_transaction_submit();
					((NVM_Transaction_Flash_RD*)transaction)->RelatedWrite->write_sectors_bitmap = page_status_bitmap;
					((NVM_Transaction_Flash_RD*)transaction)->RelatedWrite->LPA = transaction->LPA;
					((NVM_Transaction_Flash_RD*)transaction)->RelatedWrite->RelatedRead = NULL;
					_my_instance->address_mapping_unit->Allocate_new_page_for_gc(((NVM_Transaction_Flash_RD*)transaction)->RelatedWrite, pbke->Blocks[transaction->Address.BlockID].Holds_mapping_data);
					_my_instance->tsu->Submit_transaction(((NVM_Transaction_Flash_RD*)transaction)->RelatedWrite);
					_my_instance->tsu->Schedule();
				}
				else
					PRINT_ERROR("Inconsistent situation occured in GC page movement!")
			}
			break;
		}
		case Transaction_Type::WRITE:
			if (pbke->Blocks[((NVM_Transaction_Flash_WR*)transaction)->RelatedErase->Address.BlockID].Holds_mapping_data)
			{
				_my_instance->address_mapping_unit->Unlock_mvpn_after_gc(transaction->Stream_id, (MVPN_type)transaction->LPA);
				DEBUG2(Simulator->Time() << ": MVPN=" << (MVPN_type)transaction->LPA << " unlocked!!");
			}
			else
			{
				_my_instance->address_mapping_unit->Unlock_lpa_after_gc(transaction->Stream_id, transaction->LPA);
				DEBUG2(Simulator->Time() << ": LPA=" << (MVPN_type)transaction->LPA << " unlocked!!");
			}
			pbke->Blocks[((NVM_Transaction_Flash_WR*)transaction)->RelatedErase->Address.BlockID].Erase_transaction->Page_movement_activities.remove((NVM_Transaction_Flash_WR*)transaction);
			break;
		case Transaction_Type::ERASE:
			pbke->Ongoing_erase_operations.erase(pbke->Ongoing_erase_operations.find(transaction->Address.BlockID));
			_my_instance->block_manager->Add_erased_block_to_pool(transaction->Address);
			_my_instance->block_manager->GC_finished(transaction->Address);
			break;
		}
	}

	void GC_and_WL_Unit_Base::Start_simulation() {}

	void GC_and_WL_Unit_Base::Validate_simulation_config() {}

	void GC_and_WL_Unit_Base::Execute_simulator_event(MQSimEngine::Sim_Event* ev) {}

	GC_Block_Selection_Policy_Type GC_and_WL_Unit_Base::Get_gc_policy()
	{
		return block_selection_policy;
	}

	unsigned int GC_and_WL_Unit_Base::Get_GC_policy_specific_parameter()
	{
		switch (block_selection_policy)
		{
		case GC_Block_Selection_Policy_Type::RGA:
			return rga_set_size;
		case GC_Block_Selection_Policy_Type::RANDOM_PP:
			return random_pp_threshold;
		}
		return 0;
	}

	unsigned int GC_and_WL_Unit_Base::Get_minimum_number_of_free_pages_before_GC()
	{
		return block_pool_gc_threshold;
		/*if (preemptible_gc_enabled)
			return block_pool_gc_hard_threshold;
		else return block_pool_gc_threshold;*/
	}

	bool GC_and_WL_Unit_Base::Use_dynamic_wearleveling()
	{
		return dynamic_wearleveling_enabled;
	}

	inline bool GC_and_WL_Unit_Base::Use_static_wearleveling()
	{
		return static_wearleveling_enabled;
	}

	bool GC_and_WL_Unit_Base::is_safe_gc_candidate(const PlaneBookKeepingType* plane_record, const flash_block_ID_type gc_candidate_block_id)
	{
		//The block shouldn't be a current write frontier
		for (unsigned int stream_id = 0; stream_id < address_mapping_unit->Get_no_of_input_streams(); stream_id++)
			if ((&plane_record->Blocks[gc_candidate_block_id]) == plane_record->Data_wf[stream_id]
				|| (&plane_record->Blocks[gc_candidate_block_id]) == plane_record->Translation_wf[stream_id]
				|| (&plane_record->Blocks[gc_candidate_block_id]) == plane_record->GC_wf[stream_id])
				return false;

		//The block shouldn't have an ongoing program request (all pages must already be written)
		if (plane_record->Blocks[gc_candidate_block_id].Ongoing_user_program_count > 0)
			return false;

		if (plane_record->Blocks[gc_candidate_block_id].Has_ongoing_gc)
			return false;

		return true;
	}
}