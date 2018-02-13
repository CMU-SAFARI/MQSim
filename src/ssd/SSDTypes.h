#ifndef SSD_TYPE_H
#define SSD_TYPE_H

#include<cstdint>
#include <string>
#include "../sim/Sim_Defs.h"
#include "../nvm_chip/flash_memory/FlashTypes.h"

//enum class Memory_Type {FLASH};

typedef uint32_t host_pointer_type;
typedef uint64_t LSA_type;
typedef std::string io_request_type;
typedef uint64_t data_cache_content_type;
#define SECTOR_SIZE_IN_BYTE 512
#define MAX_SUPPORT_STREAMS 256 //this value shouldn't be increased as some other parameters are set based on the maximum number of 256

/* Since MQSim supports shared resources, such as DataCache and CMT,
* it needs to make the keys (i.e., LPNs) to access these resources
* unique (i.e., same LPNs from different streams must be treated as 
* different keys). To create this unique key, MQSim assumes that
* there are at most 256 concurrent input streams (a typical value
* in modern MQ-SSDs). The value 56 in the below macro is calculated
* as (64 - log_2(256)).
*/
#define LPN_TO_UNIQUE_KEY(S,L) ((((LPA_type)S)<<56)|L)


inline unsigned int sector_count(const page_status_type page_status)
{
	unsigned int size = 0;
	for (int i = 0; i < 64; i++)
		if ((((page_status_type)1) << i) & page_status)
			size++;
	return size;
}


#endif // !SSD_TYPE_H
