#include <stdexcept>
#include "../sim/Engine.h"
#include "NVM_PHY_ONFI_NVDDR2.h"
#include "Stats.h"

namespace SSD_Components {
	/*hack: using this style to emulate event/delegate*/
	NVM_PHY_ONFI_NVDDR2* NVM_PHY_ONFI_NVDDR2::_my_instance;

	NVM_PHY_ONFI_NVDDR2::NVM_PHY_ONFI_NVDDR2(const sim_object_id_type& id, ONFI_Channel_NVDDR2** channels,
		unsigned int ChannelCount, unsigned int chip_no_per_channel, unsigned int DieNoPerChip, unsigned int PlaneNoPerDie)
		: NVM_PHY_ONFI(id, ChannelCount, chip_no_per_channel, DieNoPerChip, PlaneNoPerDie), channels(channels)
	{
		WaitingReadTX = new Flash_Transaction_Queue[channel_count];
		WaitingGCRead_TX = new Flash_Transaction_Queue[channel_count];
		WaitingMappingRead_TX = new Flash_Transaction_Queue[channel_count];
		WaitingCopybackWrites = new std::list<DieBookKeepingEntry*>[channel_count];
		bookKeepingTable = new ChipBookKeepingEntry*[channel_count];
		for (unsigned int channelID = 0; channelID < channel_count; channelID++) {
			bookKeepingTable[channelID] = new ChipBookKeepingEntry[chip_no_per_channel];
			for (unsigned int chipID = 0; chipID < chip_no_per_channel; chipID++) {
				bookKeepingTable[channelID][chipID].Expected_command_exec_finish_time = T0; 
				bookKeepingTable[channelID][chipID].Last_transfer_finish_time = T0;
				bookKeepingTable[channelID][chipID].Die_book_keeping_records = new DieBookKeepingEntry[DieNoPerChip];
				bookKeepingTable[channelID][chipID].Status = ChipStatus::IDLE;
				bookKeepingTable[channelID][chipID].HasSuspend = false;
				bookKeepingTable[channelID][chipID].WaitingReadTXCount = 0;
				bookKeepingTable[channelID][chipID].No_of_active_dies = 0;
				for (unsigned int dieID = 0; dieID < DieNoPerChip; dieID++) {
					bookKeepingTable[channelID][chipID].Die_book_keeping_records[dieID].ActiveCommand = NULL;
					bookKeepingTable[channelID][chipID].Die_book_keeping_records[dieID].ActiveTransactions.clear();
					bookKeepingTable[channelID][chipID].Die_book_keeping_records[dieID].SuspendedCommand = NULL;
					bookKeepingTable[channelID][chipID].Die_book_keeping_records[dieID].SuspendedTransactions.clear();
					bookKeepingTable[channelID][chipID].Die_book_keeping_records[dieID].Free = true;
					bookKeepingTable[channelID][chipID].Die_book_keeping_records[dieID].Suspended = false;
					bookKeepingTable[channelID][chipID].Die_book_keeping_records[dieID].DieInterleavedTime = INVALID_TIME;
					bookKeepingTable[channelID][chipID].Die_book_keeping_records[dieID].Expected_finish_time = INVALID_TIME;
					bookKeepingTable[channelID][chipID].Die_book_keeping_records[dieID].RemainingExecTime = INVALID_TIME;
				}
			}
		}
		_my_instance = this;
	}

	void NVM_PHY_ONFI_NVDDR2::Setup_triggers()
	{
		Sim_Object::Setup_triggers();
		for (unsigned int i = 0; i < channel_count; i++) {
			for (unsigned int j = 0; j < chip_no_per_channel; j++) {
				channels[i]->Chips[j]->Connect_to_chip_ready_signal(handle_ready_signal_from_chip);
			}
		}
	}

	void NVM_PHY_ONFI_NVDDR2::Validate_simulation_config()
	{
	}

	void NVM_PHY_ONFI_NVDDR2::Start_simulation()
	{
	}

	inline BusChannelStatus NVM_PHY_ONFI_NVDDR2::Get_channel_status(flash_channel_ID_type channelID)
	{
		return channels[channelID]->GetStatus();
	}

	inline NVM::FlashMemory::Flash_Chip* NVM_PHY_ONFI_NVDDR2::Get_chip(flash_channel_ID_type channelID, flash_chip_ID_type chipID)
	{
		return channels[channelID]->Chips[chipID];
	}

	LPA_type NVM_PHY_ONFI_NVDDR2::Get_metadata(flash_channel_ID_type channe_id, flash_chip_ID_type chip_id, flash_die_ID_type die_id, flash_plane_ID_type plane_id, flash_block_ID_type block_id, flash_page_ID_type page_id)//A simplification to decrease the complexity of GC execution! The GC unit may need to know the metadata of a page to decide if a page is valid or invalid. 
	{
		return channels[channe_id]->Chips[chip_id]->Get_metadata(die_id, plane_id, block_id, page_id);
	}

	inline bool NVM_PHY_ONFI_NVDDR2::HasSuspendedCommand(NVM::FlashMemory::Flash_Chip* chip)
	{
		return bookKeepingTable[chip->ChannelID][chip->ChipID].HasSuspend;
	}

	inline ChipStatus NVM_PHY_ONFI_NVDDR2::GetChipStatus(NVM::FlashMemory::Flash_Chip* chip)
	{
		return bookKeepingTable[chip->ChannelID][chip->ChipID].Status;
	}
	
