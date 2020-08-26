#ifndef ZONE_H
#define ZONE_H

#include "FlashTypes.h"


namespace NVM
{
	namespace FlashMemory
	{
		class Zone
		{
		public:
			Zone(Zone_ID_type ZoneID);
			~Zone();
			//SubZone* SubZones;
			Zone_ID_type ID;
		};
	}
}
#endif // ! ZONE_H
