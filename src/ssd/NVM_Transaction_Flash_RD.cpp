#include "NVM_Transaction_Flash_RD.h"


namespace SSD_Components
{
	NVM_Transaction_Flash_RD::NVM_Transaction_Flash_RD(Transaction_Source_Type source, stream_id_type streamID,
		unsigned int data_size_in_byte, LPA_type lpn, PPA_type ppn, SSD_Components::User_Request* userIORequest,
		uint64_t content, NVM_Transaction_Flash_WR* relatedWrite, page_status_type readSectorsBitmap, data_timestamp_type DataTimeStamp) :
		NVM_Transaction_Flash(source, TransactionType::READ, streamID, data_size_in_byte, lpn, ppn, userIORequest),
		Content(content), RelatedWrite(relatedWrite), read_sectors_bitmap(readSectorsBitmap), DataTimeStamp(DataTimeStamp)
	{}
	NVM_Transaction_Flash_RD::NVM_Transaction_Flash_RD(Transaction_Source_Type source, stream_id_type streamID,
		unsigned int data_size_in_byte, LPA_type lpn, SSD_Components::User_Request* userIORequest, uint64_t content, page_status_type readSectorsBitmap, data_timestamp_type DataTimeStamp) :
		NVM_Transaction_Flash(source, TransactionType::READ, streamID, data_size_in_byte, lpn, userIORequest),
		Content(content), RelatedWrite(NULL), read_sectors_bitmap(readSectorsBitmap), DataTimeStamp(DataTimeStamp)
	{}
}