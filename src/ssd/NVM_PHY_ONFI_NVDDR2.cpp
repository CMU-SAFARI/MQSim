#include <stdexcept>
#include "../sim/Engine.h"
#include "NVM_PHY_ONFI_NVDDR2.h"
#include "Stats.h"

namespace SSD_Components {
	/*hack: using this style to emulate event/delegate*/
	NVM_PHY_ONFI_NVDDR2* NVM_PHY_ONFI_NVDDR2::_myInstance;

	NVM_PHY_ONFI_NVDDR2::NVM_PHY_ONFI_NVDDR2(const sim_object_id_type& id, ONFI_Channel_NVDDR2** channels,
		unsigned int ChannelCount, unsigned int ChipNoPerChannel, unsigned int DieNoPerChip, unsigned int PlaneNoPerDie)
		: NVM_PHY_ONFI(id, ChannelCount, ChipNoPerChannel, DieNoPerChip, PlaneNoPerDie), _Channels(channels)
	{
		WaitingReadTX = new FlashTransactionQueue[channel_count];
		WaitingGCRead_TX = new FlashTransactionQueue[channel_count];
		WaitingMappingRead_TX = new FlashTransactionQueue[channel_count];
		WaitingCopybackWrites = new std::list<DieBookKeepingEntry*>[channel_count];
		bookKeepingTable = new ChipBookKeepingEntry*[channel_count];
		for (unsigned int channelID = 0; channelID < channel_count; channelID++) {
			bookKeepingTable[channelID] = new ChipBookKeepingEntry[ChipNoPerChannel];
			for (unsigned int chipID = 0; chipID < ChipNoPerChannel; chipID++) {
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
					bookKeepingTable[channelID][chipID].Die_book_keeping_records[dieID].ExpectedFinishTime = INVALID_TIME;
					bookKeepingTable[channelID][chipID].Die_book_keeping_records[dieID].RemainingExecTime = INVALID_TIME;
				}
			}
		}
		_myInstance = this;
	}

	void NVM_PHY_ONFI_NVDDR2::Setup_triggers()
	{
		Sim_Object::Setup_triggers();
		for (unsigned int i = 0; i < channel_count; i++)
			for (unsigned int j = 0; j < ChipNoPerChannel; j++)
				_Channels[i]->Chips[j]->ConnectToChipReadySignal(handle_ready_signal_from_chip);
	}

	void NVM_PHY_ONFI_NVDDR2::Validate_simulation_config() {}
	void NVM_PHY_ONFI_NVDDR2::Start_simulation() {}
	
	inline BusChannelStatus NVM_PHY_ONFI_NVDDR2::GetChannelStatus(flash_channel_ID_type channelID)
	{
		return _Channels[channelID]->GetStatus();
	}

	inline NVM::FlashMemory::Chip* NVM_PHY_ONFI_NVDDR2::GetChip(flash_channel_ID_type channelID, flash_chip_ID_type chipID)
	{
		return _Channels[channelID]->Chips[chipID];
	}

	inline bool NVM_PHY_ONFI_NVDDR2::HasSuspendedCommand(NVM::FlashMemory::Chip* chip)
	{
		return bookKeepingTable[chip->ChannelID][chip->ChipID].HasSuspend;
	}

	inline ChipStatus NVM_PHY_ONFI_NVDDR2::GetChipStatus(NVM::FlashMemory::Chip* chip)
	{
		return bookKeepingTable[chip->ChannelID][chip->ChipID].Status;
	}
	
	inline sim_time_type NVM_PHY_ONFI_NVDDR2::ExpectedFinishTime(NVM::FlashMemory::Chip* chip)
	{
		return bookKeepingTable[chip->ChannelID][chip->ChipID].Expected_command_exec_finish_time;
	}
	
	void NVM_PHY_ONFI_NVDDR2::Send_command_to_chip(std::list<NVM_Transaction_Flash*>& transaction_list)
	{
		ONFI_Channel_NVDDR2* target_channel = _Channels[transaction_list.front()->Address.ChannelID];

		NVM::FlashMemory::Chip* targetChip = target_channel->Chips[transaction_list.front()->Address.ChipID];
		ChipBookKeepingEntry* chipBKE = &bookKeepingTable[transaction_list.front()->Address.ChannelID][transaction_list.front()->Address.ChipID];
		DieBookKeepingEntry* dieBKE = &chipBKE->Die_book_keeping_records[transaction_list.front()->Address.DieID];

		/*If this is not a die-interleaved command execution, and the channel is already busy,
		* then something illegarl is happening*/
		if (target_channel->GetStatus() == BusChannelStatus::BUSY && chipBKE->OngoingDieCMDTransfers.size() == 0)
			PRINT_ERROR("Requesting communication on a busy bus!")

			sim_time_type suspendTime = 0;
		if (!dieBKE->Free)
		{
			if (transaction_list.front()->SuspendRequired)
			{
				switch (dieBKE->ActiveTransactions.front()->Type)
				{
				case TransactionType::WRITE:
					Stats::IssuedSuspendProgramCMD++;
					suspendTime = target_channel->ProgramSuspendCommandTime + targetChip->GetSuspendProgramTime();
					break;
				case TransactionType::ERASE:
					Stats::IssuedSuspendEraseCMD++;
					suspendTime = target_channel->EraseSuspendCommandTime + targetChip->GetSuspendEraseTime();
					break;
				default:
					PRINT_ERROR("Read suspension is not supported!")
				}
				targetChip->Suspend(transaction_list.front()->Address.DieID);
				dieBKE->PrepareSuspend();
				if (chipBKE->OngoingDieCMDTransfers.size())
					chipBKE->PrepareSuspend();
			}
			else PRINT_ERROR("Read suspension is not supported!")
		}

		dieBKE->Free = false;
		dieBKE->ActiveCommand = new NVM::FlashMemory::Flash_Command();
		for (std::list<NVM_Transaction_Flash*>::iterator it = transaction_list.begin();
			it != transaction_list.end(); it++)
		{
			dieBKE->ActiveTransactions.push_back(*it);
			dieBKE->ActiveCommand->Address.push_back((*it)->Address);
		}

		switch (transaction_list.front()->Type)
		{
		case TransactionType::READ:
			if (transaction_list.size() == 1) {
				Stats::IssuedReadCMD++;
				dieBKE->ActiveCommand->CommandCode = CMD_READ_PAGE;
				//DEBUG2("Chip " << targetChip->ChannelID << ", " << targetChip->ChipID << ", " << transaction_list.front()->Address.DieID << ": Sending read command to chip for LPA: " << transaction_list.front()->LPA)
			}
			else
			{
				Stats::IssuedMultiplaneReadCMD++;
				dieBKE->ActiveCommand->CommandCode = CMD_READ_PAGE_MULTIPLANE;
				//DEBUG2("Chip " << targetChip->ChannelID << ", " << targetChip->ChipID << ", " << transaction_list.front()->Address.DieID << ": Sending multi-plane read command to chip for LPA: " << transaction_list.front()->LPA)
			}

			for (std::list<NVM_Transaction_Flash*>::iterator it = transaction_list.begin();
				it != transaction_list.end(); it++)
				(*it)->STAT_transfer_time += target_channel->ReadCommandTime[transaction_list.size()];
			if (chipBKE->OngoingDieCMDTransfers.size() == 0)
			{
				targetChip->StartCMDXfer();
				chipBKE->Status = ChipStatus::CMD_IN;
				chipBKE->Last_transfer_finish_time = Simulator->Time() + suspendTime + target_channel->ReadCommandTime[transaction_list.size()];
				Simulator->Register_sim_event(Simulator->Time() + suspendTime + target_channel->ReadCommandTime[transaction_list.size()], this,
					dieBKE, (int)NVDDR2_SimEventType::READ_CMD_ADDR_TRANSFERRED);
			}
			else
			{
				dieBKE->DieInterleavedTime = suspendTime + target_channel->ReadCommandTime[transaction_list.size()];
				chipBKE->Last_transfer_finish_time += suspendTime + target_channel->ReadCommandTime[transaction_list.size()];
			}
			chipBKE->OngoingDieCMDTransfers.push(dieBKE);

			dieBKE->ExpectedFinishTime = chipBKE->Last_transfer_finish_time + targetChip->Get_command_execution_latency(dieBKE->ActiveCommand->CommandCode, dieBKE->ActiveCommand->Address[0].PageID);
			if (chipBKE->Expected_command_exec_finish_time < dieBKE->ExpectedFinishTime)
				chipBKE->Expected_command_exec_finish_time = dieBKE->ExpectedFinishTime;
			break;
		case TransactionType::WRITE:
			if (((NVM_Transaction_Flash_WR*)transaction_list.front())->ExecutionMode == WriteExecutionModeType::SIMPLE)
			{
				if (transaction_list.size() == 1) {
					Stats::IssuedProgramCMD++;
					dieBKE->ActiveCommand->CommandCode = CMD_PROGRAM_PAGE;
					//DEBUG2("Chip " << targetChip->ChannelID << ", " << targetChip->ChipID << ", " << transaction_list.front()->Address.DieID << ": Sending program command to chip for LPA: " << transaction_list.front()->LPA)
				}
				else
				{
					Stats::IssuedMultiplaneProgramCMD++;
					dieBKE->ActiveCommand->CommandCode = CMD_PROGRAM_PAGE_MULTIPLANE;
					//DEBUG2("Chip " << targetChip->ChannelID << ", " << targetChip->ChipID << ", " << transaction_list.front()->Address.DieID << ": Sending multi-plane program command to chip for LPA: " << transaction_list.front()->LPA)
				}

				sim_time_type data_transfer_time = 0;

				for (std::list<NVM_Transaction_Flash*>::iterator it = transaction_list.begin();
					it != transaction_list.end(); it++) {
					(*it)->STAT_transfer_time += target_channel->ProgramCommandTime[transaction_list.size()] + NVDDR2DataInTransferTime((*it)->Data_and_metadata_size_in_byte, target_channel);
					data_transfer_time += NVDDR2DataInTransferTime((*it)->Data_and_metadata_size_in_byte, target_channel);
				}
				if (chipBKE->OngoingDieCMDTransfers.size() == 0)
				{
					targetChip->StartCMDDataInXfer();
					chipBKE->Status = ChipStatus::CMD_DATA_IN;
					chipBKE->Last_transfer_finish_time = Simulator->Time() + suspendTime + target_channel->ProgramCommandTime[transaction_list.size()] + data_transfer_time;
					Simulator->Register_sim_event(Simulator->Time() + suspendTime + target_channel->ProgramCommandTime[transaction_list.size()] + data_transfer_time,
						this, dieBKE, (int)NVDDR2_SimEventType::PROGRAM_CMD_ADDR_DATA_TRANSFERRED);
				}
				else
				{
					dieBKE->DieInterleavedTime = suspendTime + target_channel->ProgramCommandTime[transaction_list.size()] + data_transfer_time;
					chipBKE->Last_transfer_finish_time += suspendTime + target_channel->ProgramCommandTime[transaction_list.size()] + data_transfer_time;
				}
				chipBKE->OngoingDieCMDTransfers.push(dieBKE);

				dieBKE->ExpectedFinishTime = chipBKE->Last_transfer_finish_time + targetChip->Get_command_execution_latency(dieBKE->ActiveCommand->CommandCode, dieBKE->ActiveCommand->Address[0].PageID);
				if (chipBKE->Expected_command_exec_finish_time < dieBKE->ExpectedFinishTime)
					chipBKE->Expected_command_exec_finish_time = dieBKE->ExpectedFinishTime;
			}
			else//Copyback write for GC
			{
				if (transaction_list.size() == 1) {
					Stats::IssuedCopybackReadCMD++;
					dieBKE->ActiveCommand->CommandCode = CMD_READ_PAGE_COPYBACK;
				}
				else {
					Stats::IssuedMultiplaneCopybackProgramCMD++;
					dieBKE->ActiveCommand->CommandCode = CMD_READ_PAGE_COPYBACK_MULTIPLANE;
				}

				for (std::list<NVM_Transaction_Flash*>::iterator it = transaction_list.begin();
					it != transaction_list.end(); it++)
					(*it)->STAT_transfer_time += target_channel->ReadCommandTime[transaction_list.size()];
				if (chipBKE->OngoingDieCMDTransfers.size() == 0)
				{
					targetChip->StartCMDXfer();
					chipBKE->Status = ChipStatus::CMD_IN;
					chipBKE->Last_transfer_finish_time = Simulator->Time() + suspendTime + target_channel->ReadCommandTime[transaction_list.size()];
					Simulator->Register_sim_event(Simulator->Time() + suspendTime + target_channel->ReadCommandTime[transaction_list.size()], this,
						dieBKE, (int)NVDDR2_SimEventType::READ_CMD_ADDR_TRANSFERRED);
				}
				else
				{
					dieBKE->DieInterleavedTime = suspendTime + target_channel->ReadCommandTime[transaction_list.size()];
					chipBKE->Last_transfer_finish_time += suspendTime + target_channel->ReadCommandTime[transaction_list.size()];
				}
				chipBKE->OngoingDieCMDTransfers.push(dieBKE);

				dieBKE->ExpectedFinishTime = chipBKE->Last_transfer_finish_time + targetChip->Get_command_execution_latency(dieBKE->ActiveCommand->CommandCode, dieBKE->ActiveCommand->Address[0].PageID);
				if (chipBKE->Expected_command_exec_finish_time < dieBKE->ExpectedFinishTime)
					chipBKE->Expected_command_exec_finish_time = dieBKE->ExpectedFinishTime;
			}
			break;
		case TransactionType::ERASE:
			//DEBUG2("Chip " << targetChip->ChannelID << ", " << targetChip->ChipID << ", " << transaction_list.front()->Address.DieID << ": Sending erase command to chip")
			if (transaction_list.size() == 1) {
				Stats::IssuedEraseCMD++;
				dieBKE->ActiveCommand->CommandCode = CMD_ERASE_BLOCK;
			}
			else
			{
				Stats::IssuedMultiplaneEraseCMD++;
				dieBKE->ActiveCommand->CommandCode = CMD_ERASE_BLOCK_MULTIPLANE;
			}

			for (std::list<NVM_Transaction_Flash*>::iterator it = transaction_list.begin();
				it != transaction_list.end(); it++)
				(*it)->STAT_transfer_time += target_channel->EraseCommandTime[transaction_list.size()];
			if (chipBKE->OngoingDieCMDTransfers.size() == 0)
			{
				targetChip->StartCMDXfer();
				chipBKE->Status = ChipStatus::CMD_IN;
				chipBKE->Last_transfer_finish_time = Simulator->Time() + suspendTime + target_channel->EraseCommandTime[transaction_list.size()];
				Simulator->Register_sim_event(Simulator->Time() + suspendTime + target_channel->EraseCommandTime[transaction_list.size()],
					this, dieBKE, (int)NVDDR2_SimEventType::ERASE_SETUP_COMPLETED);
			}
			else
			{
				dieBKE->DieInterleavedTime = suspendTime + target_channel->EraseCommandTime[transaction_list.size()];
				chipBKE->Last_transfer_finish_time += suspendTime + target_channel->EraseCommandTime[transaction_list.size()];
			}
			chipBKE->OngoingDieCMDTransfers.push(dieBKE);

			dieBKE->ExpectedFinishTime = chipBKE->Last_transfer_finish_time + targetChip->Get_command_execution_latency(dieBKE->ActiveCommand->CommandCode, dieBKE->ActiveCommand->Address[0].PageID);
			if (chipBKE->Expected_command_exec_finish_time < dieBKE->ExpectedFinishTime)
				chipBKE->Expected_command_exec_finish_time = dieBKE->ExpectedFinishTime;
			break;
		default:
			throw std::invalid_argument("NVM_PHY_ONFI_NVDDR2: Unhandled event specified!");
		}

		target_channel->SetStatus(BusChannelStatus::BUSY, targetChip);
	}

	void NVM_PHY_ONFI_NVDDR2::Execute_simulator_event(MQSimEngine::Sim_Event* ev)
	{
		DieBookKeepingEntry* dieBKE = (DieBookKeepingEntry*)ev->Parameters;
		flash_channel_ID_type channel_id = dieBKE->ActiveTransactions.front()->Address.ChannelID;
		ONFI_Channel_NVDDR2* targetChannel = _Channels[channel_id];
		NVM::FlashMemory::Chip* targetChip = targetChannel->Chips[dieBKE->ActiveTransactions.front()->Address.ChipID];
		ChipBookKeepingEntry *chipBKE = &bookKeepingTable[channel_id][targetChip->ChipID];


		switch ((NVDDR2_SimEventType)ev->Type)
		{
		case NVDDR2_SimEventType::READ_CMD_ADDR_TRANSFERRED:
			//DEBUG2("Chip " << targetChip->ChannelID << ", " << targetChip->ChipID << ", " << dieBKE->ActiveTransactions.front()->Address.DieID << ": READ_CMD_ADDR_TRANSFERRED ")
			targetChip->EndCMDXfer(dieBKE->ActiveCommand);
			chipBKE->OngoingDieCMDTransfers.pop();
			chipBKE->No_of_active_dies++;
			if (chipBKE->OngoingDieCMDTransfers.size() > 0)
			{
				perform_interleaved_cmd_data_transfer(targetChip, chipBKE->OngoingDieCMDTransfers.front());
				return;
			}
			else
			{
				chipBKE->Status = ChipStatus::READING;
				targetChannel->SetStatus(BusChannelStatus::IDLE, targetChip);
			}
			break;
		case NVDDR2_SimEventType::ERASE_SETUP_COMPLETED:
			//DEBUG2("Chip " << targetChip->ChannelID << ", " << targetChip->ChipID << ", " << dieBKE->ActiveTransactions.front()->Address.DieID << ": ERASE_SETUP_COMPLETED ")
			targetChip->EndCMDXfer(dieBKE->ActiveCommand);
			chipBKE->OngoingDieCMDTransfers.pop();
			chipBKE->No_of_active_dies++;
			if (chipBKE->OngoingDieCMDTransfers.size() > 0)
			{
				perform_interleaved_cmd_data_transfer(targetChip, chipBKE->OngoingDieCMDTransfers.front());
				return;
			}
			else
			{
				chipBKE->Status = ChipStatus::ERASING;
				targetChannel->SetStatus(BusChannelStatus::IDLE, targetChip);
			}
			break;
		case NVDDR2_SimEventType::PROGRAM_CMD_ADDR_DATA_TRANSFERRED:
		case NVDDR2_SimEventType::PROGRAM_COPYBACK_CMD_ADDR_TRANSFERRED:
			//DEBUG2("Chip " << targetChip->ChannelID << ", " << targetChip->ChipID << ", " << dieBKE->ActiveTransactions.front()->Address.DieID <<  ": PROGRAM_CMD_ADDR_DATA_TRANSFERRED " )
			targetChip->EndCMDDataInXfer(dieBKE->ActiveCommand);
			chipBKE->OngoingDieCMDTransfers.pop();
			chipBKE->No_of_active_dies++;
			if (chipBKE->OngoingDieCMDTransfers.size() > 0)
			{
				perform_interleaved_cmd_data_transfer(targetChip, chipBKE->OngoingDieCMDTransfers.front());
				return;
			}
			else
			{
				chipBKE->Status = ChipStatus::WRITING;
				targetChannel->SetStatus(BusChannelStatus::IDLE, targetChip);
			}
			break;
		case NVDDR2_SimEventType::READ_DATA_TRANSFERRED:
			//DEBUG2("Chip " << targetChip->ChannelID << ", " << targetChip->ChipID << ", " << dieBKE->ActiveTransactions.front()->Address.DieID << ": READ_DATA_TRANSFERRED ")
			targetChip->EndDataOutXfer(dieBKE->ActiveCommand);
#if 0
			if (tr->ExecutionMode != ExecutionModeType::COPYBACK)
#endif
			broadcastTransactionServicedSignal(dieBKE->ActiveTransfer);

			for (std::list<NVM_Transaction_Flash*>::iterator it = dieBKE->ActiveTransactions.begin();
				it != dieBKE->ActiveTransactions.end(); it++)
				if ((*it) == dieBKE->ActiveTransfer) {
					dieBKE->ActiveTransactions.erase(it);
					break;
				}
			dieBKE->ActiveTransfer = NULL;
			if (dieBKE->ActiveTransactions.size() == 0)
				dieBKE->ClearCommand();

			chipBKE->WaitingReadTXCount--;
			if (chipBKE->No_of_active_dies == 0)
			{
				if (chipBKE->WaitingReadTXCount == 0)
					chipBKE->Status = ChipStatus::IDLE;
				else chipBKE->Status = ChipStatus::WAIT_FOR_DATA_OUT;
			}
			if (chipBKE->Status == ChipStatus::IDLE)
				if (dieBKE->Suspended)
					send_resume_command_to_chip(targetChip, chipBKE);
			targetChannel->SetStatus(BusChannelStatus::IDLE, targetChip);
			break;
		default:
			PRINT_ERROR("Unhandled SprinklerFCC event specified!")
		}

		/* Copyback requests are prioritized over other type of requests since they need very short transfer time.
		In addition, they are just used for GC purpose. */
		if (WaitingCopybackWrites[channel_id].size() > 0)
		{
			DieBookKeepingEntry* waitingBKE = WaitingCopybackWrites[channel_id].front();
			targetChip = _Channels[channel_id]->Chips[waitingBKE->ActiveTransactions.front()->Address.ChipID];
			ChipBookKeepingEntry* waitingChipBKE = &bookKeepingTable[channel_id][targetChip->ChipID];
			if (waitingBKE->ActiveTransactions.size() > 1)
			{
				Stats::IssuedMultiplaneCopybackProgramCMD++;
				waitingBKE->ActiveCommand->CommandCode = CMD_PROGRAM_PAGE_COPYBACK_MULTIPLANE;
			}
			else
			{
				Stats::IssuedCopybackProgramCMD++;
				waitingBKE->ActiveCommand->CommandCode = CMD_PROGRAM_PAGE_COPYBACK;
			}
			targetChip->StartCMDXfer();
			waitingChipBKE->Status = ChipStatus::CMD_IN;
			Simulator->Register_sim_event(Simulator->Time() + this->_Channels[channel_id]->ProgramCommandTime[waitingBKE->ActiveTransactions.size()],
				this, waitingBKE, (int)NVDDR2_SimEventType::PROGRAM_COPYBACK_CMD_ADDR_TRANSFERRED);
			waitingChipBKE->OngoingDieCMDTransfers.push(waitingBKE);

			waitingBKE->ExpectedFinishTime = Simulator->Time() + this->_Channels[channel_id]->ProgramCommandTime[waitingBKE->ActiveTransactions.size()]
				+ targetChip->Get_command_execution_latency(waitingBKE->ActiveCommand->CommandCode, waitingBKE->ActiveCommand->Address[0].PageID);
			if (waitingChipBKE->Expected_command_exec_finish_time < waitingBKE->ExpectedFinishTime)
				waitingChipBKE->Expected_command_exec_finish_time = waitingBKE->ExpectedFinishTime;

			WaitingCopybackWrites[channel_id].pop_front();
			_Channels[channel_id]->SetStatus(BusChannelStatus::BUSY, targetChip);
			return;
		}
		else if (WaitingMappingRead_TX[channel_id].size() > 0)
		{
			NVM_Transaction_Flash_RD* waitingTR = (NVM_Transaction_Flash_RD*)WaitingMappingRead_TX[channel_id].front();
			WaitingMappingRead_TX[channel_id].pop_front();
			transfer_read_data_from_chip(&bookKeepingTable[channel_id][waitingTR->Address.ChipID],
				&(bookKeepingTable[channel_id][waitingTR->Address.ChipID].Die_book_keeping_records[waitingTR->Address.DieID]), waitingTR);
			return;
		}
		else if (WaitingReadTX[channel_id].size() > 0)
		{
			NVM_Transaction_Flash_RD* waitingTR = (NVM_Transaction_Flash_RD*)WaitingReadTX[channel_id].front();
			WaitingReadTX[channel_id].pop_front();
			transfer_read_data_from_chip(&bookKeepingTable[channel_id][waitingTR->Address.ChipID],
				&(bookKeepingTable[channel_id][waitingTR->Address.ChipID].Die_book_keeping_records[waitingTR->Address.DieID]), waitingTR);
			return;
		}
		else if (WaitingGCRead_TX[channel_id].size() > 0)
		{
			NVM_Transaction_Flash_RD* waitingTR = (NVM_Transaction_Flash_RD*)WaitingGCRead_TX[channel_id].front();
			WaitingGCRead_TX[channel_id].pop_front();
			transfer_read_data_from_chip(&bookKeepingTable[channel_id][waitingTR->Address.ChipID],
				&(bookKeepingTable[channel_id][waitingTR->Address.ChipID].Die_book_keeping_records[waitingTR->Address.DieID]), waitingTR);
			return;
		}

		//If the execution reaches here, then the bus channel became idle
		broadcastChannelIdleSignal(channel_id);
	}

	inline void NVM_PHY_ONFI_NVDDR2::handle_ready_signal_from_chip(NVM::FlashMemory::Chip* chip, NVM::FlashMemory::Flash_Command* command)
	{
		ChipBookKeepingEntry *chipBKE = &_myInstance->bookKeepingTable[chip->ChannelID][chip->ChipID];
		DieBookKeepingEntry *dieBKE = &(chipBKE->Die_book_keeping_records[command->Address[0].DieID]);

		switch (command->CommandCode)
		{
		case CMD_READ_PAGE:
		case CMD_READ_PAGE_MULTIPLANE:
			//DEBUG2("Chip " << chip->ChannelID << ", " << chip->ChipID << ": finished  read command")
			chipBKE->No_of_active_dies--;
			if (chipBKE->No_of_active_dies == 0)//After finishing the last command, the chip state is changed
				chipBKE->Status = ChipStatus::WAIT_FOR_DATA_OUT;

			for (std::list<NVM_Transaction_Flash*>::iterator it = dieBKE->ActiveTransactions.begin();
				it != dieBKE->ActiveTransactions.end(); it++)
			{
				chipBKE->WaitingReadTXCount++;
				if (_myInstance->_Channels[chip->ChannelID]->GetStatus() == BusChannelStatus::IDLE)
					_myInstance->transfer_read_data_from_chip(chipBKE, dieBKE, (*it));
				else
				{
					switch (dieBKE->ActiveTransactions.front()->Source)
					{
					case Transaction_Source_Type::USERIO:
						_myInstance->WaitingReadTX[chip->ChannelID].push_back((*it));
						break;
					case Transaction_Source_Type::GC:
						_myInstance->WaitingGCRead_TX[chip->ChannelID].push_back((*it));
						break;
					case Transaction_Source_Type::MAPPING:
						_myInstance->WaitingMappingRead_TX[chip->ChannelID].push_back((*it));
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
			if (_myInstance->_Channels[chip->ChannelID]->GetStatus() == BusChannelStatus::IDLE)
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
					(*it)->STAT_transfer_time += _myInstance->_Channels[chip->ChannelID]->ProgramCommandTime[dieBKE->ActiveTransactions.size()];
				chip->StartCMDXfer();
				chipBKE->Status = ChipStatus::CMD_IN;
				Simulator->Register_sim_event(Simulator->Time() + _myInstance->_Channels[chip->ChannelID]->ProgramCommandTime[dieBKE->ActiveTransactions.size()],
					_myInstance, dieBKE, (int)NVDDR2_SimEventType::PROGRAM_COPYBACK_CMD_ADDR_TRANSFERRED);
				chipBKE->OngoingDieCMDTransfers.push(dieBKE);
				_myInstance->_Channels[chip->ChannelID]->SetStatus(BusChannelStatus::BUSY, chip);

				dieBKE->ExpectedFinishTime = Simulator->Time() + _myInstance->_Channels[chip->ChannelID]->ProgramCommandTime[dieBKE->ActiveTransactions.size()]
					+ chip->Get_command_execution_latency(dieBKE->ActiveCommand->CommandCode, dieBKE->ActiveCommand->Address[0].PageID);
				if (chipBKE->Expected_command_exec_finish_time < dieBKE->ExpectedFinishTime)
					chipBKE->Expected_command_exec_finish_time = dieBKE->ExpectedFinishTime;
#if 0	
				//Copyback data should be read out in order to get rid of bit error propagation
				Simulator->RegisterEvent(Simulator->Time() + _Channels[targetChip->ChannelID]->ProgramCommandTime + NVDDR2DataOutTransferTime(targetTransaction->SizeInByte, _Channels[targetChip->ChannelID]),
					this, targetTransaction, (int)NVDDR2_SimEventType::READ_DATA_TRANSFERRED);
				targetTransaction->STAT_TransferTime += NVDDR2DataOutTransferTime(targetTransaction->SizeInByte, _Channels[targetChip->ChannelID]);
#endif
			}
			else _myInstance->WaitingCopybackWrites->push_back(dieBKE);
			break;
		case CMD_PROGRAM_PAGE:
		case CMD_PROGRAM_PAGE_MULTIPLANE:
		case CMD_PROGRAM_PAGE_COPYBACK:
		case CMD_PROGRAM_PAGE_COPYBACK_MULTIPLANE:
		case CMD_ERASE_BLOCK:
		case CMD_ERASE_BLOCK_MULTIPLANE:
			//DEBUG2("Chip " << chip->ChannelID << ", " << chip->ChipID << ": finished program/erase command")
			for (std::list<NVM_Transaction_Flash*>::iterator it = dieBKE->ActiveTransactions.begin();
				it != dieBKE->ActiveTransactions.end(); it++)
				_myInstance->broadcastTransactionServicedSignal(*it);
			dieBKE->ActiveTransactions.clear();
			dieBKE->ClearCommand();

			chipBKE->No_of_active_dies--;
			if (chipBKE->No_of_active_dies == 0 && chipBKE->WaitingReadTXCount == 0)
				chipBKE->Status = ChipStatus::IDLE;
			//Since the time required to send the resume command is very small, we ignore it
			if (chipBKE->Status == ChipStatus::IDLE)
				if (chipBKE->HasSuspend)
					_myInstance->send_resume_command_to_chip(chip, chipBKE);
			break;
		default:
			break;
		}

		if (_myInstance->_Channels[chip->ChannelID]->GetStatus() == BusChannelStatus::IDLE)
			_myInstance->broadcastChannelIdleSignal(chip->ChannelID);
		else if (chipBKE->Status == ChipStatus::IDLE)
			_myInstance->broadcastChipIdleSignal(chip);
	}

	inline void NVM_PHY_ONFI_NVDDR2::transfer_read_data_from_chip(ChipBookKeepingEntry* chipBKE, DieBookKeepingEntry* dieBKE, NVM_Transaction_Flash* tr)
	{
		//DEBUG2("Chip " << tr->Address.ChannelID << ", " << tr->Address.ChipID << ": transfer read data started for LPA: " << tr->LPA)
		dieBKE->ActiveTransfer = tr;
		_Channels[tr->Address.ChannelID]->Chips[tr->Address.ChipID]->StartDataOutXfer();
		chipBKE->Status = ChipStatus::DATA_OUT;
		Simulator->Register_sim_event(Simulator->Time() + NVDDR2DataOutTransferTime(tr->Data_and_metadata_size_in_byte, _Channels[tr->Address.ChannelID]),
			this, dieBKE, (int)NVDDR2_SimEventType::READ_DATA_TRANSFERRED);

		tr->STAT_transfer_time += NVDDR2DataOutTransferTime(tr->Data_and_metadata_size_in_byte, _Channels[tr->Address.ChannelID]);
		_Channels[tr->Address.ChannelID]->SetStatus(BusChannelStatus::BUSY, _Channels[tr->Address.ChannelID]->Chips[tr->Address.ChipID]);
	}

	void NVM_PHY_ONFI_NVDDR2::perform_interleaved_cmd_data_transfer(NVM::FlashMemory::Chip* chip, DieBookKeepingEntry* bookKeepingEntry)
	{
		ONFI_Channel_NVDDR2* target_channel = _Channels[bookKeepingEntry->ActiveTransactions.front()->Address.ChannelID];
		/*if (target_channel->Status == BusChannelStatus::BUSY)
			PRINT_ERROR("Requesting communication on a busy bus!")*/

		switch (bookKeepingEntry->ActiveTransactions.front()->Type)
		{
		case TransactionType::READ:
			chip->StartCMDXfer();
			bookKeepingTable[chip->ChannelID][chip->ChipID].Status = ChipStatus::CMD_IN;
			Simulator->Register_sim_event(Simulator->Time() + bookKeepingEntry->DieInterleavedTime,
				this, bookKeepingEntry, (int)NVDDR2_SimEventType::READ_CMD_ADDR_TRANSFERRED);
			break;
		case TransactionType::WRITE:
			if (((NVM_Transaction_Flash_WR*)bookKeepingEntry->ActiveTransactions.front())->RelatedRead == NULL) {
				chip->StartCMDDataInXfer();
				bookKeepingTable[chip->ChannelID][chip->ChipID].Status = ChipStatus::CMD_DATA_IN;
				Simulator->Register_sim_event(Simulator->Time() + bookKeepingEntry->DieInterleavedTime,
					this, bookKeepingEntry, (int)NVDDR2_SimEventType::PROGRAM_CMD_ADDR_DATA_TRANSFERRED);
			}
			else
			{
				chip->StartCMDXfer();
				bookKeepingTable[chip->ChannelID][chip->ChipID].Status = ChipStatus::CMD_IN;
				Simulator->Register_sim_event(Simulator->Time() + bookKeepingEntry->DieInterleavedTime, this,
					bookKeepingEntry, (int)NVDDR2_SimEventType::READ_CMD_ADDR_TRANSFERRED);
			}
			break;
		case TransactionType::ERASE:
			chip->StartCMDXfer();
			bookKeepingTable[chip->ChannelID][chip->ChipID].Status = ChipStatus::CMD_IN;
			Simulator->Register_sim_event(Simulator->Time() + bookKeepingEntry->DieInterleavedTime,
				this, bookKeepingEntry, (int)NVDDR2_SimEventType::ERASE_SETUP_COMPLETED);
			break;
		default:
			PRINT_ERROR("NVMController_NVDDR2: Unhandled transaction type!")
		}
		target_channel->SetStatus(BusChannelStatus::BUSY, chip);
	}

	inline void NVM_PHY_ONFI_NVDDR2::send_resume_command_to_chip(NVM::FlashMemory::Chip* chip, ChipBookKeepingEntry* chipBKE)
	{
		//DEBUG2("Chip " << chip->ChannelID << ", " << chip->ChipID << ": resume command " )
		for (unsigned int i = 0; i < die_no_per_chip; i++)
		{
			DieBookKeepingEntry *dieBKE = &chipBKE->Die_book_keeping_records[i];
			//Since the time required to send the resume command is very small, MQSim ignores it to simplify the simulation
			dieBKE->PrepareResume();
			chipBKE->PrepareResume();
			chip->Resume(dieBKE->ActiveCommand->Address[0].DieID);
			switch (dieBKE->ActiveCommand->CommandCode)
			{
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
