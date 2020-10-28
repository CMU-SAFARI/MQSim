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
			erase_count = 0;
			zone_status = Zone_Status::EMPTY;
			Has_ongoing_gc_wl = false;
		}

		Zone::~Zone()
		{
		
		}
	}
}
