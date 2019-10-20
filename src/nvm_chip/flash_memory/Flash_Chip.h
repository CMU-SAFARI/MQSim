#ifndef FLASH_CHIP_H
#define FLASH_CHIP_H

#include "../../sim/Sim_Defs.h"
#include "../../sim/Sim_Event.h"
#include "../../sim/Engine.h"
#include "../../sim/Sim_Reporter.h"
#include "../NVM_Chip.h"
#include "FlashTypes.h"
#include "Die.h"
#include "Flash_Command.h"
#include <vector>
#include <stdexcept>

namespace NVM
{
	namespace FlashMemory
	{
		class Flash_Chip : public NVM_Chip
		{
			enum class Internal_Status { IDLE, BUSY };
			enum class Chip_Sim_Event_Type { COMMAND_FINISHED };
		public:
			Flash_Chip(const sim_object_id_type&, flash_channel_ID_type channelID, flash_chip_ID_type localChipID,
				Flash_Technology_Type flash_technology, 
				unsigned int dieNo, unsigned int PlaneNoPerDie, unsigned int Block_no_per_plane, unsigned int Page_no_per_block,
				sim_time_type *readLatency, sim_time_type *programLatency, sim_time_type eraseLatency,
				sim_time_type suspendProgramLatency, sim_time_type suspendEraseLatency,
				sim_time_type commProtocolDelayRead = 20, sim_time_type commProtocolDelayWrite = 0, sim_time_type commProtocolDelayErase = 0);
			~Flash_Chip();
			flash_channel_ID_type ChannelID;
			flash_chip_ID_type ChipID;         //Flashchip position in its related channel

			void StartCMDXfer()
			{
				this->lastTransferStart = Simulator->Time();
			}
			void StartCMDDataInXfer()
			{
				this->lastTransferStart = Simulator->Time();
			}
			void StartDataOutXfer()
			{
				this->lastTransferStart = Simulator->Time();
			}
			void EndCMDXfer(Flash_Command* command)//End transferring write data to the Flash chip
			{
				this->STAT_totalXferTime += (Simulator->Time() - this->lastTransferStart);
				if (this->idleDieNo != die_no)
					STAT_totalOverlappedXferExecTime += (Simulator->Time() - lastTransferStart);
				this->Dies[command->Address[0].DieID]->STAT_TotalXferTime += (Simulator->Time() - lastTransferStart);

				start_command_execution(command);

				this->lastTransferStart = INVALID_TIME;
			}
			void EndCMDDataInXfer(Flash_Command* command)//End transferring write data of a group of multi-plane transactions to the Flash chip
			{
				this->STAT_totalXferTime += (Simulator->Time() - this->lastTransferStart);
				if (this->idleDieNo != die_no)
					STAT_totalOverlappedXferExecTime += (Simulator->Time() - lastTransferStart);
				this->Dies[command->Address[0].DieID]->STAT_TotalXferTime += (Simulator->Time() - lastTransferStart);

				start_command_execution(command);

				this->lastTransferStart = INVALID_TIME;
			}
			void EndDataOutXfer(Flash_Command* command)
			{
				this->STAT_totalXferTime += (Simulator->Time() - this->lastTransferStart);
				if (this->idleDieNo != die_no)
					STAT_totalOverlappedXferExecTime += (Simulator->Time() - lastTransferStart);
				this->Dies[command->Address[0].DieID]->STAT_TotalXferTime += (Simulator->Time() - lastTransferStart);

				this->lastTransferStart = INVALID_TIME;
			}
			void Change_memory_status_preconditioning(const NVM_Memory_Address* address, const void* status_info);
			void Start_simulation();
			void Validate_simulation_config();
			void Setup_triggers();
			void Execute_simulator_event(MQSimEngine::Sim_Event*);
			typedef void(*ChipReadySignalHandlerType) (Flash_Chip* targetChip, Flash_Command* command);
			void Connect_to_chip_ready_signal(ChipReadySignalHandlerType);
			
			sim_time_type Get_command_execution_latency(command_code_type CMDCode, flash_page_ID_type pageID)
			{
				int latencyType = 0;
				if (flash_technology == Flash_Technology_Type::MLC) {
					latencyType = pageID % 2;
				} else if (flash_technology == Flash_Technology_Type::TLC) {
					//From: Yaakobi et al., "Characterization and Error-Correcting Codes for TLC Flash Memories", ICNC 2012
					latencyType = (pageID <= 5) ? 0 : ((pageID <= 7) ? 1 : (((pageID - 8) >> 1) % 3));;
				}

				switch (CMDCode)
				{
					case CMD_READ_PAGE:
					case CMD_READ_PAGE_MULTIPLANE:
					case CMD_READ_PAGE_COPYBACK:
					case CMD_READ_PAGE_COPYBACK_MULTIPLANE:
						return _readLatency[latencyType] + _RBSignalDelayRead;
					case CMD_PROGRAM_PAGE:
					case CMD_PROGRAM_PAGE_MULTIPLANE:
					case CMD_PROGRAM_PAGE_COPYBACK:
					case CMD_PROGRAM_PAGE_COPYBACK_MULTIPLANE:
						return _programLatency[latencyType] + _RBSignalDelayWrite;
					case CMD_ERASE_BLOCK:
					case CMD_ERASE_BLOCK_MULTIPLANE:
						return _eraseLatency + _RBSignalDelayErase;
					default:
						throw std::invalid_argument("Unsupported command for flash chip.");
				}
			}

			void Suspend(flash_die_ID_type dieID);
			void Resume(flash_die_ID_type dieID);
			sim_time_type GetSuspendProgramTime();
			sim_time_type GetSuspendEraseTime();
			void Report_results_in_XML(std::string name_prefix, Utils::XmlWriter& xmlwriter);
			LPA_type Get_metadata(flash_die_ID_type die_id, flash_plane_ID_type plane_id, flash_block_ID_type block_id, flash_page_ID_type page_id);//A simplification to decrease the complexity of GC execution! The GC unit may need to know the metadata of a page to decide if a page is valid or invalid. 
		private:
			Flash_Technology_Type flash_technology;
			Internal_Status status;
			unsigned int idleDieNo;
			Die** Dies;
			unsigned int die_no;
			unsigned int plane_no_in_die;                  //indicate how many planes in a die
			unsigned int block_no_in_plane;                //indicate how many blocks in a plane
			unsigned int page_no_per_block;                 //indicate how many pages in a block
			sim_time_type *_readLatency, *_programLatency, _eraseLatency;
			sim_time_type _suspendProgramLatency, _suspendEraseLatency;
			sim_time_type _RBSignalDelayRead, _RBSignalDelayWrite, _RBSignalDelayErase;
			sim_time_type lastTransferStart;
			sim_time_type executionStartTime, expectedFinishTime;

			unsigned long STAT_readCount, STAT_progamCount, STAT_eraseCount;
			unsigned long STAT_totalSuspensionCount, STAT_totalResumeCount;
			sim_time_type STAT_totalExecTime, STAT_totalXferTime, STAT_totalOverlappedXferExecTime;

			void start_command_execution(Flash_Command* command);
			void finish_command_execution(Flash_Command* command);
			void broadcast_ready_signal(Flash_Command* command);
			std::vector<ChipReadySignalHandlerType> connectedReadyHandlers;
		};
	}
}

#endif // !FLASH_CHIP_H
