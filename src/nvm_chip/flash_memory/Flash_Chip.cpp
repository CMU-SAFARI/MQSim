#include "../../sim/Sim_Defs.h"
#include "../../sim/Engine.h"
#include "Flash_Chip.h"


namespace NVM
{
	namespace FlashMemory
	{
		Flash_Chip::Flash_Chip(const sim_object_id_type& id, flash_channel_ID_type channelID, flash_chip_ID_type localChipID,
			Flash_Technology_Type flash_technology, 
			unsigned int dieNo, unsigned int PlaneNoPerDie, unsigned int Block_no_per_plane, unsigned int Page_no_per_block,
			sim_time_type* readLatency, sim_time_type* programLatency, sim_time_type eraseLatency,
			sim_time_type suspendProgramLatency, sim_time_type suspendEraseLatency,
			sim_time_type commProtocolDelayRead, sim_time_type commProtocolDelayWrite, sim_time_type commProtocolDelayErase)
			: NVM_Chip(id), ChannelID(channelID), ChipID(localChipID), flash_technology(flash_technology),
			status(Internal_Status::IDLE), die_no(dieNo), plane_no_in_die(PlaneNoPerDie), block_no_in_plane(Block_no_per_plane), page_no_per_block(Page_no_per_block),
			_RBSignalDelayRead(commProtocolDelayRead), _RBSignalDelayWrite(commProtocolDelayWrite), _RBSignalDelayErase(commProtocolDelayErase),
			lastTransferStart(INVALID_TIME), executionStartTime(INVALID_TIME), expectedFinishTime(INVALID_TIME),
			STAT_readCount(0), STAT_progamCount(0), STAT_eraseCount(0),
			STAT_totalSuspensionCount(0), STAT_totalResumeCount(0),
			STAT_totalExecTime(0), STAT_totalXferTime(0), STAT_totalOverlappedXferExecTime(0)
		{
			int bits_per_cell = static_cast<int>(flash_technology);
			_readLatency = new sim_time_type[bits_per_cell];
			_programLatency = new sim_time_type[bits_per_cell];
			for (int i = 0; i < bits_per_cell; i++) {
				_readLatency[i] = readLatency[i];
				_programLatency[i] = programLatency[i];
			}
			_eraseLatency = eraseLatency;
			_suspendProgramLatency = suspendProgramLatency;
			_suspendEraseLatency = suspendEraseLatency;
			idleDieNo = dieNo;
			Dies = new Die*[dieNo];
			for (unsigned int dieID = 0; dieID < dieNo; dieID++) {
				Dies[dieID] = new Die(PlaneNoPerDie, Block_no_per_plane, Page_no_per_block);
			}
		}

		Flash_Chip::~Flash_Chip()
		{
			for (unsigned int dieID = 0; dieID < die_no; dieID++) {
				delete Dies[dieID];
			}
			delete[] Dies;
			delete[] _readLatency;
			delete[] _programLatency;
		}

		void Flash_Chip::Connect_to_chip_ready_signal(ChipReadySignalHandlerType function)
		{
			connectedReadyHandlers.push_back(function);
		}
		
		void Flash_Chip::Start_simulation()
		{
		}

		void Flash_Chip::Validate_simulation_config()
		{
			if (Dies == NULL || die_no == 0) {
				PRINT_ERROR("Flash chip " << ID() << ": has no dies!")
			}

			for (unsigned int i = 0; i < die_no; i++) {
				if (Dies[i]->Planes == NULL) {
					PRINT_ERROR("Flash chip" << ID() << ": die (" + ID() + ") has no planes!")
				}
			}
		}
		
		void Flash_Chip::Change_memory_status_preconditioning(const NVM_Memory_Address* address, const void* status_info)
		{
			Physical_Page_Address* flash_address = (Physical_Page_Address*)address;
			Dies[flash_address->DieID]->Planes[flash_address->PlaneID]->Blocks[flash_address->BlockID]->Pages[flash_address->PageID].Metadata.LPA = *(LPA_type*)status_info;
		}
		
		void Flash_Chip::Setup_triggers()
		{
			MQSimEngine::Sim_Object::Setup_triggers();
		}
		
		void Flash_Chip::Execute_simulator_event(MQSimEngine::Sim_Event* ev)
		{
			Chip_Sim_Event_Type eventType = (Chip_Sim_Event_Type)ev->Type;
			Flash_Command* command = (Flash_Command*)ev->Parameters;

			switch (eventType) {
				case Chip_Sim_Event_Type::COMMAND_FINISHED:
					finish_command_execution(command);
					break;
			}
		}

