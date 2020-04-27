#include "NVM_Transaction_Flash.h"


namespace SSD_Components
{
	NVM_Transaction_Flash::NVM_Transaction_Flash(Transaction_Source_Type source, Transaction_Type type, stream_id_type stream_id,
		unsigned int data_size_in_byte, LPA_type lpa, PPA_type ppa, User_Request* user_request, IO_Flow_Priority_Class::Priority priority_class) :
		NVM_Transaction(stream_id, source, type, user_request, priority_class),
		Data_and_metadata_size_in_byte(data_size_in_byte), LPA(lpa), PPA(ppa), Physical_address_determined(false), FLIN_Barrier(false)
	{
	}
	
	NVM_Transaction_Flash::NVM_Transaction_Flash(Transaction_Source_Type source, Transaction_Type type, stream_id_type stream_id,
		unsigned int data_size_in_byte, LPA_type lpa, PPA_type ppa, const NVM::FlashMemory::Physical_Page_Address& address, User_Request* user_request, IO_Flow_Priority_Class::Priority priority_class) :
		NVM_Transaction(stream_id, source, type, user_request, priority_class), Data_and_metadata_size_in_byte(data_size_in_byte), LPA(lpa), PPA(ppa), Address(address), Physical_address_determined(false)
	{
	}
}
