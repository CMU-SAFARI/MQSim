#include "IO_Flow_Parameter_Set.h"
#include <string>
#include <set>
#include <cstring>
#include <algorithm>

//All serialization and deserialization functions should be replaced by a C++ reflection implementation
void IO_Flow_Parameter_Set::XML_serialize(Utils::XmlWriter& xmlwriter)
{
	std::string attr = "Priority_Class";
	std::string val;
	switch (Priority_Class) {
		case IO_Flow_Priority_Class::URGENT:
			val = "URGENT";
			break;
		case IO_Flow_Priority_Class::HIGH:
			val = "HIGH";
			break;
		case IO_Flow_Priority_Class::MEDIUM:
			val = "MEDIUM";
			break;
		case IO_Flow_Priority_Class::LOW:
			val = "LOW";
			break;
		default:
			break;
	}
	xmlwriter.Write_attribute_string(attr, val);


	attr = "Device_Level_Data_Caching_Mode";
	switch (Device_Level_Data_Caching_Mode) {
		case SSD_Components::Caching_Mode::TURNED_OFF:
			val = "TURNED_OFF";
			break;
		case SSD_Components::Caching_Mode::READ_CACHE:
			val = "READ_CACHE";
			break;
		case SSD_Components::Caching_Mode::WRITE_CACHE:
			val = "WRITE_CACHE";
			break;
		case SSD_Components::Caching_Mode::WRITE_READ_CACHE:
			val = "WRITE_READ_CACHE";
			break;
	}
	xmlwriter.Write_attribute_string(attr, val);

	attr = "Channel_IDs";
	val = "";
	for (int i = 0; i < Channel_No; i++) {
		if (i > 0) {
			val += ",";
		}
		val += std::to_string(Channel_IDs[i]);
	}
	xmlwriter.Write_attribute_string(attr, val);

	attr = "Chip_IDs";
	val = "";
	for (int i = 0; i < Chip_No; i++) {
		if (i > 0) {
			val += ",";
		}
		val += std::to_string(Chip_IDs[i]);
	}
	xmlwriter.Write_attribute_string(attr, val);

	attr = "Die_IDs";
	val = "";
	for (int i = 0; i < Die_No; i++) {
		if (i > 0) {
			val += ",";
		}
		val += std::to_string(Die_IDs[i]);
	}
	xmlwriter.Write_attribute_string(attr, val);

	attr = "Plane_IDs";
	val = "";
	for (int i = 0; i < Plane_No; i++) {
		if (i > 0) {
			val += ",";
		}
		val += std::to_string(Plane_IDs[i]);
	}
	xmlwriter.Write_attribute_string(attr, val);

	attr = "Initial_Occupancy_Percentage";
	val = std::to_string(Initial_Occupancy_Percentage);
	xmlwriter.Write_attribute_string(attr, val);
}

