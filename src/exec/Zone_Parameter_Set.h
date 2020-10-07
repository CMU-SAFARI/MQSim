#ifndef ZONE_PARAMETER_SET_H
#define ZONE_PARAMETER_SET_H

#include "../sim/Sim_Defs.h"
#include "../nvm_chip/flash_memory/FlashTypes.h"
#include "../ssd/Address_Mapping_Unit_Base.h"
#include "Parameter_Set_Base.h"

class Zone_Parameter_Set : Parameter_Set_Base
{
public:
	static unsigned int Zone_Size;
	static unsigned int Channel_No_Per_Zone;
	static unsigned int Chip_No_Per_Zone;
	static unsigned int Die_No_Per_Zone;
	static unsigned int Plane_No_Per_Zone;
	SSD_Components::Zone_Allocation_Scheme_Type Zone_allocation_scheme;
	SSD_Components::SubZone_Allocation_Scheme_Type SubZone_allocation_scheme;
	void XML_serialize(Utils::XmlWriter& xmlwriter);
	void XML_deserialize(rapidxml::xml_node<> *node);
};

#endif // !ZONE_PARAMETER_SET_H
