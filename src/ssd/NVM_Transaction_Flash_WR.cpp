#include "NVM_Transaction_Flash_WR.h"


namespace SSD_Components
{
	NVM_Transaction_Flash_WR::NVM_Transaction_Flash_WR(TransactionSourceType source, stream_id_type streamID,
		unsigned int data_size_in_byte, LPA_type lpn, PPA_type ppn, SSD_Components::User_Request* userIORequest, uint64_t content,
		NVM_Transaction_Flash_RD* relatedRead, page_status_type writtenSectorsBitmap, data_timestamp_type DataTimeStamp) :
		NVM_Transaction_Flash(source, TransactionType::WRITE, streamID, data_size_in_byte, lpn, ppn, userIORequest),
		Content(content), RelatedRead(relatedRead), write_sectors_bitmap(writtenSectorsBitmap), DataTimeStamp(DataTimeStamp),
		ExecutionMode(WriteExecutionModeType::SIMPLE)
	{}

	NVM_Transaction_Flash_WR::NVM_Transaction_Flash_WR(TransactionSourceType source, stream_id_type streamID,
		unsigned int data_size_in_byte, LPA_type lpn, SSD_Components::User_Request* userIORequest, uint64_t content,
		page_status_type writtenSectorsBitmap, data_timestamp_type DataTimeStamp) :
		NVM_Transaction_Flash(source, TransactionType::WRITE, streamID, data_size_in_byte, lpn, userIORequest),
		Content(content), RelatedRead(NULL), write_sectors_bitmap(writtenSectorsBitmap), DataTimeStamp(DataTimeStamp),
		ExecutionMode(WriteExecutionModeType::SIMPLE)
	{}
}