void IO_Flow_Parameter_Set::XML_deserialize(rapidxml::xml_node<> *node)
{
	try {
		for (auto param = node->first_node(); param; param = param->next_sibling()) {
			if (strcmp(param->name(), "Device_Level_Data_Caching_Mode") == 0) {
				std::string val = param->value();
				std::transform(val.begin(), val.end(), val.begin(), ::toupper);
				if (strcmp(val.c_str(), "TURNED_OFF") == 0) {
					Device_Level_Data_Caching_Mode = SSD_Components::Caching_Mode::TURNED_OFF;
				} else if (strcmp(val.c_str(), "WRITE_CACHE") == 0) {
					Device_Level_Data_Caching_Mode = SSD_Components::Caching_Mode::WRITE_CACHE;
				} else if (strcmp(val.c_str(), "READ_CACHE") == 0) {
					Device_Level_Data_Caching_Mode = SSD_Components::Caching_Mode::READ_CACHE;
				} else if (strcmp(val.c_str(), "WRITE_READ_CACHE") == 0) {
					Device_Level_Data_Caching_Mode = SSD_Components::Caching_Mode::WRITE_READ_CACHE;
				} else {
					PRINT_ERROR("Wrong caching mode definition for input flow")
				}
			}
			else if (strcmp(param->name(), "Priority_Class") == 0)
			{
				std::string val = param->value();
				std::transform(val.begin(), val.end(), val.begin(), ::toupper);
				if (strcmp(val.c_str(), "URGENT") == 0) {
					Priority_Class = IO_Flow_Priority_Class::URGENT;
				} else if (strcmp(val.c_str(), "HIGH") == 0) {
					Priority_Class = IO_Flow_Priority_Class::HIGH;
				} else if (strcmp(val.c_str(), "MEDIUM") == 0) {
					Priority_Class = IO_Flow_Priority_Class::MEDIUM;
				} else if (strcmp(val.c_str(), "LOW") == 0) {
					Priority_Class = IO_Flow_Priority_Class::LOW;
				} else {
					PRINT_ERROR("Wrong priority class definition for input flow")
				}
			} else if (strcmp(param->name(), "Channel_IDs") == 0) {
				std::set<int> ids;
				char tmp[1000], *tmp2;
				strncpy(tmp, param->value(), 1000);
				std::string id = strtok(tmp, ",");
				while (1) {
					std::string::size_type sz;
					ids.insert(std::stoi(id, &sz));
					tmp2 = strtok(NULL, ",");
					if (tmp2 == NULL) {
						break;
					} else {
						id = tmp2;
					}
				}
				Channel_No = (int)ids.size();
				Channel_IDs = new flash_block_ID_type[Channel_No];
				int i = 0;
				for (auto it = ids.begin(); it != ids.end(); it++) {
					Channel_IDs[i++] = *it;
				}
			} else if (strcmp(param->name(), "Chip_IDs") == 0) {
				std::set<int> ids;
				char tmp[1000], *tmp2;
				strncpy(tmp, param->value(), 1000);
				std::string id = strtok(tmp, ",");
				while (1) {
					std::string::size_type sz;
					ids.insert(std::stoi(id, &sz));
					tmp2 = strtok(NULL, ",");
					if (tmp2 == NULL) {
						break;
					} else {
						id = tmp2;
					}
				}
				Chip_No = (int)ids.size();
				Chip_IDs = new flash_block_ID_type[Chip_No];
				int i = 0;
				for (auto it = ids.begin(); it != ids.end(); it++) {
					Chip_IDs[i++] = *it;
				}
			} else if (strcmp(param->name(), "Die_IDs") == 0) {
				std::set<int> ids;
				char tmp[1000], *tmp2;
				strncpy(tmp, param->value(), 1000);
				std::string id = strtok(tmp, ",");
				while (1) {
					std::string::size_type sz;
					ids.insert(std::stoi(id, &sz));
					tmp2 = strtok(NULL, ",");
					if (tmp2 == NULL) {
						break;
					} else {
						id = tmp2;
					}
				}
				Die_No = (int)ids.size();
				Die_IDs = new flash_block_ID_type[Die_No];
				int i = 0;
				for (auto it = ids.begin(); it != ids.end(); it++) {
					Die_IDs[i++] = *it;
				}
			} else if (strcmp(param->name(), "Plane_IDs") == 0) {
				std::set<int> ids;
				char tmp[1000], *tmp2;
				strncpy(tmp, param->value(), 1000);
				std::string id = strtok(tmp, ",");
				while (1) {
					std::string::size_type sz;
					ids.insert(std::stoi(id, &sz));
					tmp2 = strtok(NULL, ",");
					if (tmp2 == NULL) {
						break;
					} else {
						id = tmp2;
					}
				}
				Plane_No = (int)ids.size();
				Plane_IDs = new flash_block_ID_type[Plane_No];
				int i = 0;
				for (auto it = ids.begin(); it != ids.end(); it++) {
					Plane_IDs[i++] = *it;
				}
			} else if (strcmp(param->name(), "Initial_Occupancy_Percentage") == 0) {
				std::string val = param->value();
				Initial_Occupancy_Percentage = std::stoul(val);
			}
		}
	} catch (...) {
		PRINT_ERROR("Error in IO_Flow_Parameter_Set!")
	}
}

