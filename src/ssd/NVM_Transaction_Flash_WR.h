#ifndef NVM_TRANSACTION_FLASH_WR
#define NVM_TRANSACTION_FLASH_WR

#include "../nvm_chip/flash_memory/FlashTypes.h"
#include "../nvm_chip/NVM_Types.h"
#include "NVM_Transaction_Flash.h"
#include "NVM_Transaction_Flash_RD.h"
#include "NVM_Transaction_Flash_ER.h"

namespace SSD_Components
{
	class NVM_Transaction_Flash_ER;
	enum class WriteExecutionModeType { SIMPLE, COPYBACK };
	
	class NVM_Transaction_Flash_WR : public NVM_Transaction_Flash
	{
	public:
		NVM_Transaction_Flash_WR(Transaction_Source_Type source, stream_id_type stream_id,
			unsigned int data_size_in_byte, LPA_type lpa, PPA_type ppa, SSD_Components::User_Request* user_io_request,
			NVM::memory_content_type content, NVM_Transaction_Flash_RD* related_read, page_status_type write_sectors_bitmap, data_timestamp_type data_timestamp);
		NVM_Transaction_Flash_WR(Transaction_Source_Type source, stream_id_type stream_id,
			unsigned int data_size_in_byte, LPA_type lpa, PPA_type ppa, const NVM::FlashMemory::Physical_Page_Address& address,
			SSD_Components::User_Request* user_io_request, NVM::memory_content_type content,
			NVM_Transaction_Flash_RD* related_read, page_status_type write_sectors_bitmap, data_timestamp_type data_timestamp);
		NVM_Transaction_Flash_WR(Transaction_Source_Type source, stream_id_type stream_id,
			unsigned int data_size_in_byte, LPA_type lpa, SSD_Components::User_Request* user_io_request, IO_Flow_Priority_Class::Priority priority_class, NVM::memory_content_type content,
			page_status_type write_sectors_bitmap, data_timestamp_type data_timestamp);
		NVM_Transaction_Flash_WR(Transaction_Source_Type source, stream_id_type stream_id,
			unsigned int data_size_in_byte, LPA_type lpa, SSD_Components::User_Request* user_io_request, NVM::memory_content_type content,
			page_status_type write_sectors_bitmap, data_timestamp_type data_timestamp);
		NVM::memory_content_type Content; //The content of this transaction
		NVM_Transaction_Flash_RD* RelatedRead; //If this write request must be preceded by a read (for partial page write), this variable is used to point to the corresponding read request
		NVM_Transaction_Flash_ER* RelatedErase;
		page_status_type write_sectors_bitmap;
		data_timestamp_type DataTimeStamp;
		WriteExecutionModeType ExecutionMode;
	};
}

#endif // !WRITE_TRANSACTION_H
