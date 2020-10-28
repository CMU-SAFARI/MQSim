#ifndef ZONE_H
#define ZONE_H

#include "FlashTypes.h"


namespace NVM
{
	namespace FlashMemory
	{
		enum class Zone_Status {EMPTY, OPENED, CLOSED, OFFLINE, FULL};
	
		class Zone
		{
		public:
			Zone(Zone_ID_type ZoneID, unsigned int Channel_No_Per_Zone, unsigned int Chip_No_Per_Zone, unsigned int Die_No_Per_Zone, unsigned int Plane_No_Per_Zone);
			~Zone();
			Zone_ID_type ID;
			unsigned int write_point;
			Zone_Status zone_status;
			unsigned int erase_count;
		};
	}
}
#endif // ! ZONE_H
