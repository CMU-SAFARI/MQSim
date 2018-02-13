#ifndef NVM_TRANSACTION_H
#define NVM_TRANSACTION_H

#include <list>
#include "../sim/Sim_Defs.h"
#include "User_Request.h"

namespace SSD_Components
{
	class User_Request;
	class NVM_Transaction 
	{
	public:
		NVM_Transaction(stream_id_type stream_id, User_Request* user_request) :
			Stream_id(stream_id), UserIORequest(user_request) {}
		stream_id_type Stream_id;
		User_Request* UserIORequest;
		//std::list<NVM_Transaction*>::iterator RelatedNodeInQueue;//Just used for high performance linkedlist insertion/deletion
	};
}

#endif //!NVM_TRANSACTION_H