#include "User_Request.h"

namespace SSD_Components
{
	unsigned int User_Request::lastId = 0;

	User_Request::User_Request() : Sectors_serviced_from_cache(0)
	{
		ID = "" + std::to_string(lastId++);
		ToBeIgnored = false;
	}
}