	inline sim_time_type NVM_PHY_ONFI_NVDDR2::Expected_finish_time(NVM::FlashMemory::Flash_Chip* chip)
	{
		return bookKeepingTable[chip->ChannelID][chip->ChipID].Expected_command_exec_finish_time;
	}

	sim_time_type NVM_PHY_ONFI_NVDDR2::Expected_finish_time(NVM_Transaction_Flash* transaction)
	{
		return Expected_finish_time(channels[transaction->Address.ChannelID]->Chips[transaction->Address.ChipID]);
	}


	sim_time_type NVM_PHY_ONFI_NVDDR2::Expected_transfer_time(NVM_Transaction_Flash* transaction)
	{
		return NVDDR2DataInTransferTime(transaction->Data_and_metadata_size_in_byte, channels[transaction->Address.ChannelID]);
	}

	NVM_Transaction_Flash* NVM_PHY_ONFI_NVDDR2::Is_chip_busy_with_stream(NVM_Transaction_Flash* transaction)
	{
		ChipBookKeepingEntry* chipBKE = &bookKeepingTable[transaction->Address.ChannelID][transaction->Address.ChipID];
		stream_id_type stream_id = transaction->Stream_id;

		for (unsigned int die_id = 0; die_id < die_no_per_chip; die_id++) {
			for (auto &tr : chipBKE->Die_book_keeping_records[die_id].ActiveTransactions) {
				if (tr->Stream_id == stream_id) {
					return tr;
				}
			}
		}

		return NULL;
	}

	bool NVM_PHY_ONFI_NVDDR2::Is_chip_busy(NVM_Transaction_Flash* transaction)
	{
		ChipBookKeepingEntry* chipBKE = &bookKeepingTable[transaction->Address.ChannelID][transaction->Address.ChipID];
		return (chipBKE->Status != ChipStatus::IDLE);
	}

	void NVM_PHY_ONFI_NVDDR2::Change_flash_page_status_for_preconditioning(const NVM::FlashMemory::Physical_Page_Address& page_address, const LPA_type lpa)
	{
		channels[page_address.ChannelID]->Chips[page_address.ChipID]->Change_memory_status_preconditioning(&page_address, &lpa);
	}
	
