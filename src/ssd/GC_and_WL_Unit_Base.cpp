#include "GC_and_WL_Unit_Base.h"

namespace SSD_Components
{
	GC_and_WL_Unit_Base* GC_and_WL_Unit_Base::_my_instance;
	
	GC_and_WL_Unit_Base::GC_and_WL_Unit_Base(const sim_object_id_type& id,
		Address_Mapping_Unit_Base* address_mapping_unit, Flash_Block_Manager_Base* block_manager, TSU_Base* tsu, NVM_PHY_ONFI* flash_controller,
		GC_Block_Selection_Policy_Type block_selection_policy, double gc_threshold, bool preemptible_gc_enabled, double gc_hard_threshold,
		unsigned int channel_count, unsigned int chip_no_per_channel, unsigned int die_no_per_chip, unsigned int plane_no_per_die,
		unsigned int block_no_per_plane, unsigned int page_no_per_block, unsigned int sector_no_per_page) :
		Sim_Object(id), address_mapping_unit(address_mapping_unit), block_manager(block_manager), tsu(tsu), flash_controller(flash_controller), force_gc(false),
		block_selection_policy(block_selection_policy), gc_threshold(gc_threshold), preemptible_gc_enabled(preemptible_gc_enabled), gc_hard_threshold(gc_hard_threshold),
		channel_count(channel_count), chip_no_per_channel(chip_no_per_channel), die_no_per_chip(die_no_per_chip), plane_no_per_die(plane_no_per_die),
		block_no_per_plane(block_no_per_plane), pages_no_per_block(page_no_per_block), sector_no_per_page(sector_no_per_page)
	{
		_my_instance = this;
		block_pool_gc_threshold = (unsigned int)(gc_threshold * (double)block_no_per_plane);
		if (block_pool_gc_threshold < 1)
			block_pool_gc_threshold = 1;
		block_pool_gc_hard_threshold = (unsigned int)(gc_hard_threshold * (double)block_no_per_plane);
		if (block_pool_gc_hard_threshold < 1)
			block_pool_gc_hard_threshold = 1;
	}

	void GC_and_WL_Unit_Base::Setup_triggers()
	{
		Sim_Object::Setup_triggers();
		flash_controller->ConnectToTransactionServicedSignal(handle_transaction_serviced_signal_from_PHY);
	}

	void GC_and_WL_Unit_Base::handle_transaction_serviced_signal_from_PHY(NVM_Transaction_Flash* transaction)
	{
		if (transaction->Source != Transaction_Source_Type::GC)
			return;

		PlaneBookKeepingType* pbke = &(_my_instance->block_manager->plane_manager[transaction->Address.ChannelID][transaction->Address.ChipID][transaction->Address.DieID][transaction->Address.PlaneID]);

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
					_my_instance->address_mapping_unit->Lock_mvpn(transaction->Stream_id, (MVPN_type)transaction->LPA);
					_my_instance->tsu->Prepare_for_transaction_submit();
					((NVM_Transaction_Flash_RD*)transaction)->RelatedWrite->write_sectors_bitmap = FULL_PROGRAMMED_PAGE;
					((NVM_Transaction_Flash_RD*)transaction)->RelatedWrite->LPA = transaction->LPA;
					((NVM_Transaction_Flash_RD*)transaction)->RelatedWrite->RelatedRead = NULL;
					_my_instance->address_mapping_unit->Allocate_new_page_for_gc(((NVM_Transaction_Flash_RD*)transaction)->RelatedWrite, pbke->Blocks[transaction->Address.BlockID].Holds_mapping_data);
					_my_instance->tsu->Submit_transaction(((NVM_Transaction_Flash_RD*)transaction)->RelatedWrite);
					_my_instance->tsu->Schedule();
				}
				else
					pbke->Blocks[transaction->Address.BlockID].Erase_transaction->Page_movement_activities.remove(((NVM_Transaction_Flash_RD*)transaction)->RelatedWrite);
			}
			else
			{
				_my_instance->address_mapping_unit->Get_data_mapping_info_for_gc(transaction->Stream_id, transaction->LPA, ppa, page_status_bitmap);
				if (ppa == transaction->PPA)//There has been no write on the page since GC start, and it is still valid
				{
					_my_instance->address_mapping_unit->Lock_lpa(transaction->Stream_id, transaction->LPA);
					_my_instance->tsu->Prepare_for_transaction_submit();
					((NVM_Transaction_Flash_RD*)transaction)->RelatedWrite->write_sectors_bitmap = page_status_bitmap;
					((NVM_Transaction_Flash_RD*)transaction)->RelatedWrite->LPA = transaction->LPA;
					((NVM_Transaction_Flash_RD*)transaction)->RelatedWrite->RelatedRead = NULL;
					_my_instance->address_mapping_unit->Allocate_new_page_for_gc(((NVM_Transaction_Flash_RD*)transaction)->RelatedWrite, pbke->Blocks[transaction->Address.BlockID].Holds_mapping_data);
					_my_instance->tsu->Submit_transaction(((NVM_Transaction_Flash_RD*)transaction)->RelatedWrite);
					_my_instance->tsu->Schedule();
				}
				else
					pbke->Blocks[transaction->Address.BlockID].Erase_transaction->Page_movement_activities.remove(((NVM_Transaction_Flash_RD*)transaction)->RelatedWrite);
			}
			break;
		}
		case Transaction_Type::WRITE:
			if (pbke->Blocks[transaction->Address.BlockID].Holds_mapping_data)
				_my_instance->address_mapping_unit->Unlock_mvpn(transaction->Stream_id, (MVPN_type)transaction->LPA);
			else 
				_my_instance->address_mapping_unit->Unlock_lpa(transaction->Stream_id, transaction->LPA);
			pbke->Blocks[transaction->Address.BlockID].Erase_transaction->Page_movement_activities.remove((NVM_Transaction_Flash_WR*)transaction);
			break;
		case Transaction_Type::ERASE:
			pbke->Ongoing_erase_operations.erase(pbke->Ongoing_erase_operations.find(transaction->Address.BlockID));
			_my_instance->block_manager->Add_erased_block_to_pool(transaction->Address);
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

}