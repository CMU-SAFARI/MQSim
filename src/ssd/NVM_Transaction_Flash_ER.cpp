#include "NVM_Transaction_Flash_ER.h"


namespace SSD_Components
{
	NVM_Transaction_Flash_ER::NVM_Transaction_Flash_ER(TransactionSourceType source, stream_id_type streamID,
		LPA_type lpn, PPA_type ppn, SSD_Components::User_Request* userIORequest) :
		NVM_Transaction_Flash(source, TransactionType::ERASE, streamID, 0, lpn, ppn, userIORequest)
	{}
}