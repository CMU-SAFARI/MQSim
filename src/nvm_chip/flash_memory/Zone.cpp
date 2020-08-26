#include "Zone.h"

namespace NVM
{
	namespace FlashMemory
	{
		Zone::Zone(Zone_ID_type ZoneID)
		{
			ID = ZoneID;
			//SubZone = new SubZone[SubZoneNo];
		}

		Zone::~Zone()
		{
		
		}
	}
}
