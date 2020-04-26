#include "TSU_OutofOrder.h"

namespace SSD_Components
{

TSU_OutOfOrder::TSU_OutOfOrder(const sim_object_id_type &id, FTL *ftl, NVM_PHY_ONFI_NVDDR2 *NVMController, unsigned int ChannelCount, unsigned int chip_no_per_channel,
							   unsigned int DieNoPerChip, unsigned int PlaneNoPerDie,
							   sim_time_type WriteReasonableSuspensionTimeForRead,
							   sim_time_type EraseReasonableSuspensionTimeForRead,
							   sim_time_type EraseReasonableSuspensionTimeForWrite,
							   bool EraseSuspensionEnabled, bool ProgramSuspensionEnabled)
	: TSU_Base(id, ftl, NVMController, Flash_Scheduling_Type::OUT_OF_ORDER, ChannelCount, chip_no_per_channel, DieNoPerChip, PlaneNoPerDie,
			   WriteReasonableSuspensionTimeForRead, EraseReasonableSuspensionTimeForRead, EraseReasonableSuspensionTimeForWrite,
			   EraseSuspensionEnabled, ProgramSuspensionEnabled)
{
	UserReadTRQueue = new Flash_Transaction_Queue *[channel_count];
	UserWriteTRQueue = new Flash_Transaction_Queue *[channel_count];
	GCReadTRQueue = new Flash_Transaction_Queue *[channel_count];
	GCWriteTRQueue = new Flash_Transaction_Queue *[channel_count];
	GCEraseTRQueue = new Flash_Transaction_Queue *[channel_count];
	MappingReadTRQueue = new Flash_Transaction_Queue *[channel_count];
	MappingWriteTRQueue = new Flash_Transaction_Queue *[channel_count];
	for (unsigned int channelID = 0; channelID < channel_count; channelID++)
	{
		UserReadTRQueue[channelID] = new Flash_Transaction_Queue[chip_no_per_channel];
		UserWriteTRQueue[channelID] = new Flash_Transaction_Queue[chip_no_per_channel];
		GCReadTRQueue[channelID] = new Flash_Transaction_Queue[chip_no_per_channel];
		GCWriteTRQueue[channelID] = new Flash_Transaction_Queue[chip_no_per_channel];
		GCEraseTRQueue[channelID] = new Flash_Transaction_Queue[chip_no_per_channel];
		MappingReadTRQueue[channelID] = new Flash_Transaction_Queue[chip_no_per_channel];
		MappingWriteTRQueue[channelID] = new Flash_Transaction_Queue[chip_no_per_channel];
		for (unsigned int chip_cntr = 0; chip_cntr < chip_no_per_channel; chip_cntr++)
		{
			UserReadTRQueue[channelID][chip_cntr].Set_id("User_Read_TR_Queue@" + std::to_string(channelID) + "@" + std::to_string(chip_cntr));
			UserWriteTRQueue[channelID][chip_cntr].Set_id("User_Write_TR_Queue@" + std::to_string(channelID) + "@" + std::to_string(chip_cntr));
			GCReadTRQueue[channelID][chip_cntr].Set_id("GC_Read_TR_Queue@" + std::to_string(channelID) + "@" + std::to_string(chip_cntr));
			MappingReadTRQueue[channelID][chip_cntr].Set_id("Mapping_Read_TR_Queue@" + std::to_string(channelID) + "@" + std::to_string(chip_cntr));
			MappingWriteTRQueue[channelID][chip_cntr].Set_id("Mapping_Write_TR_Queue@" + std::to_string(channelID) + "@" + std::to_string(chip_cntr));
			GCWriteTRQueue[channelID][chip_cntr].Set_id("GC_Write_TR_Queue@" + std::to_string(channelID) + "@" + std::to_string(chip_cntr));
			GCEraseTRQueue[channelID][chip_cntr].Set_id("GC_Erase_TR_Queue@" + std::to_string(channelID) + "@" + std::to_string(chip_cntr));
		}
	}
}

TSU_OutOfOrder::~TSU_OutOfOrder()
{
	for (unsigned int channelID = 0; channelID < channel_count; channelID++)
	{
		delete[] UserReadTRQueue[channelID];
		delete[] UserWriteTRQueue[channelID];
		delete[] GCReadTRQueue[channelID];
		delete[] GCWriteTRQueue[channelID];
		delete[] GCEraseTRQueue[channelID];
		delete[] MappingReadTRQueue[channelID];
		delete[] MappingWriteTRQueue[channelID];
	}
	delete[] UserReadTRQueue;
	delete[] UserWriteTRQueue;
	delete[] GCReadTRQueue;
	delete[] GCWriteTRQueue;
	delete[] GCEraseTRQueue;
	delete[] MappingReadTRQueue;
	delete[] MappingWriteTRQueue;
}

void TSU_OutOfOrder::Start_simulation()
{
}

void TSU_OutOfOrder::Validate_simulation_config()
{
}

void TSU_OutOfOrder::Execute_simulator_event(MQSimEngine::Sim_Event *event)
{
}

void TSU_OutOfOrder::Report_results_in_XML(std::string name_prefix, Utils::XmlWriter &xmlwriter)
{
	name_prefix = name_prefix + ".TSU";
	xmlwriter.Write_open_tag(name_prefix);

	TSU_Base::Report_results_in_XML(name_prefix, xmlwriter);

	for (unsigned int channelID = 0; channelID < channel_count; channelID++)
	{
		for (unsigned int chip_cntr = 0; chip_cntr < chip_no_per_channel; chip_cntr++)
		{
			UserReadTRQueue[channelID][chip_cntr].Report_results_in_XML(name_prefix + ".User_Read_TR_Queue", xmlwriter);
		}
	}

	for (unsigned int channelID = 0; channelID < channel_count; channelID++)
	{
		for (unsigned int chip_cntr = 0; chip_cntr < chip_no_per_channel; chip_cntr++)
		{
			UserWriteTRQueue[channelID][chip_cntr].Report_results_in_XML(name_prefix + ".User_Write_TR_Queue", xmlwriter);
		}
	}

	for (unsigned int channelID = 0; channelID < channel_count; channelID++)
	{
		for (unsigned int chip_cntr = 0; chip_cntr < chip_no_per_channel; chip_cntr++)
		{
			MappingReadTRQueue[channelID][chip_cntr].Report_results_in_XML(name_prefix + ".Mapping_Read_TR_Queue", xmlwriter);
		}
	}

	for (unsigned int channelID = 0; channelID < channel_count; channelID++)
	{
		for (unsigned int chip_cntr = 0; chip_cntr < chip_no_per_channel; chip_cntr++)
		{
			MappingWriteTRQueue[channelID][chip_cntr].Report_results_in_XML(name_prefix + ".Mapping_Write_TR_Queue", xmlwriter);
		}
	}

	for (unsigned int channelID = 0; channelID < channel_count; channelID++)
	{
		for (unsigned int chip_cntr = 0; chip_cntr < chip_no_per_channel; chip_cntr++)
		{
			GCReadTRQueue[channelID][chip_cntr].Report_results_in_XML(name_prefix + ".GC_Read_TR_Queue", xmlwriter);
		}
	}

	for (unsigned int channelID = 0; channelID < channel_count; channelID++)
	{
		for (unsigned int chip_cntr = 0; chip_cntr < chip_no_per_channel; chip_cntr++)
		{
			GCWriteTRQueue[channelID][chip_cntr].Report_results_in_XML(name_prefix + ".GC_Write_TR_Queue", xmlwriter);
		}
	}

	for (unsigned int channelID = 0; channelID < channel_count; channelID++)
	{
		for (unsigned int chip_cntr = 0; chip_cntr < chip_no_per_channel; chip_cntr++)
		{
			GCEraseTRQueue[channelID][chip_cntr].Report_results_in_XML(name_prefix + ".GC_Erase_TR_Queue", xmlwriter);
		}
	}

	xmlwriter.Write_close_tag();
}

void TSU_OutOfOrder::Schedule()
{
	opened_scheduling_reqs--;
	if (opened_scheduling_reqs > 0)
	{
		return;
	}

	if (opened_scheduling_reqs < 0)
	{
		PRINT_ERROR("TSU_OutOfOrder: Illegal status!");
	}

	if (transaction_receive_slots.size() == 0)
	{
		return;
	}

	for (std::list<NVM_Transaction_Flash *>::iterator it = transaction_receive_slots.begin(); it != transaction_receive_slots.end(); it++)
	{
		switch ((*it)->Type)
		{
		case Transaction_Type::READ:
			switch ((*it)->Source)
			{
			case Transaction_Source_Type::CACHE:
			case Transaction_Source_Type::USERIO:
				UserReadTRQueue[(*it)->Address.ChannelID][(*it)->Address.ChipID].push_back((*it));
				break;
			case Transaction_Source_Type::MAPPING:
				MappingReadTRQueue[(*it)->Address.ChannelID][(*it)->Address.ChipID].push_back((*it));
				break;
			case Transaction_Source_Type::GC_WL:
				GCReadTRQueue[(*it)->Address.ChannelID][(*it)->Address.ChipID].push_back((*it));
				break;
			default:
				PRINT_ERROR("TSU_OutOfOrder: unknown source type for a read transaction!")
			}
			break;
		case Transaction_Type::WRITE:
			switch ((*it)->Source)
			{
			case Transaction_Source_Type::CACHE:
			case Transaction_Source_Type::USERIO:
				UserWriteTRQueue[(*it)->Address.ChannelID][(*it)->Address.ChipID].push_back((*it));
				break;
			case Transaction_Source_Type::MAPPING:
				MappingWriteTRQueue[(*it)->Address.ChannelID][(*it)->Address.ChipID].push_back((*it));
				break;
			case Transaction_Source_Type::GC_WL:
				GCWriteTRQueue[(*it)->Address.ChannelID][(*it)->Address.ChipID].push_back((*it));
				break;
			default:
				PRINT_ERROR("TSU_OutOfOrder: unknown source type for a write transaction!")
			}
			break;
		case Transaction_Type::ERASE:
			GCEraseTRQueue[(*it)->Address.ChannelID][(*it)->Address.ChipID].push_back((*it));
			break;
		default:
			break;
		}
	}

	for (flash_channel_ID_type channelID = 0; channelID < channel_count; channelID++)
	{
		if (_NVMController->Get_channel_status(channelID) == BusChannelStatus::IDLE)
		{
			for (unsigned int i = 0; i < chip_no_per_channel; i++)
			{
				NVM::FlashMemory::Flash_Chip *chip = _NVMController->Get_chip(channelID, Round_robin_turn_of_channel[channelID]);
				//The TSU does not check if the chip is idle or not since it is possible to suspend a busy chip and issue a new command
				process_chip_requests(chip);
				Round_robin_turn_of_channel[channelID] = (flash_chip_ID_type)(Round_robin_turn_of_channel[channelID] + 1) % chip_no_per_channel;
				if (_NVMController->Get_channel_status(chip->ChannelID) != BusChannelStatus::IDLE)
				{
					break;
				}
			}
		}
	}
}

bool TSU_OutOfOrder::service_read_transaction(NVM::FlashMemory::Flash_Chip *chip)
{
	Flash_Transaction_Queue *sourceQueue1 = NULL, *sourceQueue2 = NULL;

	//Flash transactions that are related to FTL mapping data have the highest priority
	if (MappingReadTRQueue[chip->ChannelID][chip->ChipID].size() > 0)
	{
		sourceQueue1 = &MappingReadTRQueue[chip->ChannelID][chip->ChipID];
		if (ftl->GC_and_WL_Unit->GC_is_in_urgent_mode(chip) && GCReadTRQueue[chip->ChannelID][chip->ChipID].size() > 0)
		{
			sourceQueue2 = &GCReadTRQueue[chip->ChannelID][chip->ChipID];
		}
		else if (UserReadTRQueue[chip->ChannelID][chip->ChipID].size() > 0)
		{
			sourceQueue2 = &UserReadTRQueue[chip->ChannelID][chip->ChipID];
		}
	}
	else if (ftl->GC_and_WL_Unit->GC_is_in_urgent_mode(chip))
	{
		//If flash transactions related to GC are prioritzed (non-preemptive execution mode of GC), then GC queues are checked first

		if (GCReadTRQueue[chip->ChannelID][chip->ChipID].size() > 0)
		{
			sourceQueue1 = &GCReadTRQueue[chip->ChannelID][chip->ChipID];
			if (UserReadTRQueue[chip->ChannelID][chip->ChipID].size() > 0)
			{
				sourceQueue2 = &UserReadTRQueue[chip->ChannelID][chip->ChipID];
			}
		}
		else if (GCWriteTRQueue[chip->ChannelID][chip->ChipID].size() > 0)
		{
			return false;
		}
		else if (GCEraseTRQueue[chip->ChannelID][chip->ChipID].size() > 0)
		{
			return false;
		}
		else if (UserReadTRQueue[chip->ChannelID][chip->ChipID].size() > 0)
		{
			sourceQueue1 = &UserReadTRQueue[chip->ChannelID][chip->ChipID];
		}
		else
		{
			return false;
		}
	}
	else
	{
		//If GC is currently executed in the preemptive mode, then user IO transaction queues are checked first

		if (UserReadTRQueue[chip->ChannelID][chip->ChipID].size() > 0)
		{
			sourceQueue1 = &UserReadTRQueue[chip->ChannelID][chip->ChipID];
			if (GCReadTRQueue[chip->ChannelID][chip->ChipID].size() > 0)
			{
				sourceQueue2 = &GCReadTRQueue[chip->ChannelID][chip->ChipID];
			}
		}
		else if (UserWriteTRQueue[chip->ChannelID][chip->ChipID].size() > 0)
		{
			return false;
		}
		else if (GCReadTRQueue[chip->ChannelID][chip->ChipID].size() > 0)
		{
			sourceQueue1 = &GCReadTRQueue[chip->ChannelID][chip->ChipID];
		}
		else
		{
			return false;
		}
	}

	bool suspensionRequired = false;
	ChipStatus cs = _NVMController->GetChipStatus(chip);
	switch (cs)
	{
	case ChipStatus::IDLE:
		break;
	case ChipStatus::WRITING:
		if (!programSuspensionEnabled || _NVMController->HasSuspendedCommand(chip))
		{
			return false;
		}
		if (_NVMController->Expected_finish_time(chip) - Simulator->Time() < writeReasonableSuspensionTimeForRead)
		{
			return false;
		}
		suspensionRequired = true;
	case ChipStatus::ERASING:
		if (!eraseSuspensionEnabled || _NVMController->HasSuspendedCommand(chip))
		{
			return false;
		}
		if (_NVMController->Expected_finish_time(chip) - Simulator->Time() < eraseReasonableSuspensionTimeForRead)
		{
			return false;
		}
		suspensionRequired = true;
	default:
		return false;
	}

	issue_command_to_chip(sourceQueue1, sourceQueue2, Transaction_Type::READ, suspensionRequired);

	return true;
}

bool TSU_OutOfOrder::service_write_transaction(NVM::FlashMemory::Flash_Chip *chip)
{
	Flash_Transaction_Queue *sourceQueue1 = NULL, *sourceQueue2 = NULL;

	//If flash transactions related to GC are prioritzed (non-preemptive execution mode of GC), then GC queues are checked first
	if (ftl->GC_and_WL_Unit->GC_is_in_urgent_mode(chip))
	{
		if (GCWriteTRQueue[chip->ChannelID][chip->ChipID].size() > 0)
		{
			sourceQueue1 = &GCWriteTRQueue[chip->ChannelID][chip->ChipID];
			if (UserWriteTRQueue[chip->ChannelID][chip->ChipID].size() > 0)
			{
				sourceQueue2 = &UserWriteTRQueue[chip->ChannelID][chip->ChipID];
			}
		}
		else if (GCEraseTRQueue[chip->ChannelID][chip->ChipID].size() > 0)
		{
			return false;
		}
		else if (UserWriteTRQueue[chip->ChannelID][chip->ChipID].size() > 0)
		{
			sourceQueue1 = &UserWriteTRQueue[chip->ChannelID][chip->ChipID];
		}
		else
		{
			return false;
		}
	}
	else
	{
		//If GC is currently executed in the preemptive mode, then user IO transaction queues are checked first
		if (UserWriteTRQueue[chip->ChannelID][chip->ChipID].size() > 0)
		{
			sourceQueue1 = &UserWriteTRQueue[chip->ChannelID][chip->ChipID];
			if (GCWriteTRQueue[chip->ChannelID][chip->ChipID].size() > 0)
			{
				sourceQueue2 = &GCWriteTRQueue[chip->ChannelID][chip->ChipID];
			}
		}
		else if (GCWriteTRQueue[chip->ChannelID][chip->ChipID].size() > 0)
		{
			sourceQueue1 = &GCWriteTRQueue[chip->ChannelID][chip->ChipID];
		}
		else
		{
			return false;
		}
	}

	bool suspensionRequired = false;
	ChipStatus cs = _NVMController->GetChipStatus(chip);
	switch (cs)
	{
	case ChipStatus::IDLE:
		break;
	case ChipStatus::ERASING:
		if (!eraseSuspensionEnabled || _NVMController->HasSuspendedCommand(chip))
			return false;
		if (_NVMController->Expected_finish_time(chip) - Simulator->Time() < eraseReasonableSuspensionTimeForWrite)
			return false;
		suspensionRequired = true;
	default:
		return false;
	}

	issue_command_to_chip(sourceQueue1, sourceQueue2, Transaction_Type::WRITE, suspensionRequired);

	return true;
}

bool TSU_OutOfOrder::service_erase_transaction(NVM::FlashMemory::Flash_Chip *chip)
{
	if (_NVMController->GetChipStatus(chip) != ChipStatus::IDLE)
	{
		return false;
	}

	Flash_Transaction_Queue *source_queue = &GCEraseTRQueue[chip->ChannelID][chip->ChipID];
	if (source_queue->size() == 0)
	{
		return false;
	}

	issue_command_to_chip(source_queue, NULL, Transaction_Type::ERASE, false);

	return true;
}
} // namespace SSD_Components
