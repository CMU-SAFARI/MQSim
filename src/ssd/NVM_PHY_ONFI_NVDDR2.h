#ifndef NVM_PHY_ONFI_NVDDR2_H
#define NVM_PHY_ONFI_NVDDR2_H

#include <queue>
#include <list>
#include "../sim/Sim_Defs.h"
#include "../nvm_chip/flash_memory/FlashTypes.h"
#include "../nvm_chip/flash_memory/Flash_Command.h"
#include "NVM_PHY_ONFI.h"
#include "ONFI_Channel_NVDDR2.h"
#include "Flash_Transaction_Queue.h"

namespace SSD_Components
{
	enum class NVDDR2_SimEventType
	{
		READ_DATA_TRANSFERRED, READ_CMD_ADDR_TRANSFERRED,
		PROGRAM_CMD_ADDR_DATA_TRANSFERRED, 
		PROGRAM_COPYBACK_CMD_ADDR_TRANSFERRED, 
		ERASE_SETUP_COMPLETED
	};

	class DieBookKeepingEntry
	{
	public:
		NVM::FlashMemory::Flash_Command* ActiveCommand; //The current command that is executing on the die

		/*The current transactions that are being serviced. For the set of transactions in ActiveTransactions,
		there is one ActiveCommand that is geting executed on the die. Transaction is a FTL-level concept, and
		command is a flash chip-level concept*/
		std::list<NVM_Transaction_Flash*> ActiveTransactions; 
		NVM::FlashMemory::Flash_Command* SuspendedCommand;
		std::list<NVM_Transaction_Flash*> SuspendedTransactions;
		NVM_Transaction_Flash* ActiveTransfer; //The current transaction 
		bool Free;
		bool Suspended;
		sim_time_type Expected_finish_time;
		sim_time_type RemainingExecTime;
		sim_time_type DieInterleavedTime;//If the command transfer is done in die-interleaved mode, the transfer time is recorded in this temporary variable

		void PrepareSuspend()
		{
			SuspendedCommand = ActiveCommand;
			RemainingExecTime = Expected_finish_time - Simulator->Time();
			SuspendedTransactions.insert(SuspendedTransactions.begin(), ActiveTransactions.begin(), ActiveTransactions.end());
			Suspended = true;
			ActiveCommand = NULL;
			ActiveTransactions.clear();
			Free = true;
		}

		void PrepareResume()
		{
			ActiveCommand = SuspendedCommand;
			Expected_finish_time = Simulator->Time() + RemainingExecTime;
			ActiveTransactions.insert(ActiveTransactions.begin(), SuspendedTransactions.begin(), SuspendedTransactions.end());
			Suspended = false;
			SuspendedCommand = NULL;
			SuspendedTransactions.clear();
			Free = false;
		}

		void ClearCommand()
		{
			delete ActiveCommand;
			ActiveCommand = NULL;
			ActiveTransactions.clear();
			Free = true;
		}
	};

	class ChipBookKeepingEntry
	{
	public:
		ChipStatus Status;
		DieBookKeepingEntry* Die_book_keeping_records;
		sim_time_type Expected_command_exec_finish_time;
		sim_time_type Last_transfer_finish_time;
		bool HasSuspend;
		std::queue<DieBookKeepingEntry*> OngoingDieCMDTransfers;
		unsigned int WaitingReadTXCount;
		unsigned int No_of_active_dies;

		void PrepareSuspend() { HasSuspend = true; No_of_active_dies = 0; }
		void PrepareResume() { HasSuspend = false; }
	};

	class NVM_PHY_ONFI_NVDDR2 : public NVM_PHY_ONFI
	{
	public:
		NVM_PHY_ONFI_NVDDR2(const sim_object_id_type& id, ONFI_Channel_NVDDR2** channels,
			unsigned int ChannelCount, unsigned int chip_no_per_channel, unsigned int DieNoPerChip, unsigned int PlaneNoPerDie);
		void Setup_triggers();
		void Validate_simulation_config();
		void Start_simulation();

		void Send_command_to_chip(std::list<NVM_Transaction_Flash*>& transactionList);
		void Change_flash_page_status_for_preconditioning(const NVM::FlashMemory::Physical_Page_Address& page_address, const LPA_type lpa);
		void Execute_simulator_event(MQSimEngine::Sim_Event*);
		BusChannelStatus Get_channel_status(flash_channel_ID_type channelID);
		NVM::FlashMemory::Flash_Chip* Get_chip(flash_channel_ID_type channel_id, flash_chip_ID_type chip_id);
		LPA_type Get_metadata(flash_channel_ID_type channe_id, flash_chip_ID_type chip_id, flash_die_ID_type die_id, flash_plane_ID_type plane_id, flash_block_ID_type block_id, flash_page_ID_type page_id);//A simplification to decrease the complexity of GC execution! The GC unit may need to know the metadata of a page to decide if a page is valid or invalid. 
		bool HasSuspendedCommand(NVM::FlashMemory::Flash_Chip* chip);
		ChipStatus GetChipStatus(NVM::FlashMemory::Flash_Chip* chip);
		sim_time_type Expected_finish_time(NVM::FlashMemory::Flash_Chip* chip);
		sim_time_type Expected_finish_time(NVM_Transaction_Flash* transaction);
		sim_time_type Expected_transfer_time(NVM_Transaction_Flash* transaction);
		NVM_Transaction_Flash* Is_chip_busy_with_stream(NVM_Transaction_Flash* transaction);
		bool Is_chip_busy(NVM_Transaction_Flash* transaction);
		void Change_memory_status_preconditioning(const NVM::NVM_Memory_Address* address, const void* status_info);
	private:
		void transfer_read_data_from_chip(ChipBookKeepingEntry* chipBKE, DieBookKeepingEntry* dieBKE, NVM_Transaction_Flash* tr);
		void perform_interleaved_cmd_data_transfer(NVM::FlashMemory::Flash_Chip* chip, DieBookKeepingEntry* bookKeepingEntry);
		void send_resume_command_to_chip(NVM::FlashMemory::Flash_Chip* chip, ChipBookKeepingEntry* chipBKE);
		static void handle_ready_signal_from_chip(NVM::FlashMemory::Flash_Chip* chip, NVM::FlashMemory::Flash_Command* command);

		static NVM_PHY_ONFI_NVDDR2* _my_instance;
		ONFI_Channel_NVDDR2** channels;
		ChipBookKeepingEntry** bookKeepingTable;
		Flash_Transaction_Queue *WaitingReadTX, *WaitingGCRead_TX, *WaitingMappingRead_TX;
		std::list<DieBookKeepingEntry*> *WaitingCopybackWrites;
	};
}

#endif // !NVM_PHY_ONFI_NVDDR2_H
