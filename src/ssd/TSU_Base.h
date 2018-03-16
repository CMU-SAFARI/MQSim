#ifndef TSU_H
#define TSU_H

#include <list>
#include "../sim/Sim_Defs.h"
#include "../sim/Sim_Object.h"
#include "../nvm_chip/flash_memory/Chip.h"
#include "FTL.h"
#include "NVM_PHY_ONFI_NVDDR2.h"
#include "FlashTransactionQueue.h"

namespace SSD_Components
{
	enum class Flash_Scheduling_Type {OUT_OF_ORDER};
	class FTL;
	class TSU_Base : public MQSimEngine::Sim_Object
	{
	public:
		TSU_Base(const sim_object_id_type& id, FTL* ftl, NVM_PHY_ONFI_NVDDR2* NVMController, Flash_Scheduling_Type Type,
			unsigned int ChannelNo, unsigned int ChipNoPerChannel, unsigned int DieNoPerChip, unsigned int PlaneNoPerDie,
			bool EraseSuspensionEnabled, bool ProgramSuspensionEnabled,
			sim_time_type WriteReasonableSuspensionTimeForRead,
			sim_time_type EraseReasonableSuspensionTimeForRead,
			sim_time_type EraseReasonableSuspensionTimeForWrite);
		void Setup_triggers();


		/*When an MQSim needs to send a set of transactions for execution, the following 
		* three funcitons should be invoked in this order:
		* Prepare_for_transaction_submit()
		* Submit_transaction(transaction)
		* .....
		* Submit_transaction(transaction)
		* Schedule()
		*
		* The above mentioned mechanism helps to exploit die-level and plane-level parallelism.
		* More precisely, first the transactions are queued and then, when the Schedule function
		* is invoked, the TSU has that opportunity to schedule them together to exploit multiplane
		* and die-interleaved execution.
		*/
		virtual void Prepare_for_transaction_submit() = 0;
		virtual void Submit_transaction(NVM_Transaction_Flash* transaction) = 0;
		
		/* Shedules the transactions currently stored in inputTransactionSlots. The transactions could
		* be mixes of reads, writes, and erases.
		*/
		virtual void Schedule() = 0;

	protected:
		FTL* ftl;
		NVM_PHY_ONFI_NVDDR2* _NVMController;
		Flash_Scheduling_Type type;
		unsigned int channel_count;
		unsigned int chip_no_per_channel;
		unsigned int die_no_per_chip;
		unsigned int plane_no_per_die;
		bool eraseSuspensionEnabled, programSuspensionEnabled;
		sim_time_type writeReasonableSuspensionTimeForRead;
		sim_time_type eraseReasonableSuspensionTimeForRead;//the time period 
		sim_time_type eraseReasonableSuspensionTimeForWrite;
		flash_chip_ID_type* LastChip;//Used for round-robin service of the chips in channels
		FlashTransactionQueue** UserReadTRQueue;
		FlashTransactionQueue** UserWriteTRQueue;
		FlashTransactionQueue** GCReadTRQueue;
		FlashTransactionQueue** GCWriteTRQueue;
		FlashTransactionQueue** GCEraseTRQueue;
		FlashTransactionQueue** MappingReadTRQueue;
		FlashTransactionQueue** MappingWriteTRQueue;

		static TSU_Base* _myInstance;
		std::list<NVM_Transaction_Flash*> incomingTransactionSlots;//Stores the transactions that are received for sheduling
		std::list<NVM_Transaction_Flash*> transaction_dispatch_slots;//Used to submit transactions to the channel controller
		virtual bool serviceReadTransaction(NVM::FlashMemory::Chip* chip) = 0;
		virtual bool serviceWriteTransaction(NVM::FlashMemory::Chip* chip) = 0;
		virtual bool serviceEraseTransaction(NVM::FlashMemory::Chip* chip) = 0;
		static void handle_transaction_serviced_signal_from_PHY(NVM_Transaction_Flash* transaction);
		static void handleChannelIdleSignal(flash_channel_ID_type);
		static void handleChipIdleSignal(NVM::FlashMemory::Chip* chip);
	};
}

#endif //TSU_H
