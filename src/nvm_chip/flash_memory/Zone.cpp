#include "Zone.h"

namespace NVM
{
	namespace FlashMemory
	{
		Zone::Zone(Zone_ID_type ZoneID,
					unsigned int Channel_No_Per_Zone, 
					unsigned int Chip_No_Per_Zone, 
					unsigned int Die_No_Per_Zone, 
					unsigned int Plane_No_Per_Zone)
		{
			ID = ZoneID;
			write_point = 0;
			//SubZone = new SubZone[SubZoneNo];
			// TODO!!
			// how to express a physical zone? 
			// which channels, which chips, which dies, planes?
		}

		Zone::~Zone()
		{
		
		}
	}
}