	void NVM_PHY_ONFI_NVDDR2::Send_command_to_chip(std::list<NVM_Transaction_Flash*>& transaction_list)
	{
		ONFI_Channel_NVDDR2* target_channel = channels[transaction_list.front()->Address.ChannelID];

		NVM::FlashMemory::Flash_Chip* targetChip = target_channel->Chips[transaction_list.front()->Address.ChipID];
		ChipBookKeepingEntry* chipBKE = &bookKeepingTable[transaction_list.front()->Address.ChannelID][transaction_list.front()->Address.ChipID];
		DieBookKeepingEntry* dieBKE = &chipBKE->Die_book_keeping_records[transaction_list.front()->Address.DieID];

		/*If this is not a die-interleaved command execution, and the channel is already busy,
		* then something illegarl is happening*/
		if (target_channel->GetStatus() == BusChannelStatus::BUSY && chipBKE->OngoingDieCMDTransfers.size() == 0) {
			PRINT_ERROR("Bus " << target_channel->ChannelID << ": starting communication on a busy flash channel!");
		}

		sim_time_type suspendTime = 0;
		if (!dieBKE->Free) {
			if (transaction_list.front()->SuspendRequired) {
				switch (dieBKE->ActiveTransactions.front()->Type) {
					case Transaction_Type::WRITE:
						Stats::IssuedSuspendProgramCMD++;
						suspendTime = target_channel->ProgramSuspendCommandTime + targetChip->GetSuspendProgramTime();
						break;
					case Transaction_Type::ERASE:
						Stats::IssuedSuspendEraseCMD++;
						suspendTime = target_channel->EraseSuspendCommandTime + targetChip->GetSuspendEraseTime();
						break;
					default:
						PRINT_ERROR("Read suspension is not supported!")
				}
				targetChip->Suspend(transaction_list.front()->Address.DieID);
				dieBKE->PrepareSuspend();
				if (chipBKE->OngoingDieCMDTransfers.size()) {
					chipBKE->PrepareSuspend();
				}
			} else {
				PRINT_ERROR("Read suspension is not supported!")
			}
		}

		dieBKE->Free = false;
		dieBKE->ActiveCommand = new NVM::FlashMemory::Flash_Command();
		for (std::list<NVM_Transaction_Flash*>::iterator it = transaction_list.begin();
			it != transaction_list.end(); it++) {
			dieBKE->ActiveTransactions.push_back(*it);
			dieBKE->ActiveCommand->Address.push_back((*it)->Address);
			NVM::FlashMemory::PageMetadata metadata;
			metadata.LPA = (*it)->LPA;
			dieBKE->ActiveCommand->Meta_data.push_back(metadata);
		}

		switch (transaction_list.front()->Type) {
			case Transaction_Type::READ:
				if (transaction_list.size() == 1) {
					Stats::IssuedReadCMD++;
					dieBKE->ActiveCommand->CommandCode = CMD_READ_PAGE;
					DEBUG("Chip " << targetChip->ChannelID << ", " << targetChip->ChipID << ", " << transaction_list.front()->Address.DieID << ": Sending read command to chip for LPA: " << transaction_list.front()->LPA)
				} else {
					Stats::IssuedMultiplaneReadCMD++;
					dieBKE->ActiveCommand->CommandCode = CMD_READ_PAGE_MULTIPLANE;
					DEBUG("Chip " << targetChip->ChannelID << ", " << targetChip->ChipID << ", " << transaction_list.front()->Address.DieID << ": Sending multi-plane read command to chip for LPA: " << transaction_list.front()->LPA)
				}

				for (std::list<NVM_Transaction_Flash*>::iterator it = transaction_list.begin();
					it != transaction_list.end(); it++) {
					(*it)->STAT_transfer_time += target_channel->ReadCommandTime[transaction_list.size()];
				}
				if (chipBKE->OngoingDieCMDTransfers.size() == 0) {
					targetChip->StartCMDXfer();
					chipBKE->Status = ChipStatus::CMD_IN;
					chipBKE->Last_transfer_finish_time = Simulator->Time() + suspendTime + target_channel->ReadCommandTime[transaction_list.size()];
					Simulator->Register_sim_event(Simulator->Time() + suspendTime + target_channel->ReadCommandTime[transaction_list.size()], this,
						dieBKE, (int)NVDDR2_SimEventType::READ_CMD_ADDR_TRANSFERRED);
				} else {
					dieBKE->DieInterleavedTime = suspendTime + target_channel->ReadCommandTime[transaction_list.size()];
					chipBKE->Last_transfer_finish_time += suspendTime + target_channel->ReadCommandTime[transaction_list.size()];
				}
				chipBKE->OngoingDieCMDTransfers.push(dieBKE);

				dieBKE->Expected_finish_time = chipBKE->Last_transfer_finish_time + targetChip->Get_command_execution_latency(dieBKE->ActiveCommand->CommandCode, dieBKE->ActiveCommand->Address[0].PageID);
				if (chipBKE->Expected_command_exec_finish_time < dieBKE->Expected_finish_time) {
					chipBKE->Expected_command_exec_finish_time = dieBKE->Expected_finish_time;
				}
				break;
			case Transaction_Type::WRITE:
				if (((NVM_Transaction_Flash_WR*)transaction_list.front())->ExecutionMode == WriteExecutionModeType::SIMPLE) {
					if (transaction_list.size() == 1) {
						Stats::IssuedProgramCMD++;
						dieBKE->ActiveCommand->CommandCode = CMD_PROGRAM_PAGE;
						DEBUG("Chip " << targetChip->ChannelID << ", " << targetChip->ChipID << ", " << transaction_list.front()->Address.DieID << ": Sending program command to chip for LPA: " << transaction_list.front()->LPA)
					} else {
						Stats::IssuedMultiplaneProgramCMD++;
						dieBKE->ActiveCommand->CommandCode = CMD_PROGRAM_PAGE_MULTIPLANE;
						DEBUG("Chip " << targetChip->ChannelID << ", " << targetChip->ChipID << ", " << transaction_list.front()->Address.DieID << ": Sending multi-plane program command to chip for LPA: " << transaction_list.front()->LPA)
					}

					sim_time_type data_transfer_time = 0;

					for (std::list<NVM_Transaction_Flash*>::iterator it = transaction_list.begin();
						it != transaction_list.end(); it++) {
						(*it)->STAT_transfer_time += target_channel->ProgramCommandTime[transaction_list.size()] + NVDDR2DataInTransferTime((*it)->Data_and_metadata_size_in_byte, target_channel);
						data_transfer_time += NVDDR2DataInTransferTime((*it)->Data_and_metadata_size_in_byte, target_channel);
					}
					if (chipBKE->OngoingDieCMDTransfers.size() == 0) {
						targetChip->StartCMDDataInXfer();
						chipBKE->Status = ChipStatus::CMD_DATA_IN;
						chipBKE->Last_transfer_finish_time = Simulator->Time() + suspendTime + target_channel->ProgramCommandTime[transaction_list.size()] + data_transfer_time;
						Simulator->Register_sim_event(Simulator->Time() + suspendTime + target_channel->ProgramCommandTime[transaction_list.size()] + data_transfer_time,
							this, dieBKE, (int)NVDDR2_SimEventType::PROGRAM_CMD_ADDR_DATA_TRANSFERRED);
					} else {
						dieBKE->DieInterleavedTime = suspendTime + target_channel->ProgramCommandTime[transaction_list.size()] + data_transfer_time;
						chipBKE->Last_transfer_finish_time += suspendTime + target_channel->ProgramCommandTime[transaction_list.size()] + data_transfer_time;
					}
					chipBKE->OngoingDieCMDTransfers.push(dieBKE);

					dieBKE->Expected_finish_time = chipBKE->Last_transfer_finish_time + targetChip->Get_command_execution_latency(dieBKE->ActiveCommand->CommandCode, dieBKE->ActiveCommand->Address[0].PageID);
					if (chipBKE->Expected_command_exec_finish_time < dieBKE->Expected_finish_time) {
						chipBKE->Expected_command_exec_finish_time = dieBKE->Expected_finish_time;
					}
				} else {
					//Copyback write for GC

					if (transaction_list.size() == 1) {
						Stats::IssuedCopybackReadCMD++;
						dieBKE->ActiveCommand->CommandCode = CMD_READ_PAGE_COPYBACK;
					} else {
						Stats::IssuedMultiplaneCopybackProgramCMD++;
						dieBKE->ActiveCommand->CommandCode = CMD_READ_PAGE_COPYBACK_MULTIPLANE;
					}

					for (std::list<NVM_Transaction_Flash*>::iterator it = transaction_list.begin();
						it != transaction_list.end(); it++) {
						(*it)->STAT_transfer_time += target_channel->ReadCommandTime[transaction_list.size()];
					}
					if (chipBKE->OngoingDieCMDTransfers.size() == 0) {
						targetChip->StartCMDXfer();
						chipBKE->Status = ChipStatus::CMD_IN;
						chipBKE->Last_transfer_finish_time = Simulator->Time() + suspendTime + target_channel->ReadCommandTime[transaction_list.size()];
						Simulator->Register_sim_event(Simulator->Time() + suspendTime + target_channel->ReadCommandTime[transaction_list.size()], this,
							dieBKE, (int)NVDDR2_SimEventType::READ_CMD_ADDR_TRANSFERRED);
					} else {
						dieBKE->DieInterleavedTime = suspendTime + target_channel->ReadCommandTime[transaction_list.size()];
						chipBKE->Last_transfer_finish_time += suspendTime + target_channel->ReadCommandTime[transaction_list.size()];
					}
					chipBKE->OngoingDieCMDTransfers.push(dieBKE);

					dieBKE->Expected_finish_time = chipBKE->Last_transfer_finish_time + targetChip->Get_command_execution_latency(dieBKE->ActiveCommand->CommandCode, dieBKE->ActiveCommand->Address[0].PageID);
					if (chipBKE->Expected_command_exec_finish_time < dieBKE->Expected_finish_time) {
						chipBKE->Expected_command_exec_finish_time = dieBKE->Expected_finish_time;
					}
				}
				break;
			case Transaction_Type::ERASE:
				//DEBUG2("Chip " << targetChip->ChannelID << ", " << targetChip->ChipID << ", " << transaction_list.front()->Address.DieID << ": Sending erase command to chip")
				if (transaction_list.size() == 1) {
					Stats::IssuedEraseCMD++;
					dieBKE->ActiveCommand->CommandCode = CMD_ERASE_BLOCK;
				} else {
					Stats::IssuedMultiplaneEraseCMD++;
					dieBKE->ActiveCommand->CommandCode = CMD_ERASE_BLOCK_MULTIPLANE;
				}

				for (std::list<NVM_Transaction_Flash*>::iterator it = transaction_list.begin();
					it != transaction_list.end(); it++) {
					(*it)->STAT_transfer_time += target_channel->EraseCommandTime[transaction_list.size()];
				}
				if (chipBKE->OngoingDieCMDTransfers.size() == 0) {
					targetChip->StartCMDXfer();
					chipBKE->Status = ChipStatus::CMD_IN;
					chipBKE->Last_transfer_finish_time = Simulator->Time() + suspendTime + target_channel->EraseCommandTime[transaction_list.size()];
					Simulator->Register_sim_event(Simulator->Time() + suspendTime + target_channel->EraseCommandTime[transaction_list.size()],
						this, dieBKE, (int)NVDDR2_SimEventType::ERASE_SETUP_COMPLETED);
				} else {
					dieBKE->DieInterleavedTime = suspendTime + target_channel->EraseCommandTime[transaction_list.size()];
					chipBKE->Last_transfer_finish_time += suspendTime + target_channel->EraseCommandTime[transaction_list.size()];
				}
				chipBKE->OngoingDieCMDTransfers.push(dieBKE);

				dieBKE->Expected_finish_time = chipBKE->Last_transfer_finish_time + targetChip->Get_command_execution_latency(dieBKE->ActiveCommand->CommandCode, dieBKE->ActiveCommand->Address[0].PageID);
				if (chipBKE->Expected_command_exec_finish_time < dieBKE->Expected_finish_time) {
					chipBKE->Expected_command_exec_finish_time = dieBKE->Expected_finish_time;
				}
				break;
			default:
				throw std::invalid_argument("NVM_PHY_ONFI_NVDDR2: Unhandled event specified!");
		}

		target_channel->SetStatus(BusChannelStatus::BUSY, targetChip);
	}

