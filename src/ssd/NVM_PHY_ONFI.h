#ifndef NVM_PHY_ONFI_ONFI_H
#define NVM_PHY_ONFI_ONFI_H

#include <vector>
#include "../nvm_chip/flash_memory/Flash_Command.h"
#include "../nvm_chip/flash_memory/Flash_Chip.h"
#include "NVM_Transaction_Flash.h"
#include "NVM_Transaction_Flash_RD.h"
#include "NVM_Transaction_Flash_WR.h"
#include "NVM_Transaction_Flash_ER.h"
#include "NVM_PHY_Base.h"
#include "ONFI_Channel_Base.h"


namespace SSD_Components
{
	enum class ChipStatus { IDLE, CMD_IN, CMD_DATA_IN, DATA_OUT, READING, WRITING, ERASING, WAIT_FOR_DATA_OUT, WAIT_FOR_COPYBACK_CMD };
	class NVM_PHY_ONFI : public NVM_PHY_Base
	{
	public:
		NVM_PHY_ONFI(sim_object_id_type id,
			unsigned int ChannelCount, unsigned int ChipNoPerChannel, unsigned int DieNoPerChip, unsigned int PlaneNoPerDie)
			: NVM_PHY_Base(id),
			channel_count(ChannelCount), ChipNoPerChannel(ChipNoPerChannel), die_no_per_chip(DieNoPerChip), plane_no_per_die(PlaneNoPerDie){}
		~NVM_PHY_ONFI() {};


		virtual BusChannelStatus GetChannelStatus(flash_channel_ID_type) = 0;
		virtual NVM::FlashMemory::Flash_Chip* GetChip(flash_channel_ID_type channelID, flash_chip_ID_type chipID) = 0;
		virtual bool HasSuspendedCommand(NVM::FlashMemory::Flash_Chip* chip) = 0;
		virtual ChipStatus GetChipStatus(NVM::FlashMemory::Flash_Chip* chip) = 0;
		virtual sim_time_type ExpectedFinishTime(NVM::FlashMemory::Flash_Chip* chip) = 0;
		/// Provides communication between controller and NVM chips for a simple read/write/erase command.
		virtual void Send_command_to_chip(std::list<NVM_Transaction_Flash*>& transactionList) = 0;
		virtual void Change_flash_page_status_for_preconditioning(const NVM::FlashMemory::Physical_Page_Address& page_address, const LPA_type lpa) = 0;

		typedef void(*TransactionServicedHandlerType) (NVM_Transaction_Flash*);
		void ConnectToTransactionServicedSignal(TransactionServicedHandlerType);
		typedef void(*ChannelIdleHandlerType) (flash_channel_ID_type);
		void ConnectToChannelIdleSignal(ChannelIdleHandlerType);
		typedef void(*ChipIdleHandlerType) (NVM::FlashMemory::Flash_Chip*);
		void ConnectToChipIdleSignal(ChipIdleHandlerType);
	protected:
		unsigned int channel_count;
		unsigned int ChipNoPerChannel;
		unsigned int die_no_per_chip;
		unsigned int plane_no_per_die;
		std::vector<TransactionServicedHandlerType> connectedTransactionServicedHandlers;
		void broadcastTransactionServicedSignal(NVM_Transaction_Flash* transaction);
		std::vector<ChannelIdleHandlerType> connectedChannelIdleHandlers;
		void broadcastChannelIdleSignal(flash_channel_ID_type);
		std::vector<ChipIdleHandlerType> connectedChipIdleHandlers;
		void broadcastChipIdleSignal(NVM::FlashMemory::Flash_Chip* chip);
	};
}


#endif // !NVM_PHY_ONFI_ONFI_H
