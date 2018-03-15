#include "Host_Parameter_Set.h"


double Host_Parameter_Set::PCIe_Lane_Bandwidth = 0.4;//uint is GB/s
unsigned int Host_Parameter_Set::PCIe_Lane_Count = 4;
std::vector<IO_Flow_Parameter_Set*> Host_Parameter_Set::IO_Flow_Definitions;

void Host_Parameter_Set::XML_serialize(Utils::XmlWriter& xmlwriter)
{
	std::string tmp;
	tmp = "Host_Parameter_Set";
	xmlwriter.Write_open_tag(tmp);

	std::string attr = "PCIe_Lane_Bandwidth";
	std::string val = std::to_string(PCIe_Lane_Bandwidth);
	xmlwriter.Write_attribute_string(attr, val);

	attr = "PCIe_Lane_Count";
	val = std::to_string(PCIe_Lane_Count);
	xmlwriter.Write_attribute_string(attr, val);

	xmlwriter.Write_close_tag();
}

void Host_Parameter_Set::XML_deserialize(rapidxml::xml_node<> *node)
{
	try
	{
		for (auto param = node->first_node(); param; param = param->next_sibling())
		{
			if (strcmp(param->name(), "PCIe_Lane_Bandwidth") == 0)
			{
				std::string val = param->value();
				PCIe_Lane_Bandwidth = std::stod(val);
			}
			else if (strcmp(param->name(), "PCIe_Lane_Count") == 0)
			{
				std::string val = param->value();
				PCIe_Lane_Count = std::stoul(val);
			}
		}
	}
	catch (...)
	{
		PRINT_ERROR("Error in the Host_Parameter_Set!")
	}
}