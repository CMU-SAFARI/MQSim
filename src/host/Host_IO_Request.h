#ifndef HOST_IO_REQUEST_H
#define HOST_IO_REQUEST_H

#include "../ssd/SSDTypes.h"

namespace Host_Components
{
	class Host_IO_Reqeust
	{
	public:
		sim_time_type Arrival_time;//The time that the request has been generated
		sim_time_type Enqueue_time;//The time that the request enqueued into the I/O queue
		LSA_type Start_LBA;
		unsigned int LBA_count;
		bool Is_read;
		uint16_t IO_queue_info;
	};
}

#endif // !HOST_IO_REQUEST_H
