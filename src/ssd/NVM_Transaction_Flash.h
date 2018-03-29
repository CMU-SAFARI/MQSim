#ifndef NVM_TRANSACTION_FLASH_H
#define NVM_TRANSACTION_FLASH_H

#include <string>
#include <list>
#include<cstdint>
#include "../sim/Sim_Defs.h"
#include "../sim/Sim_Event.h"
#include "../sim/Engine.h"
#include "../nvm_chip/flash_memory/FlashTypes.h"
#include "../nvm_chip/flash_memory/Flash_Chip.h"
#include "../nvm_chip/flash_memory/Physical_Page_Address.h"
#include "NVM_Transaction.h"
#include "User_Request.h"

namespace SSD_Components
{
	class User_Request;
	enum class Transaction_Source_Type { USERIO, CACHE, GC, MAPPING };
	enum class Transaction_Type { READ, WRITE, ERASE, UNKOWN };
	class NVM_Transaction_Flash : public NVM_Transaction
	{
	public:
		NVM_Transaction_Flash(Transaction_Source_Type source, Transaction_Type type, stream_id_type stream_id,
			unsigned int data_size_in_byte, LPA_type lpa, PPA_type ppa, User_Request* user_request);
		NVM_Transaction_Flash(Transaction_Source_Type source, Transaction_Type type, stream_id_type stream_id,
			unsigned int data_size_in_byte, LPA_type lpa, PPA_type ppa, const NVM::FlashMemory::Physical_Page_Address& address, User_Request* user_request);
		Transaction_Source_Type Source;
		Transaction_Type Type;
		NVM::FlashMemory::Physical_Page_Address Address;
		unsigned int Data_and_metadata_size_in_byte; //number of bytes contained in the request: bytes in the real page + bytes of metadata

		LPA_type LPA;
		PPA_type PPA;

		sim_time_type Issue_time;
		/* Used to calculate service time and transfer time for a normal read/program operation used to respond to the host IORequests.
		In other words, these variables are not important if FlashTransactions is used for garbage collection.*/
		sim_time_type STAT_execution_time, STAT_transfer_time;

		bool SuspendRequired;
		bool Physical_address_determined;
	private:

	};
}

#endif // !FLASH_TRANSACTION_H