	void NVM_PHY_ONFI_NVDDR2::Change_memory_status_preconditioning(const NVM::NVM_Memory_Address* address, const void* status_info)
	{
		channels[((NVM::FlashMemory::Physical_Page_Address*)address)->ChannelID]->Chips[((NVM::FlashMemory::Physical_Page_Address*)address)->ChipID]->Change_memory_status_preconditioning(address, status_info);
	}

	void copy_read_data_to_transaction(NVM_Transaction_Flash_RD* read_transaction, NVM::FlashMemory::Flash_Command* command)
	{
		int i = 0;
		for (auto &address : command->Address) {
			if (address.PlaneID == read_transaction->Address.PlaneID) {
				read_transaction->LPA = command->Meta_data[i].LPA;
			}
			i++;
		}
	}

	void NVM_PHY_ONFI_NVDDR2::Execute_simulator_event(MQSimEngine::Sim_Event* ev)
	{
		DieBookKeepingEntry* dieBKE = (DieBookKeepingEntry*)ev->Parameters;
		flash_channel_ID_type channel_id = dieBKE->ActiveTransactions.front()->Address.ChannelID;
		ONFI_Channel_NVDDR2* targetChannel = channels[channel_id];
		NVM::FlashMemory::Flash_Chip* targetChip = targetChannel->Chips[dieBKE->ActiveTransactions.front()->Address.ChipID];
		ChipBookKeepingEntry *chipBKE = &bookKeepingTable[channel_id][targetChip->ChipID];

		switch ((NVDDR2_SimEventType)ev->Type) {
			case NVDDR2_SimEventType::READ_CMD_ADDR_TRANSFERRED:
				//DEBUG2("Chip " << targetChip->ChannelID << ", " << targetChip->ChipID << ", " << dieBKE->ActiveTransactions.front()->Address.DieID << ": READ_CMD_ADDR_TRANSFERRED ")
				targetChip->EndCMDXfer(dieBKE->ActiveCommand);
				for (auto tr : dieBKE->ActiveTransactions) {
					tr->STAT_execution_time = dieBKE->Expected_finish_time - Simulator->Time();
				}
				chipBKE->OngoingDieCMDTransfers.pop();
				chipBKE->No_of_active_dies++;
				if (chipBKE->OngoingDieCMDTransfers.size() > 0) {
					perform_interleaved_cmd_data_transfer(targetChip, chipBKE->OngoingDieCMDTransfers.front());
					return;
				} else {
					chipBKE->Status = ChipStatus::READING;
					targetChannel->SetStatus(BusChannelStatus::IDLE, targetChip);
				}
				break;
			case NVDDR2_SimEventType::ERASE_SETUP_COMPLETED:
				//DEBUG2("Chip " << targetChip->ChannelID << ", " << targetChip->ChipID << ", " << dieBKE->ActiveTransactions.front()->Address.DieID << ": ERASE_SETUP_COMPLETED ")
				targetChip->EndCMDXfer(dieBKE->ActiveCommand);
				for (auto &tr : dieBKE->ActiveTransactions) {
					tr->STAT_execution_time = dieBKE->Expected_finish_time - Simulator->Time();
				}
				chipBKE->OngoingDieCMDTransfers.pop();
				chipBKE->No_of_active_dies++;
				if (chipBKE->OngoingDieCMDTransfers.size() > 0) {
					perform_interleaved_cmd_data_transfer(targetChip, chipBKE->OngoingDieCMDTransfers.front());
					return;
				} else {
					chipBKE->Status = ChipStatus::ERASING;
					targetChannel->SetStatus(BusChannelStatus::IDLE, targetChip);
				}
				break;
			case NVDDR2_SimEventType::PROGRAM_CMD_ADDR_DATA_TRANSFERRED:
			case NVDDR2_SimEventType::PROGRAM_COPYBACK_CMD_ADDR_TRANSFERRED:
				//DEBUG2("Chip " << targetChip->ChannelID << ", " << targetChip->ChipID << ", " << dieBKE->ActiveTransactions.front()->Address.DieID <<  ": PROGRAM_CMD_ADDR_DATA_TRANSFERRED " )
				targetChip->EndCMDDataInXfer(dieBKE->ActiveCommand);
				for (auto &tr : dieBKE->ActiveTransactions) {
					tr->STAT_execution_time = dieBKE->Expected_finish_time - Simulator->Time();
				}
				chipBKE->OngoingDieCMDTransfers.pop();
				chipBKE->No_of_active_dies++;
				if (chipBKE->OngoingDieCMDTransfers.size() > 0)
				{
					perform_interleaved_cmd_data_transfer(targetChip, chipBKE->OngoingDieCMDTransfers.front());
					return;
				} else {
					chipBKE->Status = ChipStatus::WRITING;
					targetChannel->SetStatus(BusChannelStatus::IDLE, targetChip);
				}
				break;
			case NVDDR2_SimEventType::READ_DATA_TRANSFERRED:
				//DEBUG2("Chip " << targetChip->ChannelID << ", " << targetChip->ChipID << ", " << dieBKE->ActiveTransactions.front()->Address.DieID << ": READ_DATA_TRANSFERRED ")
				targetChip->EndDataOutXfer(dieBKE->ActiveCommand);
				copy_read_data_to_transaction((NVM_Transaction_Flash_RD*)dieBKE->ActiveTransfer, dieBKE->ActiveCommand);
	#if 0
				if (tr->ExecutionMode != ExecutionModeType::COPYBACK)
	#endif
				broadcastTransactionServicedSignal(dieBKE->ActiveTransfer);

				for (std::list<NVM_Transaction_Flash*>::iterator it = dieBKE->ActiveTransactions.begin();
					it != dieBKE->ActiveTransactions.end(); it++) {
					if ((*it) == dieBKE->ActiveTransfer) {
						dieBKE->ActiveTransactions.erase(it);
						break;
					}
				}
				dieBKE->ActiveTransfer = NULL;
				if (dieBKE->ActiveTransactions.size() == 0) {
					dieBKE->ClearCommand();
				}

				chipBKE->WaitingReadTXCount--;
				if (chipBKE->No_of_active_dies == 0) {
					if (chipBKE->WaitingReadTXCount == 0) {
						chipBKE->Status = ChipStatus::IDLE;
					} else {
						chipBKE->Status = ChipStatus::WAIT_FOR_DATA_OUT;
					}
				}
				if (chipBKE->Status == ChipStatus::IDLE) {
					if (dieBKE->Suspended) {
						send_resume_command_to_chip(targetChip, chipBKE);
					}
				}
				targetChannel->SetStatus(BusChannelStatus::IDLE, targetChip);
				break;
			default:
				PRINT_ERROR("Unknown simulation event specified for NVM_PHY_ONFI_NVDDR2!")
		}

		/* Copyback requests are prioritized over other type of requests since they need very short transfer time.
		In addition, they are just used for GC purpose. */
		if (WaitingCopybackWrites[channel_id].size() > 0) {
			DieBookKeepingEntry* waitingBKE = WaitingCopybackWrites[channel_id].front();
			targetChip = channels[channel_id]->Chips[waitingBKE->ActiveTransactions.front()->Address.ChipID];
			ChipBookKeepingEntry* waitingChipBKE = &bookKeepingTable[channel_id][targetChip->ChipID];
			if (waitingBKE->ActiveTransactions.size() > 1) {
				Stats::IssuedMultiplaneCopybackProgramCMD++;
				waitingBKE->ActiveCommand->CommandCode = CMD_PROGRAM_PAGE_COPYBACK_MULTIPLANE;
			} else {
				Stats::IssuedCopybackProgramCMD++;
				waitingBKE->ActiveCommand->CommandCode = CMD_PROGRAM_PAGE_COPYBACK;
			}
			targetChip->StartCMDXfer();
			waitingChipBKE->Status = ChipStatus::CMD_IN;
			Simulator->Register_sim_event(Simulator->Time() + this->channels[channel_id]->ProgramCommandTime[waitingBKE->ActiveTransactions.size()],
				this, waitingBKE, (int)NVDDR2_SimEventType::PROGRAM_COPYBACK_CMD_ADDR_TRANSFERRED);
			waitingChipBKE->OngoingDieCMDTransfers.push(waitingBKE);

			waitingBKE->Expected_finish_time = Simulator->Time() + this->channels[channel_id]->ProgramCommandTime[waitingBKE->ActiveTransactions.size()]
				+ targetChip->Get_command_execution_latency(waitingBKE->ActiveCommand->CommandCode, waitingBKE->ActiveCommand->Address[0].PageID);
			if (waitingChipBKE->Expected_command_exec_finish_time < waitingBKE->Expected_finish_time) {
				waitingChipBKE->Expected_command_exec_finish_time = waitingBKE->Expected_finish_time;
			}

			WaitingCopybackWrites[channel_id].pop_front();
			channels[channel_id]->SetStatus(BusChannelStatus::BUSY, targetChip);

			return;
		} else if (WaitingMappingRead_TX[channel_id].size() > 0) {
			NVM_Transaction_Flash_RD* waitingTR = (NVM_Transaction_Flash_RD*)WaitingMappingRead_TX[channel_id].front();
			WaitingMappingRead_TX[channel_id].pop_front();
			transfer_read_data_from_chip(&bookKeepingTable[channel_id][waitingTR->Address.ChipID],
				&(bookKeepingTable[channel_id][waitingTR->Address.ChipID].Die_book_keeping_records[waitingTR->Address.DieID]), waitingTR);

			return;
		} else if (WaitingReadTX[channel_id].size() > 0) {
			NVM_Transaction_Flash_RD* waitingTR = (NVM_Transaction_Flash_RD*)WaitingReadTX[channel_id].front();
			WaitingReadTX[channel_id].pop_front();
			transfer_read_data_from_chip(&bookKeepingTable[channel_id][waitingTR->Address.ChipID],
				&(bookKeepingTable[channel_id][waitingTR->Address.ChipID].Die_book_keeping_records[waitingTR->Address.DieID]), waitingTR);

			return;
		} else if (WaitingGCRead_TX[channel_id].size() > 0) {
			NVM_Transaction_Flash_RD* waitingTR = (NVM_Transaction_Flash_RD*)WaitingGCRead_TX[channel_id].front();
			WaitingGCRead_TX[channel_id].pop_front();
			transfer_read_data_from_chip(&bookKeepingTable[channel_id][waitingTR->Address.ChipID],
				&(bookKeepingTable[channel_id][waitingTR->Address.ChipID].Die_book_keeping_records[waitingTR->Address.DieID]), waitingTR);
			return;
		}

		//If the execution reaches here, then the bus channel became idle
		broadcastChannelIdleSignal(channel_id);
	}

