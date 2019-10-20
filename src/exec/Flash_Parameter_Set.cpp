#include <algorithm>
#include <string.h>
#include "../sim/Engine.h"
#include "Flash_Parameter_Set.h"

Flash_Technology_Type Flash_Parameter_Set::Flash_Technology = Flash_Technology_Type::MLC;
NVM::FlashMemory::Command_Suspension_Mode Flash_Parameter_Set::CMD_Suspension_Support = NVM::FlashMemory::Command_Suspension_Mode::ERASE;
sim_time_type Flash_Parameter_Set::Page_Read_Latency_LSB = 75000;
sim_time_type Flash_Parameter_Set::Page_Read_Latency_CSB = 75000;
sim_time_type Flash_Parameter_Set::Page_Read_Latency_MSB = 75000;
sim_time_type Flash_Parameter_Set::Page_Program_Latency_LSB = 750000;
sim_time_type Flash_Parameter_Set::Page_Program_Latency_CSB = 750000;
sim_time_type Flash_Parameter_Set::Page_Program_Latency_MSB = 750000;
sim_time_type Flash_Parameter_Set::Block_Erase_Latency = 3800000;//Block erase latency in nano-seconds
unsigned int Flash_Parameter_Set::Block_PE_Cycles_Limit = 10000;
sim_time_type Flash_Parameter_Set::Suspend_Erase_Time = 700000;//in nano-seconds
sim_time_type Flash_Parameter_Set::Suspend_Program_Time = 100000;//in nano-seconds
unsigned int Flash_Parameter_Set::Die_No_Per_Chip = 2;
unsigned int Flash_Parameter_Set::Plane_No_Per_Die = 2;
unsigned int Flash_Parameter_Set::Block_No_Per_Plane = 2048;
unsigned int Flash_Parameter_Set::Page_No_Per_Block = 256;//Page no per block
unsigned int Flash_Parameter_Set::Page_Capacity = 8192;//Flash page capacity in bytes
unsigned int Flash_Parameter_Set::Page_Metadat_Capacity = 1872;//Flash page capacity in bytes

void Flash_Parameter_Set::XML_serialize(Utils::XmlWriter& xmlwriter)
{
	std::string tmp;
	tmp = "Flash_Parameter_Set";
	xmlwriter.Write_open_tag(tmp);

	std::string attr = "Flash_Technology";
	std::string val;
	switch (Flash_Technology) {
		case Flash_Technology_Type::SLC:
			val = "SLC";
			break;
		case Flash_Technology_Type::MLC:
			val = "MLC";
			break;
		case Flash_Technology_Type::TLC:
			val = "TLC";
			break;
		default:
			break;
	}
	xmlwriter.Write_attribute_string(attr, val);

	attr = "CMD_Suspension_Support";
	switch (CMD_Suspension_Support) {
		case NVM::FlashMemory::Command_Suspension_Mode::NONE:
			val = "NONE";
			break;
		case NVM::FlashMemory::Command_Suspension_Mode::ERASE:
			val = "ERASE";
			break;
		case NVM::FlashMemory::Command_Suspension_Mode::PROGRAM:
			val = "PROGRAM";
			break;
		case NVM::FlashMemory::Command_Suspension_Mode::PROGRAM_ERASE:
			val = "PROGRAM_ERASE";
			break;
		default:
			break;
	}
	xmlwriter.Write_attribute_string(attr, val);

	attr = "Page_Read_Latency_LSB";
	val = std::to_string(Page_Read_Latency_LSB);
	xmlwriter.Write_attribute_string(attr, val);

	attr = "Page_Read_Latency_CSB";
	val = std::to_string(Page_Read_Latency_CSB);
	xmlwriter.Write_attribute_string(attr, val);

	attr = "Page_Read_Latency_MSB";
	val = std::to_string(Page_Read_Latency_MSB);
	xmlwriter.Write_attribute_string(attr, val);

	attr = "Page_Program_Latency_LSB";
	val = std::to_string(Page_Program_Latency_LSB);
	xmlwriter.Write_attribute_string(attr, val);

	attr = "Page_Program_Latency_CSB";
	val = std::to_string(Page_Program_Latency_CSB);
	xmlwriter.Write_attribute_string(attr, val);

	attr = "Page_Program_Latency_MSB";
	val = std::to_string(Page_Program_Latency_MSB);
	xmlwriter.Write_attribute_string(attr, val);

	attr = "Block_Erase_Latency";
	val = std::to_string(Block_Erase_Latency);
	xmlwriter.Write_attribute_string(attr, val);

	attr = "Block_PE_Cycles_Limit";
	val = std::to_string(Block_PE_Cycles_Limit);
	xmlwriter.Write_attribute_string(attr, val);

	attr = "Suspend_Erase_Time";
	val = std::to_string(Suspend_Erase_Time);
	xmlwriter.Write_attribute_string(attr, val);

	attr = "Suspend_Program_Time";
	val = std::to_string(Suspend_Program_Time);
	xmlwriter.Write_attribute_string(attr, val);

	attr = "Die_No_Per_Chip";
	val = std::to_string(Die_No_Per_Chip);
	xmlwriter.Write_attribute_string(attr, val);

	attr = "Plane_No_Per_Die";
	val = std::to_string(Plane_No_Per_Die);
	xmlwriter.Write_attribute_string(attr, val);

	attr = "Block_No_Per_Plane";
	val = std::to_string(Block_No_Per_Plane);
	xmlwriter.Write_attribute_string(attr, val);

	attr = "Page_No_Per_Block";
	val = std::to_string(Page_No_Per_Block);
	xmlwriter.Write_attribute_string(attr, val);

	attr = "Page_Capacity";
	val = std::to_string(Page_Capacity);
	xmlwriter.Write_attribute_string(attr, val);

	attr = "Page_Metadat_Capacity";
	val = std::to_string(Page_Metadat_Capacity);
	xmlwriter.Write_attribute_string(attr, val);

	xmlwriter.Write_close_tag();
}