void IO_Flow_Parameter_Set_Synthetic::XML_serialize(Utils::XmlWriter& xmlwriter)
{
	std::string tmp;
	tmp = "IO_Flow_Parameter_Set_Synthetic";
	xmlwriter.Write_open_tag(tmp);
	IO_Flow_Parameter_Set::XML_serialize(xmlwriter);

	std::string attr = "Working_Set_Percentage";
	std::string val = std::to_string(Working_Set_Percentage);
	xmlwriter.Write_attribute_string(attr, val);

	attr = "Synthetic_Generator_Type";
	switch (Synthetic_Generator_Type) {
		case Utils::Request_Generator_Type::BANDWIDTH:
			val = "BANDWIDTH";
			break;
		case Utils::Request_Generator_Type::QUEUE_DEPTH:
			val = "QUEUE_DEPTH";
			break;
	}
	xmlwriter.Write_attribute_string(attr, val);

	attr = "Read_Percentage";
	val = std::to_string(Read_Percentage);
	xmlwriter.Write_attribute_string(attr, val);


	attr = "Address_Distribution";
	switch (Address_Distribution) {
		case Utils::Address_Distribution_Type::STREAMING:
			val = "STREAMING";
			break;
		case Utils::Address_Distribution_Type::RANDOM_HOTCOLD:
			val = "RANDOM_HOTCOLD";
			break;
		case Utils::Address_Distribution_Type::RANDOM_UNIFORM:
			val = "RANDOM_UNIFORM";
			break;
	}
	xmlwriter.Write_attribute_string(attr, val);
	 

	attr = "Percentage_of_Hot_Region";
	val = std::to_string(Percentage_of_Hot_Region);
	xmlwriter.Write_attribute_string(attr, val);

	attr = "Generated_Aligned_Addresses";
	val = (Generated_Aligned_Addresses ? "true" : "false");
	xmlwriter.Write_attribute_string(attr, val);

	attr = "Address_Alignment_Unit";
	val = std::to_string(Address_Alignment_Unit);
	xmlwriter.Write_attribute_string(attr, val);
	
	attr = "Request_Size_Distribution";
	switch (Request_Size_Distribution) {
		case Utils::Request_Size_Distribution_Type::FIXED:
			val = "FIXED";
			break;
		case Utils::Request_Size_Distribution_Type::NORMAL:
			val = "NORMAL";
			break;
	}
	xmlwriter.Write_attribute_string(attr, val);

	attr = "Average_Request_Size";
	val = std::to_string(Average_Request_Size);
	xmlwriter.Write_attribute_string(attr, val);


	attr = "Variance_Request_Size";
	val = std::to_string(Variance_Request_Size);
	xmlwriter.Write_attribute_string(attr, val);


	attr = "Seed";
	val = std::to_string(Seed);
	xmlwriter.Write_attribute_string(attr, val);


	attr = "Average_No_of_Reqs_in_Queue";
	val = std::to_string(Average_No_of_Reqs_in_Queue);
	xmlwriter.Write_attribute_string(attr, val);


	attr = "Bandwidth";
	val = std::to_string(Bandwidth);
	xmlwriter.Write_attribute_string(attr, val);


	attr = "Stop_Time";
	val = std::to_string(Stop_Time);
	xmlwriter.Write_attribute_string(attr, val);


	attr = "Total_Requests_To_Generate";
	val = std::to_string(Total_Requests_To_Generate);
	xmlwriter.Write_attribute_string(attr, val);

	xmlwriter.Write_close_tag();
}

void IO_Flow_Parameter_Set_Synthetic::XML_deserialize(rapidxml::xml_node<> *node)
{
	IO_Flow_Parameter_Set::XML_deserialize(node);
	try {
		for (auto param = node->first_node(); param; param = param->next_sibling()) {
			if (strcmp(param->name(), "Working_Set_Percentage") == 0) {
				std::string val = param->value();
				Working_Set_Percentage = std::stoi(val);
			} else if (strcmp(param->name(), "Synthetic_Generator_Type") == 0) {
				std::string val = param->value();
				std::transform(val.begin(), val.end(), val.begin(), ::toupper);
				if (strcmp(val.c_str(), "BANDWIDTH") == 0) {
					Synthetic_Generator_Type = Utils::Request_Generator_Type::BANDWIDTH;
				} else if (strcmp(val.c_str(), "QUEUE_DEPTH") == 0) {
					Synthetic_Generator_Type = Utils::Request_Generator_Type::QUEUE_DEPTH;
				} else {
					PRINT_ERROR("Unknown synthetic generator type specified in the input file")
				}
			} else if (strcmp(param->name(), "Read_Percentage") == 0) {
				std::string val = param->value();
				Read_Percentage = std::stoi(val);
			} else if (strcmp(param->name(), "Address_Distribution") == 0) {
				std::string val = param->value();
				std::transform(val.begin(), val.end(), val.begin(), ::toupper);
				if (strcmp(val.c_str(), "STREAMING") == 0) {
					Address_Distribution = Utils::Address_Distribution_Type::STREAMING;
				} else if (strcmp(val.c_str(), "RANDOM_HOTCOLD") == 0) {
					Address_Distribution = Utils::Address_Distribution_Type::RANDOM_HOTCOLD;
				} else if (strcmp(val.c_str(), "RANDOM_UNIFORM") == 0) {
					Address_Distribution = Utils::Address_Distribution_Type::RANDOM_UNIFORM;
				} else {
					PRINT_ERROR("Wrong address distribution type for input synthetic flow")
				}
			} else if (strcmp(param->name(), "Percentage_of_Hot_Region") == 0) {
				std::string val = param->value();
				Percentage_of_Hot_Region = std::stoi(val);
			} else if (strcmp(param->name(), "Generated_Aligned_Addresses") == 0) {
				std::string val = param->value();
				std::transform(val.begin(), val.end(), val.begin(), ::toupper);
				Generated_Aligned_Addresses = (val.compare("FALSE") == 0 ? false : true);
			} else if (strcmp(param->name(), "Address_Alignment_Unit") == 0) {
				std::string val = param->value();
				Address_Alignment_Unit = std::stoi(val);
			} else if (strcmp(param->name(), "Request_Size_Distribution") == 0) {
				std::string val = param->value();
				std::transform(val.begin(), val.end(), val.begin(), ::toupper);
				if (strcmp(val.c_str(), "FIXED") == 0) {
					Request_Size_Distribution = Utils::Request_Size_Distribution_Type::FIXED;
				} else if (strcmp(val.c_str(), "NORMAL") == 0) {
					Request_Size_Distribution = Utils::Request_Size_Distribution_Type::NORMAL;
				} else {
					PRINT_ERROR("Wrong request size distribution type for input synthetic flow")
				}
			} else if (strcmp(param->name(), "Average_Request_Size") == 0) {
				std::string val = param->value();
				Average_Request_Size = std::stoi(val);
			} else if (strcmp(param->name(), "Variance_Request_Size") == 0) {
				std::string val = param->value();
				Variance_Request_Size = std::stoi(val);
			} else if (strcmp(param->name(), "Seed") == 0) {
				std::string val = param->value();
				Seed = std::stoi(val);
			} else if (strcmp(param->name(), "Average_No_of_Reqs_in_Queue") == 0) {
				std::string val = param->value();
				Average_No_of_Reqs_in_Queue = std::stoi(val);
			} else if (strcmp(param->name(), "Bandwidth") == 0) {
				std::string val = param->value();
				Bandwidth = std::stoi(val);
			} else if (strcmp(param->name(), "Stop_Time") == 0) {
				std::string val = param->value();
				Stop_Time = std::stoll(val);
			} else if (strcmp(param->name(), "Total_Requests_To_Generate") == 0) {
				std::string val = param->value();
				Total_Requests_To_Generate = std::stoi(val);
			}
		}
	} catch (...) {
		PRINT_ERROR("Error in IO_Flow_Parameter_Set_Synthetic!")
	}
}