	inline void NVM_PHY_ONFI_NVDDR2::handle_ready_signal_from_chip(NVM::FlashMemory::Flash_Chip* chip, NVM::FlashMemory::Flash_Command* command)
	{
		ChipBookKeepingEntry *chipBKE = &_my_instance->bookKeepingTable[chip->ChannelID][chip->ChipID];
		DieBookKeepingEntry *dieBKE = &(chipBKE->Die_book_keeping_records[command->Address[0].DieID]);

		switch (command->CommandCode)
		{
		case CMD_READ_PAGE:
		case CMD_READ_PAGE_MULTIPLANE:
			DEBUG("Chip " << chip->ChannelID << ", " << chip->ChipID << ": finished  read command")
			chipBKE->No_of_active_dies--;
			if (chipBKE->No_of_active_dies == 0)//After finishing the last command, the chip state is changed
				chipBKE->Status = ChipStatus::WAIT_FOR_DATA_OUT;

			for (std::list<NVM_Transaction_Flash*>::iterator it = dieBKE->ActiveTransactions.begin();
				it != dieBKE->ActiveTransactions.end(); it++)
			{
				chipBKE->WaitingReadTXCount++;
				if (_my_instance->channels[chip->ChannelID]->GetStatus() == BusChannelStatus::IDLE)
					_my_instance->transfer_read_data_from_chip(chipBKE, dieBKE, (*it));
				else
				{
					switch (dieBKE->ActiveTransactions.front()->Source)
					{
					case Transaction_Source_Type::CACHE:
					case Transaction_Source_Type::USERIO:
						_my_instance->WaitingReadTX[chip->ChannelID].push_back((*it));
						break;
					case Transaction_Source_Type::GC_WL:
						_my_instance->WaitingGCRead_TX[chip->ChannelID].push_back((*it));
						break;
					case Transaction_Source_Type::MAPPING:
						_my_instance->WaitingMappingRead_TX[chip->ChannelID].push_back((*it));
						break;
					}
				}
			}
			break;
		case CMD_READ_PAGE_COPYBACK:
		case CMD_READ_PAGE_COPYBACK_MULTIPLANE:
			chipBKE->No_of_active_dies--;
			if (chipBKE->No_of_active_dies == 0)
				chipBKE->Status = ChipStatus::WAIT_FOR_COPYBACK_CMD;
			if (_my_instance->channels[chip->ChannelID]->GetStatus() == BusChannelStatus::IDLE)
			{
				if (dieBKE->ActiveTransactions.size() > 1)
				{
					Stats::IssuedMultiplaneCopybackProgramCMD++;
					dieBKE->ActiveCommand->CommandCode = CMD_PROGRAM_PAGE_COPYBACK_MULTIPLANE;
				}
				else
				{
					Stats::IssuedCopybackProgramCMD++;
					dieBKE->ActiveCommand->CommandCode = CMD_PROGRAM_PAGE_COPYBACK;
				}

				for (std::list<NVM_Transaction_Flash*>::iterator it = dieBKE->ActiveTransactions.begin();
					it != dieBKE->ActiveTransactions.end(); it++)
				{
					(*it)->STAT_transfer_time += _my_instance->channels[chip->ChannelID]->ProgramCommandTime[dieBKE->ActiveTransactions.size()];
				}
				chip->StartCMDXfer();
				chipBKE->Status = ChipStatus::CMD_IN;
				Simulator->Register_sim_event(Simulator->Time() + _my_instance->channels[chip->ChannelID]->ProgramCommandTime[dieBKE->ActiveTransactions.size()],
					_my_instance, dieBKE, (int)NVDDR2_SimEventType::PROGRAM_COPYBACK_CMD_ADDR_TRANSFERRED);
				chipBKE->OngoingDieCMDTransfers.push(dieBKE);
				_my_instance->channels[chip->ChannelID]->SetStatus(BusChannelStatus::BUSY, chip);

				dieBKE->Expected_finish_time = Simulator->Time() + _my_instance->channels[chip->ChannelID]->ProgramCommandTime[dieBKE->ActiveTransactions.size()]
					+ chip->Get_command_execution_latency(dieBKE->ActiveCommand->CommandCode, dieBKE->ActiveCommand->Address[0].PageID);
				if (chipBKE->Expected_command_exec_finish_time < dieBKE->Expected_finish_time)
					chipBKE->Expected_command_exec_finish_time = dieBKE->Expected_finish_time;
#if 0	
				//Copyback data should be read out in order to get rid of bit error propagation
				Simulator->RegisterEvent(Simulator->Time() + channels[targetChip->ChannelID]->ProgramCommandTime + NVDDR2DataOutTransferTime(targetTransaction->SizeInByte, channels[targetChip->ChannelID]),
					this, targetTransaction, (int)NVDDR2_SimEventType::READ_DATA_TRANSFERRED);
				targetTransaction->STAT_TransferTime += NVDDR2DataOutTransferTime(targetTransaction->SizeInByte, channels[targetChip->ChannelID]);
#endif
			}
			else _my_instance->WaitingCopybackWrites->push_back(dieBKE);
			break;
		case CMD_PROGRAM_PAGE:
		case CMD_PROGRAM_PAGE_MULTIPLANE:
		case CMD_PROGRAM_PAGE_COPYBACK:
		case CMD_PROGRAM_PAGE_COPYBACK_MULTIPLANE:
		{
			DEBUG("Chip " << chip->ChannelID << ", " << chip->ChipID << ": finished program command")
			int i = 0;
			for (std::list<NVM_Transaction_Flash*>::iterator it = dieBKE->ActiveTransactions.begin();
				it != dieBKE->ActiveTransactions.end(); it++, i++)
			{
				((NVM_Transaction_Flash_WR*)(*it))->Content = command->Meta_data[i].LPA;
				_my_instance->broadcastTransactionServicedSignal(*it);
			}
			dieBKE->ActiveTransactions.clear();
			dieBKE->ClearCommand();

			chipBKE->No_of_active_dies--;
			if (chipBKE->No_of_active_dies == 0 && chipBKE->WaitingReadTXCount == 0)
				chipBKE->Status = ChipStatus::IDLE;
			//Since the time required to send the resume command is very small, we ignore it
			if (chipBKE->Status == ChipStatus::IDLE)
				if (chipBKE->HasSuspend)
					_my_instance->send_resume_command_to_chip(chip, chipBKE);
			break;
		}
		case CMD_ERASE_BLOCK:
		case CMD_ERASE_BLOCK_MULTIPLANE:
			DEBUG("Chip " << chip->ChannelID << ", " << chip->ChipID << ": finished erase command")
			for (std::list<NVM_Transaction_Flash*>::iterator it = dieBKE->ActiveTransactions.begin();
				it != dieBKE->ActiveTransactions.end(); it++)
				_my_instance->broadcastTransactionServicedSignal(*it);
			dieBKE->ActiveTransactions.clear();
			dieBKE->ClearCommand();

			chipBKE->No_of_active_dies--;
			if (chipBKE->No_of_active_dies == 0 && chipBKE->WaitingReadTXCount == 0)
				chipBKE->Status = ChipStatus::IDLE;
			//Since the time required to send the resume command is very small, we ignore it
			if (chipBKE->Status == ChipStatus::IDLE)
				if (chipBKE->HasSuspend)
					_my_instance->send_resume_command_to_chip(chip, chipBKE);
			break;
		default:
			break;
		}

		if (_my_instance->channels[chip->ChannelID]->GetStatus() == BusChannelStatus::IDLE)
			_my_instance->broadcastChannelIdleSignal(chip->ChannelID);
		else if (chipBKE->Status == ChipStatus::IDLE)
			_my_instance->broadcastChipIdleSignal(chip);
	}

