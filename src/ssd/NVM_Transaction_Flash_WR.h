#ifndef NVM_TRANSACTION_FLASH_WR
#define NVM_TRANSACTION_FLASH_WR

#include "../nvm_chip/flash_memory/FlashTypes.h"
#include "NVM_Transaction_Flash.h"
#include "NVM_Transaction_Flash_RD.h"

namespace SSD_Components
{
	enum class WriteExecutionModeType { SIMPLE, COPYBACK };
	class NVM_Transaction_Flash_WR : public NVM_Transaction_Flash
	{
	public:
		NVM_Transaction_Flash_WR(Transaction_Source_Type source, stream_id_type streamID,
			unsigned int data_size_in_byte, LPA_type lpn, PPA_type ppn, SSD_Components::User_Request* userIORequest, uint64_t content,
			NVM_Transaction_Flash_RD* relatedRead, page_status_type write_sectors_bitmap, data_timestamp_type DataTimeStamp);
		NVM_Transaction_Flash_WR(Transaction_Source_Type source, stream_id_type streamID, unsigned int data_size_in_byte,
			LPA_type lpn, SSD_Components::User_Request* userIORequest, uint64_t content, page_status_type write_sectors_bitmap, 
			data_timestamp_type DataTimeStamp);
		uint64_t Content; //The content of this transaction
		NVM_Transaction_Flash_RD* RelatedRead; //If this write request must be preceded by a read (for partial page write), this variable is used to point to the corresponding read request
		page_status_type write_sectors_bitmap;
		data_timestamp_type DataTimeStamp;
		WriteExecutionModeType ExecutionMode;
	};
}

#endif // !WRITE_TRANSACTION_H