void IO_Flow_Parameter_Set_Trace_Based::XML_serialize(Utils::XmlWriter& xmlwriter)
{

	std::string tmp = "IO_Flow_Parameter_Set_Trace_Based";
	xmlwriter.Write_open_tag(tmp);
	IO_Flow_Parameter_Set::XML_serialize(xmlwriter);

	std::string attr = "File_Path";
	std::string val = File_Path;
	xmlwriter.Write_attribute_string(attr, val);

	attr = "Percentage_To_Be_Executed";
	val = std::to_string(Percentage_To_Be_Executed);
	xmlwriter.Write_attribute_string(attr, val);

	attr = "Relay_Count";
	val = std::to_string(Relay_Count);
	xmlwriter.Write_attribute_string(attr, val);

	attr = "Time_Unit";
	switch (Time_Unit) {
		case Trace_Time_Unit::PICOSECOND:
			val = "PICOSECOND";
			break;
		case Trace_Time_Unit::NANOSECOND:
			val = "NANOSECOND";
			break;
		case Trace_Time_Unit::MICROSECOND:
			val = "MICROSECOND";
			break;
	}
	xmlwriter.Write_attribute_string(attr, val);

	xmlwriter.Write_close_tag();
}

void IO_Flow_Parameter_Set_Trace_Based::XML_deserialize(rapidxml::xml_node<> *node)
{
	IO_Flow_Parameter_Set::XML_deserialize(node);

	try {
		for (auto param = node->first_node(); param; param = param->next_sibling()) {
			if (strcmp(param->name(), "Relay_Count") == 0) {
				std::string val = param->value();
				Relay_Count = std::stoi(val);
			} else if (strcmp(param->name(), "Percentage_To_Be_Executed") == 0) {
				std::string val = param->value();
				Percentage_To_Be_Executed = std::stoi(val);
			} else if (strcmp(param->name(), "File_Path") == 0) {
				File_Path = param->value();
			} else if (strcmp(param->name(), "Time_Unit") == 0) {
				std::string val = param->value();
				std::transform(val.begin(), val.end(), val.begin(), ::toupper);
				if (strcmp(val.c_str(), "PICOSECOND") == 0) {
					Time_Unit = Trace_Time_Unit::PICOSECOND;
				} else if (strcmp(val.c_str(), "NANOSECOND") == 0) {
					Time_Unit = Trace_Time_Unit::NANOSECOND;
				} else if (strcmp(val.c_str(), "MICROSECOND") == 0) {
					Time_Unit = Trace_Time_Unit::MICROSECOND;
				} else {
					PRINT_ERROR("Wrong time unit specified for the trace based flow")
				}
			}

		}
	} catch (...) {
		PRINT_ERROR("Error in IO_Flow_Parameter_Set_Trace_Based!")
	}
}