#include "NVM_Transaction_Flash.h"


namespace SSD_Components
{
	NVM_Transaction_Flash::NVM_Transaction_Flash(TransactionSourceType source, TransactionType type, stream_id_type stream_id,
		unsigned int data_size_in_byte, LPA_type lpn, PPA_type ppn, User_Request* user_request) :
		NVM_Transaction(stream_id, user_request),
		Source(source), Type(type),
		Data_and_metadata_size_in_byte(data_size_in_byte), LPA(lpn), PPA(ppn),
		Issue_time(Simulator->Time()), STAT_execution_time(INVALID_TIME), STAT_transfer_time(INVALID_TIME), Physical_address_determined(false)
	{
	}

	NVM_Transaction_Flash::NVM_Transaction_Flash(TransactionSourceType source, TransactionType type, stream_id_type stream_id,
		unsigned int data_size_in_byte, LPA_type lpn, User_Request* user_request) :
		NVM_Transaction(stream_id, user_request),
		Source(source), Type(type), Data_and_metadata_size_in_byte(data_size_in_byte), LPA(lpn), PPA(NO_PPA),
		Issue_time(Simulator->Time()), STAT_execution_time(INVALID_TIME), STAT_transfer_time(INVALID_TIME), Physical_address_determined(false)
	{
	}
}