		LPA_type Flash_Chip::Get_metadata(flash_die_ID_type die_id, flash_plane_ID_type plane_id, flash_block_ID_type block_id, flash_page_ID_type page_id)//A simplification to decrease the complexity of GC execution! The GC unit may need to know the metadata of a page to decide if a page is valid or invalid. 
		{
			Page* page = &(Dies[die_id]->Planes[plane_id]->Blocks[block_id]->Pages[page_id]);
			return Dies[die_id]->Planes[plane_id]->Blocks[block_id]->Pages[page_id].Metadata.LPA;
		}

		void Flash_Chip::start_command_execution(Flash_Command* command)
		{
			Die* targetDie = Dies[command->Address[0].DieID];

			//If this is a simple command (not multiplane) then there should be only one address
			if (command->Address.size() > 1
				&& (command->CommandCode == CMD_READ_PAGE
					|| command->CommandCode == CMD_PROGRAM_PAGE
					|| command->CommandCode == CMD_ERASE_BLOCK)) {
				PRINT_ERROR("Flash chip " << ID() << ": executing a flash operation on a busy die!")
			}

			targetDie->Expected_finish_time = Simulator->Time() + Get_command_execution_latency(command->CommandCode, command->Address[0].PageID);
			targetDie->CommandFinishEvent = Simulator->Register_sim_event(targetDie->Expected_finish_time,
				this, command, static_cast<int>(Chip_Sim_Event_Type::COMMAND_FINISHED));
			targetDie->CurrentCMD = command;
			targetDie->Status = DieStatus::BUSY;
			idleDieNo--;

			if (status == Internal_Status::IDLE) {
				executionStartTime = Simulator->Time();
				expectedFinishTime = targetDie->Expected_finish_time;
				status = Internal_Status::BUSY;
			}

			DEBUG("Command execution started on channel: " << this->ChannelID << " chip: " << this->ChipID)
		}

		void Flash_Chip::finish_command_execution(Flash_Command* command)
		{
			Die* targetDie = Dies[command->Address[0].DieID];

			targetDie->STAT_TotalReadTime += Get_command_execution_latency(command->CommandCode, command->Address[0].PageID);
			targetDie->Expected_finish_time = INVALID_TIME;
			targetDie->CommandFinishEvent = NULL;
			targetDie->CurrentCMD = NULL;
			targetDie->Status = DieStatus::IDLE;
			this->idleDieNo++;
			if (idleDieNo == die_no) {
				this->status = Internal_Status::IDLE;
				STAT_totalExecTime += Simulator->Time() - executionStartTime;
				if (this->lastTransferStart != INVALID_TIME) {
					STAT_totalOverlappedXferExecTime += Simulator->Time() - lastTransferStart;
				}
			}

			switch (command->CommandCode)
			{
				case CMD_READ_PAGE:
				case CMD_READ_PAGE_MULTIPLANE:
				case CMD_READ_PAGE_COPYBACK:
				case CMD_READ_PAGE_COPYBACK_MULTIPLANE:
					DEBUG("Channel " << this->ChannelID << " Chip " << this->ChipID << "- Finished executing read command")
					for (unsigned int planeCntr = 0; planeCntr < command->Address.size(); planeCntr++) {
						STAT_readCount++;
						targetDie->Planes[command->Address[planeCntr].PlaneID]->Read_count++;
						targetDie->Planes[command->Address[planeCntr].PlaneID]->Blocks[command->Address[planeCntr].BlockID]->Pages[command->Address[planeCntr].PageID].Read_metadata(command->Meta_data[planeCntr]);
					}
					break;
				case CMD_PROGRAM_PAGE:
				case CMD_PROGRAM_PAGE_MULTIPLANE:
				case CMD_PROGRAM_PAGE_COPYBACK:
				case CMD_PROGRAM_PAGE_COPYBACK_MULTIPLANE:
					DEBUG("Channel " << this->ChannelID << " Chip " << this->ChipID << "- Finished executing program command")
					for (unsigned int planeCntr = 0; planeCntr < command->Address.size(); planeCntr++) {
						STAT_progamCount++;
						targetDie->Planes[command->Address[planeCntr].PlaneID]->Progam_count++;
						targetDie->Planes[command->Address[planeCntr].PlaneID]->Blocks[command->Address[planeCntr].BlockID]->Pages[command->Address[planeCntr].PageID].Write_metadata(command->Meta_data[planeCntr]);
					}
					break;
				case CMD_ERASE_BLOCK:
				case CMD_ERASE_BLOCK_MULTIPLANE:
				{
					for (unsigned int planeCntr = 0; planeCntr < command->Address.size(); planeCntr++) {
						STAT_eraseCount++;
						targetDie->Planes[command->Address[planeCntr].PlaneID]->Erase_count++;
						Block* targetBlock = targetDie->Planes[command->Address[planeCntr].PlaneID]->Blocks[command->Address[planeCntr].BlockID];
						for (unsigned int i = 0; i < page_no_per_block; i++) {
							//targetBlock->Pages[i].Metadata.SourceStreamID = NO_STREAM;
							//targetBlock->Pages[i].Metadata.Status = FREE_PAGE;
							targetBlock->Pages[i].Metadata.LPA = NO_LPA;
						}
					}
					break;
				}
				default:
					PRINT_ERROR("Flash chip " << ID() << ": unhandled flash command type!")
			}

			//In MQSim, flash chips always announce their status using the ready/busy signal; the controller does not issue a die status read command
			broadcast_ready_signal(command);
		}

