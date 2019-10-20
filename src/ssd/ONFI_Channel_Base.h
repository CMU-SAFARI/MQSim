#ifndef ONFI_CHANNEL_BASE_H
#define ONFI_CHANNEL_BASE_H

#include "../nvm_chip/flash_memory/Flash_Chip.h"
#include "NVM_Channel_Base.h"

namespace SSD_Components
{
	enum class ONFI_Protocol {NVDDR2};
	class ONFI_Channel_Base : public NVM_Channel_Base
	{
	public:
		ONFI_Channel_Base(flash_channel_ID_type channelID, unsigned int chipCount, NVM::FlashMemory::Flash_Chip** flashChips, ONFI_Protocol type);
		flash_channel_ID_type ChannelID;
		NVM::FlashMemory::Flash_Chip** Chips;
		ONFI_Protocol Type;

		BusChannelStatus GetStatus()
		{
			return status;
		}

		void SetStatus(BusChannelStatus new_status, NVM::FlashMemory::Flash_Chip* target_chip)
		{
			if (((status == BusChannelStatus::IDLE && new_status == BusChannelStatus::IDLE)
				|| (status == BusChannelStatus::BUSY && new_status == BusChannelStatus::BUSY))
				&& (current_active_chip != target_chip)) {
				PRINT_ERROR("Bus " << ChannelID << ": illegal bus status transition!")
			}

			status = new_status;
			if (status == BusChannelStatus::BUSY) {
				current_active_chip = target_chip;
			} else {
				current_active_chip = NULL;
			}
		}
	private:
		BusChannelStatus status;
		NVM::FlashMemory::Flash_Chip* current_active_chip;
	};
}

#endif // !CHANNEL_H
