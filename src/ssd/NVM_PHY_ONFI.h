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
			unsigned int ChannelCount, unsigned int chip_no_per_channel, unsigned int DieNoPerChip, unsigned int PlaneNoPerDie)
			: NVM_PHY_Base(id),
			channel_count(ChannelCount), chip_no_per_channel(chip_no_per_channel), die_no_per_chip(DieNoPerChip), plane_no_per_die(PlaneNoPerDie){}
		~NVM_PHY_ONFI() {};

		virtual BusChannelStatus Get_channel_status(flash_channel_ID_type) = 0;
		virtual NVM::FlashMemory::Flash_Chip* Get_chip(flash_channel_ID_type channel_id, flash_chip_ID_type chip_id) = 0;
		virtual LPA_type Get_metadata(flash_channel_ID_type channe_id, flash_chip_ID_type chip_id, flash_die_ID_type die_id, flash_plane_ID_type plane_id, flash_block_ID_type block_id, flash_page_ID_type page_id) = 0;//A simplification to decrease the complexity of GC execution! The GC unit may need to know the metadata of a page to decide if a page is valid or invalid. 
		virtual bool HasSuspendedCommand(NVM::FlashMemory::Flash_Chip* chip) = 0;
		virtual ChipStatus GetChipStatus(NVM::FlashMemory::Flash_Chip* chip) = 0;
		virtual sim_time_type Expected_finish_time(NVM::FlashMemory::Flash_Chip* chip) = 0;
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
		unsigned int chip_no_per_channel;
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