	inline void NVM_PHY_ONFI_NVDDR2::transfer_read_data_from_chip(ChipBookKeepingEntry* chipBKE, DieBookKeepingEntry* dieBKE, NVM_Transaction_Flash* tr)
	{
		//DEBUG2("Chip " << tr->Address.ChannelID << ", " << tr->Address.ChipID << ": transfer read data started for LPA: " << tr->LPA)
		dieBKE->ActiveTransfer = tr;
		channels[tr->Address.ChannelID]->Chips[tr->Address.ChipID]->StartDataOutXfer();
		chipBKE->Status = ChipStatus::DATA_OUT;
		Simulator->Register_sim_event(Simulator->Time() + NVDDR2DataOutTransferTime(tr->Data_and_metadata_size_in_byte, channels[tr->Address.ChannelID]),
			this, dieBKE, (int)NVDDR2_SimEventType::READ_DATA_TRANSFERRED);

		tr->STAT_transfer_time += NVDDR2DataOutTransferTime(tr->Data_and_metadata_size_in_byte, channels[tr->Address.ChannelID]);
		channels[tr->Address.ChannelID]->SetStatus(BusChannelStatus::BUSY, channels[tr->Address.ChannelID]->Chips[tr->Address.ChipID]);
	}

	void NVM_PHY_ONFI_NVDDR2::perform_interleaved_cmd_data_transfer(NVM::FlashMemory::Flash_Chip* chip, DieBookKeepingEntry* bookKeepingEntry)
	{
		ONFI_Channel_NVDDR2* target_channel = channels[bookKeepingEntry->ActiveTransactions.front()->Address.ChannelID];
		/*if (target_channel->Status == BusChannelStatus::BUSY)
			PRINT_ERROR("Requesting communication on a busy bus!")*/

		switch (bookKeepingEntry->ActiveTransactions.front()->Type)
		{
			case Transaction_Type::READ:
				chip->StartCMDXfer();
				bookKeepingTable[chip->ChannelID][chip->ChipID].Status = ChipStatus::CMD_IN;
				Simulator->Register_sim_event(Simulator->Time() + bookKeepingEntry->DieInterleavedTime,
					this, bookKeepingEntry, (int)NVDDR2_SimEventType::READ_CMD_ADDR_TRANSFERRED);
				break;
			case Transaction_Type::WRITE:
				if (((NVM_Transaction_Flash_WR*)bookKeepingEntry->ActiveTransactions.front())->RelatedRead == NULL) {
					chip->StartCMDDataInXfer();
					bookKeepingTable[chip->ChannelID][chip->ChipID].Status = ChipStatus::CMD_DATA_IN;
					Simulator->Register_sim_event(Simulator->Time() + bookKeepingEntry->DieInterleavedTime,
						this, bookKeepingEntry, (int)NVDDR2_SimEventType::PROGRAM_CMD_ADDR_DATA_TRANSFERRED);
				} else {
					chip->StartCMDXfer();
					bookKeepingTable[chip->ChannelID][chip->ChipID].Status = ChipStatus::CMD_IN;
					Simulator->Register_sim_event(Simulator->Time() + bookKeepingEntry->DieInterleavedTime, this,
						bookKeepingEntry, (int)NVDDR2_SimEventType::READ_CMD_ADDR_TRANSFERRED);
				}
				break;
			case Transaction_Type::ERASE:
				chip->StartCMDXfer();
				bookKeepingTable[chip->ChannelID][chip->ChipID].Status = ChipStatus::CMD_IN;
				Simulator->Register_sim_event(Simulator->Time() + bookKeepingEntry->DieInterleavedTime,
					this, bookKeepingEntry, (int)NVDDR2_SimEventType::ERASE_SETUP_COMPLETED);
				break;
			default:
				PRINT_ERROR("NVMController_NVDDR2: Uknown flash transaction type!")
		}
		target_channel->SetStatus(BusChannelStatus::BUSY, chip);
	}

