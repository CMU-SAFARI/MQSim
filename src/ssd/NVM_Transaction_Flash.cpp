#include "NVM_Transaction_Flash.h"


namespace SSD_Components
{
	NVM_Transaction_Flash::NVM_Transaction_Flash(Transaction_Source_Type source, Transaction_Type type, stream_id_type stream_id,
		unsigned int data_size_in_byte, LPA_type lpa, PPA_type ppa, User_Request* user_request) :
		NVM_Transaction(stream_id, user_request),
		Source(source), Type(type),
		Data_and_metadata_size_in_byte(data_size_in_byte), LPA(lpa), PPA(ppa),
		Issue_time(Simulator->Time()), STAT_execution_time(INVALID_TIME), STAT_transfer_time(INVALID_TIME), Physical_address_determined(false)
	{
	}
	
	NVM_Transaction_Flash::NVM_Transaction_Flash(Transaction_Source_Type source, Transaction_Type type, stream_id_type stream_id,
		unsigned int data_size_in_byte, LPA_type lpa, PPA_type ppa, const NVM::FlashMemory::Physical_Page_Address& address, User_Request* user_request) :
		NVM_Transaction(stream_id, user_request),
		Source(source), Type(type), Data_and_metadata_size_in_byte(data_size_in_byte), LPA(lpa), PPA(ppa), Address(address),
		Issue_time(Simulator->Time()), STAT_execution_time(INVALID_TIME), STAT_transfer_time(INVALID_TIME), Physical_address_determined(false)

	{}
}
