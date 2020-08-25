#include <algorithm>
#include <string.h>
#include "../sim/Engine.h"
#include "Zone_Parameter_Set.h"

unsigned int Zone_Parameter_Set::Zone_Size = 256; // Zone size in MB
unsigned int Zone_Parameter_Set::Channel_No_Per_Zone = 8;
unsigned int Zone_Parameter_Set::Chip_No_Per_Zone = 2;
unsigned int Zone_Parameter_Set::Die_No_Per_Zone = 2;
unsigned int Zone_Parameter_Set::Plane_No_Per_Zone = 2;

void Zone_Parameter_Set::XML_serialize(Utils::XmlWriter& xmlwriter)
{
	std::string tmp;
	tmp = "Zone_Parameter_Set";
	xmlwriter.Write_open_tag(tmp);

	std::string attr = "Zone_Size";
	std::string val = std::to_string(Zone_Size);
	xmlwriter.Write_attribute_string(attr, val);

	attr = "Channel_No_Per_Zone";
	val = std::to_string(Channel_No_Per_Zone);
	xmlwriter.Write_attribute_string(attr, val);

	attr = "Chip_No_Per_Zone";
	val = std::to_string(Chip_No_Per_Zone);
	xmlwriter.Write_attribute_string(attr, val);

	attr = "Die_No_Per_Zone";
	val = std::to_string(Die_No_Per_Zone);
	xmlwriter.Write_attribute_string(attr, val);

	attr = "Plane_No_Per_Zone";
	val = std::to_string(Plane_No_Per_Zone);
	xmlwriter.Write_attribute_string(attr, val);

	// attr = "Zone_Allocation_Scheme";
	// switch (Zone_Allocation_Scheme) {
	// 	default:
	// 		break;
	// }
	// xmlwriter.Write_attribute_string(attr, val);

	// attr = "SubZone_Allocation_Scheme";
	// switch (SubZone_Allocation_Scheme) {
	// 	default:
	// 		break;
	// }
	// xmlwriter.Write_attribute_string(attr, val);

	xmlwriter.Write_close_tag();
}

void Zone_Parameter_Set::XML_deserialize(rapidxml::xml_node<> *node)
{
	try {
		for (auto param = node->first_node(); param; param = param->next_sibling()) {
			if (strcmp(param->name(), "Zone_Size") == 0) {
				std::string val = param->value();
				Zone_Size = std::stoull(val);
			} else if (strcmp(param->name(), "Channel_No_Per_Zone") == 0) {
				std::string val = param->value();
				Channel_No_Per_Zone = std::stoull(val);
			} else if (strcmp(param->name(), "Chip_No_Per_Zone") == 0) {
				std::string val = param->value();
				Chip_No_Per_Zone = std::stoull(val);
			} else if (strcmp(param->name(), "Die_No_Per_Zone") == 0) {
				std::string val = param->value();
				Die_No_Per_Zone = std::stoull(val);	
			} else if (strcmp(param->name(), "Plane_No_Per_Zone") == 0) {
				std::string val = param->value();
				Plane_No_Per_Zone = std::stoull(val);
			// } else if (strcmp(param->name(), "Zone_Allocation_Scheme") == 0) {
			// 	std::string val = param->value();
			// 	std::transform(val.begin(), val.end(), val.begin(), ::toupper);
			// 	if (strcmp(val.c_str(), "CDPW") == 0) {
			// 	} else {
			// 		PRINT_ERROR("Unknown command suspension type specified in the input file")
			// 	}
			// } else if (strcmp(param->name(), "SubZone_Allocation_Scheme") == 0) {
			// 	std::string val = param->value();
			// 	std::transform(val.begin(), val.end(), val.begin(), ::toupper);
			// 	if (strcmp(val.c_str(), "CDPW") == 0) {
			// 	} else {
			// 		PRINT_ERROR("Unknown command suspension type specified in the input file")
			// 	}
			}
		}
	} catch (...) {
		PRINT_ERROR("Error in the Flash_Parameter_Set!")
	}
}
