#ifndef ONFI_CHANNEL_BASE_H
#define ONFI_CHANNEL_BASE_H

#include "../nvm_chip/flash_memory/Chip.h"
#include "NVM_Channel_Base.h"

namespace SSD_Components
{
	enum class ONFI_Protocol {NVDDR2};
	class ONFI_Channel_Base : public NVM_Channel_Base
	{
	public:
		ONFI_Channel_Base(flash_channel_ID_type channelID, unsigned int chipCount, NVM::FlashMemory::Chip** flashChips, ONFI_Protocol type);
		flash_channel_ID_type ChannelID;
		BusChannelStatus Status;
		NVM::FlashMemory::Chip** Chips;
		ONFI_Protocol Type;
	};
}

#endif // !CHANNEL_H