	inline void NVM_PHY_ONFI_NVDDR2::send_resume_command_to_chip(NVM::FlashMemory::Flash_Chip* chip, ChipBookKeepingEntry* chipBKE)
	{
		//DEBUG2("Chip " << chip->ChannelID << ", " << chip->ChipID << ": resume command " )
		for (unsigned int i = 0; i < die_no_per_chip; i++) {
			DieBookKeepingEntry *dieBKE = &chipBKE->Die_book_keeping_records[i];
			//Since the time required to send the resume command is very small, MQSim ignores it to simplify the simulation
			dieBKE->PrepareResume();
			chipBKE->PrepareResume();
			chip->Resume(dieBKE->ActiveCommand->Address[0].DieID);
			switch (dieBKE->ActiveCommand->CommandCode) {
				case CMD_READ_PAGE:
				case CMD_READ_PAGE_MULTIPLANE:
				case CMD_READ_PAGE_COPYBACK:
				case CMD_READ_PAGE_COPYBACK_MULTIPLANE:
					chipBKE->Status = ChipStatus::READING;
					break;
				case CMD_PROGRAM_PAGE:
				case CMD_PROGRAM_PAGE_MULTIPLANE:
				case CMD_PROGRAM_PAGE_COPYBACK:
				case CMD_PROGRAM_PAGE_COPYBACK_MULTIPLANE:
					chipBKE->Status = ChipStatus::WRITING;
					break;
				case CMD_ERASE_BLOCK:
				case CMD_ERASE_BLOCK_MULTIPLANE:
					chipBKE->Status = ChipStatus::ERASING;
					break;
			}

		}
	}
}
