#ifndef NVM_TRANSACTION_FLASH_ER_H
#define NVM_TRANSACTION_FLASH_ER_H

#include "../nvm_chip/flash_memory/FlashTypes.h"
#include "NVM_Transaction_Flash.h"

namespace SSD_Components
{
	class NVM_Transaction_Flash_ER : public NVM_Transaction_Flash
	{
	public:
		NVM_Transaction_Flash_ER(TransactionSourceType source, stream_id_type streamID,
			LPA_type lpn, PPA_type ppn, SSD_Components::User_Request* userIORequest);
	};
}

#endif // !ERASE_TRANSACTION_H
