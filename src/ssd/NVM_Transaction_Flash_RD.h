#ifndef NVM_TRANSACTION_FLASH_RD_H
#define NVM_TRANSACTION_FLASH_RD_H

#include "../nvm_chip/flash_memory/FlashTypes.h"
#include "NVM_Transaction_Flash.h"


namespace SSD_Components
{
	class NVM_Transaction_Flash_WR;
	class NVM_Transaction_Flash_RD : public NVM_Transaction_Flash
	{
	public:
		NVM_Transaction_Flash_RD(Transaction_Source_Type source, stream_id_type streamID,
			unsigned int data_size_in_byte, LPA_type lpn, PPA_type ppn, SSD_Components::User_Request* userIORequest,
			uint64_t content, NVM_Transaction_Flash_WR* relatedWrite, page_status_type readSectors, data_timestamp_type DataTimeStamp);
		NVM_Transaction_Flash_RD(Transaction_Source_Type source, stream_id_type streamID,
			unsigned int data_size_in_byte, LPA_type lpn, SSD_Components::User_Request* userIORequest, uint64_t content, page_status_type readSectorsBitmap, data_timestamp_type DataTimeStamp);
		uint64_t Content; //The content of this transaction
		NVM_Transaction_Flash_WR* RelatedWrite;		//Is this read request related to another write request and provides update data (for partial page write)
		page_status_type read_sectors_bitmap;
		data_timestamp_type DataTimeStamp;
	};
}

#endif