void Flash_Parameter_Set::XML_deserialize(rapidxml::xml_node<> *node)
{
	try {
		for (auto param = node->first_node(); param; param = param->next_sibling()) {
			if (strcmp(param->name(), "Flash_Technology") == 0) {
				std::string val = param->value();
				std::transform(val.begin(), val.end(), val.begin(), ::toupper);
				if (strcmp(val.c_str(), "SLC") == 0)
					Flash_Technology = Flash_Technology_Type::SLC;
				else if (strcmp(val.c_str(), "MLC") == 0)
					Flash_Technology = Flash_Technology_Type::MLC;
				else if (strcmp(val.c_str(), "TLC") == 0)
					Flash_Technology = Flash_Technology_Type::TLC;
				else PRINT_ERROR("Unknown flash technology type specified in the input file")
			} else if (strcmp(param->name(), "CMD_Suspension_Support") == 0) {
				std::string val = param->value();
				std::transform(val.begin(), val.end(), val.begin(), ::toupper);
				if (strcmp(val.c_str(), "NONE") == 0) {
					CMD_Suspension_Support = NVM::FlashMemory::Command_Suspension_Mode::NONE;
				} else if (strcmp(val.c_str(), "ERASE") == 0) {
					CMD_Suspension_Support = NVM::FlashMemory::Command_Suspension_Mode::ERASE;
				} else if (strcmp(val.c_str(), "PROGRAM") == 0) {
					CMD_Suspension_Support = NVM::FlashMemory::Command_Suspension_Mode::PROGRAM;
				} else if (strcmp(val.c_str(), "PROGRAM_ERASE") == 0) {
					CMD_Suspension_Support = NVM::FlashMemory::Command_Suspension_Mode::PROGRAM_ERASE;
				} else {
					PRINT_ERROR("Unknown command suspension type specified in the input file")
				}
			} else if (strcmp(param->name(), "Page_Read_Latency_LSB") == 0) {
				std::string val = param->value();
				Page_Read_Latency_LSB = std::stoull(val);
			} else if (strcmp(param->name(), "Page_Read_Latency_CSB") == 0) {
				std::string val = param->value();
				Page_Read_Latency_CSB = std::stoull(val);
			} else if (strcmp(param->name(), "Page_Read_Latency_MSB") == 0) {
				std::string val = param->value();
				Page_Read_Latency_MSB = std::stoull(val);
			} else if (strcmp(param->name(), "Page_Program_Latency_LSB") == 0) {
				std::string val = param->value();
				Page_Program_Latency_LSB = std::stoull(val);
			} else if (strcmp(param->name(), "Page_Program_Latency_CSB") == 0) {
				std::string val = param->value();
				Page_Program_Latency_CSB = std::stoull(val);
			} else if (strcmp(param->name(), "Page_Program_Latency_MSB") == 0) {
				std::string val = param->value();
				Page_Program_Latency_MSB = std::stoull(val);
			} else if (strcmp(param->name(), "Block_Erase_Latency") == 0) {
				std::string val = param->value();
				Block_Erase_Latency = std::stoull(val);
			} else if (strcmp(param->name(), "Block_PE_Cycles_Limit") == 0) {
				std::string val = param->value();
				Block_PE_Cycles_Limit = std::stoul(val);
			} else if (strcmp(param->name(), "Suspend_Erase_Time") == 0) {
				std::string val = param->value();
				Suspend_Erase_Time = std::stoull(val);
			} else if (strcmp(param->name(), "Suspend_Program_Time") == 0) {
				std::string val = param->value();
				Suspend_Program_Time = std::stoull(val);
			} else if (strcmp(param->name(), "Die_No_Per_Chip") == 0) {
				std::string val = param->value();
				Die_No_Per_Chip = std::stoul(val);
			} else if (strcmp(param->name(), "Plane_No_Per_Die") == 0) {
				std::string val = param->value();
				Plane_No_Per_Die = std::stoul(val);
			} else if (strcmp(param->name(), "Block_No_Per_Plane") == 0) {
				std::string val = param->value();
				Block_No_Per_Plane = std::stoul(val);
			} else if (strcmp(param->name(), "Page_No_Per_Block") == 0) {
				std::string val = param->value();
				Page_No_Per_Block = std::stoul(val);
			} else if (strcmp(param->name(), "Page_Capacity") == 0) {
				std::string val = param->value();
				Page_Capacity = std::stoul(val);
			} else if (strcmp(param->name(), "Page_Metadat_Capacity") == 0) {
				std::string val = param->value();
				Page_Metadat_Capacity = std::stoul(val);
			}
		}
	} catch (...) {
		PRINT_ERROR("Error in the Flash_Parameter_Set!")
	}
}
