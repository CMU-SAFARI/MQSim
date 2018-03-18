#include "NVM_Transaction_Flash_ER.h"


namespace SSD_Components
{
	NVM_Transaction_Flash_ER::NVM_Transaction_Flash_ER(Transaction_Source_Type source, stream_id_type streamID,
		NVM::FlashMemory::Physical_Page_Address address) :
		NVM_Transaction_Flash(source, TransactionType::ERASE, streamID, address, NULL)
	{}
}