		void Flash_Chip::broadcast_ready_signal(Flash_Command* command)
		{
			for (std::vector<ChipReadySignalHandlerType>::iterator it = connectedReadyHandlers.begin();
				it != connectedReadyHandlers.end(); it++) {
				(*it)(this, command);
			}
		}

		void Flash_Chip::Suspend(flash_die_ID_type dieID)
		{
			STAT_totalExecTime += Simulator->Time() - executionStartTime;

			Die* targetDie = Dies[dieID];
			if (targetDie->Suspended) {
				PRINT_ERROR("Flash chip" << ID() << ": suspending a previously suspended flash chip! This is illegal.")
			}

			/*if (targetDie->CurrentCMD & CMD_READ != 0)
			throw "Suspend is not supported for read operations!";*/

			targetDie->RemainingSuspendedExecTime = targetDie->Expected_finish_time - Simulator->Time();
			Simulator->Ignore_sim_event(targetDie->CommandFinishEvent);//The simulator engine should not execute the finish event for the suspended command
			targetDie->CommandFinishEvent = NULL;

			targetDie->SuspendedCMD = targetDie->CurrentCMD;
			targetDie->CurrentCMD = NULL;
			targetDie->Suspended = true;
			STAT_totalSuspensionCount++;

			targetDie->Status = DieStatus::IDLE;
			this->idleDieNo++;
			if (this->idleDieNo == die_no) {
				this->status = Internal_Status::IDLE;
			}

			executionStartTime = INVALID_TIME;
			expectedFinishTime = INVALID_TIME;
		}

		void Flash_Chip::Resume(flash_die_ID_type dieID)
		{
			Die* targetDie = Dies[dieID];
			if (!targetDie->Suspended) {
				PRINT_ERROR("Flash chip " << ID() << ": resume flash command is requested, but there is no suspended flash command!")
			}

			targetDie->CurrentCMD = targetDie->SuspendedCMD;
			targetDie->SuspendedCMD = NULL;
			targetDie->Suspended = false;
			STAT_totalResumeCount++;

			targetDie->Expected_finish_time = Simulator->Time() + targetDie->RemainingSuspendedExecTime;
			targetDie->CommandFinishEvent = Simulator->Register_sim_event(targetDie->Expected_finish_time,
				this, targetDie->CurrentCMD, static_cast<int>(Chip_Sim_Event_Type::COMMAND_FINISHED));
			if (targetDie->Expected_finish_time > this->expectedFinishTime) {
				this->expectedFinishTime = targetDie->Expected_finish_time;
			}

			targetDie->Status = DieStatus::BUSY;
			this->idleDieNo--;
			this->status = Internal_Status::BUSY;
			executionStartTime = Simulator->Time();
		}

		sim_time_type Flash_Chip::GetSuspendProgramTime()
		{
			return _suspendProgramLatency;
		}
	
		sim_time_type Flash_Chip::GetSuspendEraseTime()
		{
			return _suspendEraseLatency;
		}

		void Flash_Chip::Report_results_in_XML(std::string name_prefix, Utils::XmlWriter& xmlwriter)
		{
			std::string tmp = name_prefix;
			xmlwriter.Write_start_element_tag(tmp + ".FlashChips");

			std::string attr = "ID";
			std::string val = "@" + std::to_string(ChannelID) + "@" + std::to_string(ChipID);
			xmlwriter.Write_attribute_string_inline(attr, val);

			attr = "Fraction_of_Time_in_Execution";
			val = std::to_string(STAT_totalExecTime / double(Simulator->Time()));
			xmlwriter.Write_attribute_string_inline(attr, val);

			attr = "Fraction_of_Time_in_DataXfer";
			val = std::to_string(STAT_totalXferTime / double(Simulator->Time()));
			xmlwriter.Write_attribute_string_inline(attr, val);

			attr = "Fraction_of_Time_in_DataXfer_and_Execution";
			val = std::to_string(STAT_totalOverlappedXferExecTime / double(Simulator->Time()));
			xmlwriter.Write_attribute_string_inline(attr, val);

			attr = "Fraction_of_Time_Idle";
			val = std::to_string((Simulator->Time() - STAT_totalOverlappedXferExecTime - STAT_totalXferTime) / double(Simulator->Time()));
			xmlwriter.Write_attribute_string_inline(attr, val);
		
			xmlwriter.Write_end_element_tag();
		}
	}
}
