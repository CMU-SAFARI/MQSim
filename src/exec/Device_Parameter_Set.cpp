#include "Device_Parameter_Set.h"
#include <algorithm>



int Device_Parameter_Set::Seed = 123;//Seed for random number generation (used in device's random number generators)
bool Device_Parameter_Set::Enabled_Preconditioning = true;
NVM::NVM_Type Device_Parameter_Set::Memory_Type = NVM::NVM_Type::FLASH;
HostInterface_Types Device_Parameter_Set::HostInterface_Type = HostInterface_Types::NVME;
uint16_t Device_Parameter_Set::IO_Queue_Depth = 1024;//For NVMe, it determines the size of the submission/completion queues; for SATA, it determines the size of NCQ_Control_Structure
uint16_t Device_Parameter_Set::Queue_Fetch_Size = 512;//Used in NVMe host interface
SSD_Components::Caching_Mechanism Device_Parameter_Set::Caching_Mechanism = SSD_Components::Caching_Mechanism::ADVANCED;
SSD_Components::Cache_Sharing_Mode Device_Parameter_Set::Data_Cache_Sharing_Mode = SSD_Components::Cache_Sharing_Mode::SHARED;//Data cache sharing among concurrently running I/O flows, if NVMe host interface is used
unsigned int Device_Parameter_Set::Data_Cache_Capacity = 1024 * 1024 * 512;//Data cache capacity in bytes
unsigned int Device_Parameter_Set::Data_Cache_DRAM_Row_Size = 8192;//The row size of DRAM in the data cache, the unit is bytes
unsigned int Device_Parameter_Set::Data_Cache_DRAM_Data_Rate = 800;//Data access rate to access DRAM in the data cache, the unit is MT/s
unsigned int Device_Parameter_Set::Data_Cache_DRAM_Data_Busrt_Size = 4;//The number of bytes that are transferred in one burst (it depends on the number of DRAM chips)
sim_time_type Device_Parameter_Set::Data_Cache_DRAM_tRCD = 13;//tRCD parameter to access DRAM in the data cache, the unit is nano-seconds
sim_time_type Device_Parameter_Set::Data_Cache_DRAM_tCL = 13;//tCL parameter to access DRAM in the data cache, the unit is nano-seconds
sim_time_type Device_Parameter_Set::Data_Cache_DRAM_tRP = 13;//tRP parameter to access DRAM in the data cache, the unit is nano-seconds
SSD_Components::Flash_Address_Mapping_Type Device_Parameter_Set::Address_Mapping = SSD_Components::Flash_Address_Mapping_Type::PAGE_LEVEL;
bool Device_Parameter_Set::Ideal_Mapping_Table = false;//If mapping is ideal, then all the mapping entries are found in the DRAM and there is no need to read mapping entries from flash
unsigned int Device_Parameter_Set::CMT_Capacity = 2 * 1024 * 1024;//Size of SRAM/DRAM space that is used to cache address mapping table in bytes
SSD_Components::CMT_Sharing_Mode Device_Parameter_Set::CMT_Sharing_Mode = SSD_Components::CMT_Sharing_Mode::SHARED;//How the entire CMT space is shared among concurrently running flows
SSD_Components::Flash_Plane_Allocation_Scheme_Type Device_Parameter_Set::Plane_Allocation_Scheme = SSD_Components::Flash_Plane_Allocation_Scheme_Type::CWDP;
SSD_Components::Flash_Scheduling_Type Device_Parameter_Set::Transaction_Scheduling_Policy = SSD_Components::Flash_Scheduling_Type::OUT_OF_ORDER;
double Device_Parameter_Set::Overprovisioning_Ratio = 0.07;//The ratio of spare space with respect to the whole available storage space of SSD
double Device_Parameter_Set::GC_Exec_Threshold = 0.05;//The threshold for the ratio of free pages that used to trigger GC
SSD_Components::GC_Block_Selection_Policy_Type Device_Parameter_Set::GC_Block_Selection_Policy = SSD_Components::GC_Block_Selection_Policy_Type::RGA;
bool Device_Parameter_Set::Use_Copyback_for_GC = false;
bool Device_Parameter_Set::Preemptible_GC_Enabled = true;
double Device_Parameter_Set::GC_Hard_Threshold = 0.005;//The hard gc execution threshold, used to stop preemptible gc execution
bool Device_Parameter_Set::Dynamic_Wearleveling_Enabled = true;
bool Device_Parameter_Set::Static_Wearleveling_Enabled = true;
unsigned int Device_Parameter_Set::Static_Wearleveling_Threshold = 100;
sim_time_type Device_Parameter_Set::Preferred_suspend_erase_time_for_read = 700000;//in nano-seconds
sim_time_type Device_Parameter_Set::Preferred_suspend_erase_time_for_write = 700000;//in nano-seconds
sim_time_type Device_Parameter_Set::Preferred_suspend_write_time_for_read = 100000;//in nano-seconds
unsigned int Device_Parameter_Set::Flash_Channel_Count = 8;
unsigned int Device_Parameter_Set::Flash_Channel_Width = 1;//Channel width in byte
unsigned int Device_Parameter_Set::Channel_Transfer_Rate = 300;//MT/s
unsigned int Device_Parameter_Set::Chip_No_Per_Channel = 4;
SSD_Components::ONFI_Protocol Device_Parameter_Set::Flash_Comm_Protocol = SSD_Components::ONFI_Protocol::NVDDR2;
Flash_Parameter_Set Device_Parameter_Set::Flash_Parameters;

void Device_Parameter_Set::XML_serialize(Utils::XmlWriter& xmlwriter)
{
	std::string tmp;
	tmp = "Device_Parameter_Set";
	xmlwriter.Write_open_tag(tmp);

	std::string attr = "Seed";
	std::string val = std::to_string(Seed);
	xmlwriter.Write_attribute_string(attr, val);

	attr = "Enabled_Preconditioning";
	val = (Enabled_Preconditioning ? "true" : "false");
	xmlwriter.Write_attribute_string(attr, val);

	attr = "Memory_Type";
	val;
	switch (Memory_Type) {
		case NVM::NVM_Type::FLASH:
			val = "FLASH";
			break;
		default:
			break;
	}
	xmlwriter.Write_attribute_string(attr, val);

	attr = "HostInterface_Type";
	val;
	switch (HostInterface_Type) {
		case HostInterface_Types::NVME:
			val = "NVME";
			break;
		case HostInterface_Types::SATA:
			val = "SATA";
			break;
		default:
			break;
	}
	xmlwriter.Write_attribute_string(attr, val);

	attr = "IO_Queue_Depth";
	val = std::to_string(IO_Queue_Depth);
	xmlwriter.Write_attribute_string(attr, val);

	attr = "Queue_Fetch_Size";
	val = std::to_string(Queue_Fetch_Size);
	xmlwriter.Write_attribute_string(attr, val);


	attr = "Caching_Mechanism";
	switch (Caching_Mechanism) {
		case SSD_Components::Caching_Mechanism::SIMPLE:
			val = "SIMPLE";
			break;
		case SSD_Components::Caching_Mechanism::ADVANCED:
			val = "ADVANCED";
			break;
		default:
			break;
	}
	xmlwriter.Write_attribute_string(attr, val);

	attr = "Data_Cache_Sharing_Mode";
	switch (Data_Cache_Sharing_Mode) {
		case SSD_Components::Cache_Sharing_Mode::SHARED:
			val = "SHARED";
			break;
		case SSD_Components::Cache_Sharing_Mode::EQUAL_PARTITIONING:
			val = "EQUAL_PARTITIONING";
			break;
		default:
			break;
	}
	xmlwriter.Write_attribute_string(attr, val);

	attr = "Data_Cache_Capacity";
	val = std::to_string(Data_Cache_Capacity);
	xmlwriter.Write_attribute_string(attr, val);

	attr = "Data_Cache_DRAM_Row_Size";
	val = std::to_string(Data_Cache_DRAM_Row_Size);
	xmlwriter.Write_attribute_string(attr, val);

	attr = "Data_Cache_DRAM_Data_Rate";
	val = std::to_string(Data_Cache_DRAM_Data_Rate);
	xmlwriter.Write_attribute_string(attr, val);

	attr = "Data_Cache_DRAM_Data_Busrt_Size";
	val = std::to_string(Data_Cache_DRAM_Data_Busrt_Size);
	xmlwriter.Write_attribute_string(attr, val);

	attr = "Data_Cache_DRAM_tRCD";
	val = std::to_string(Data_Cache_DRAM_tRCD);
	xmlwriter.Write_attribute_string(attr, val);

	attr = "Data_Cache_DRAM_tCL";
	val = std::to_string(Data_Cache_DRAM_tCL);
	xmlwriter.Write_attribute_string(attr, val);

	attr = "Data_Cache_DRAM_tRP";
	val = std::to_string(Data_Cache_DRAM_tRP);
	xmlwriter.Write_attribute_string(attr, val);

	attr = "Address_Mapping";
	switch (Address_Mapping) {
		case SSD_Components::Flash_Address_Mapping_Type::PAGE_LEVEL:
			val = "PAGE_LEVEL";
			break;
		case SSD_Components::Flash_Address_Mapping_Type::HYBRID:
			val = "HYBRID";
			break;
		default:
			break;
	}
	xmlwriter.Write_attribute_string(attr, val);

	attr = "Ideal_Mapping_Table";
	val = (Use_Copyback_for_GC ? "true" : "false");
	xmlwriter.Write_attribute_string(attr, val);
	
	attr = "CMT_Capacity";
	val = std::to_string(CMT_Capacity);
	xmlwriter.Write_attribute_string(attr, val);

	attr = "CMT_Sharing_Mode";
	switch (CMT_Sharing_Mode) {
		case SSD_Components::CMT_Sharing_Mode::SHARED:
			val = "SHARED";
			break;
		case SSD_Components::CMT_Sharing_Mode::EQUAL_SIZE_PARTITIONING:
			val = "EQUAL_SIZE_PARTITIONING";
			break;
		default:
			break;
	}
	xmlwriter.Write_attribute_string(attr, val);

	attr = "Plane_Allocation_Scheme";
	switch (Plane_Allocation_Scheme) {
		case SSD_Components::Flash_Plane_Allocation_Scheme_Type::CDPW:
			val = "CDPW";
			break;
		case SSD_Components::Flash_Plane_Allocation_Scheme_Type::CDWP:
			val = "CDWP";
			break;
		case SSD_Components::Flash_Plane_Allocation_Scheme_Type::CPDW:
			val = "CPDW";
			break;
		case SSD_Components::Flash_Plane_Allocation_Scheme_Type::CPWD:
			val = "CPWD";
			break;
		case SSD_Components::Flash_Plane_Allocation_Scheme_Type::CWDP:
			val = "CWDP";
			break;
		case SSD_Components::Flash_Plane_Allocation_Scheme_Type::CWPD:
			val = "CWPD";
			break;
		case SSD_Components::Flash_Plane_Allocation_Scheme_Type::DCPW:
			val = "DCPW";
			break;
		case SSD_Components::Flash_Plane_Allocation_Scheme_Type::DCWP:
			val = "DCWP";
			break;
		case SSD_Components::Flash_Plane_Allocation_Scheme_Type::DPCW:
			val = "DPCW";
			break;
		case SSD_Components::Flash_Plane_Allocation_Scheme_Type::DPWC:
			val = "DPWC";
			break;
		case SSD_Components::Flash_Plane_Allocation_Scheme_Type::DWCP:
			val = "DWCP";
			break;
		case SSD_Components::Flash_Plane_Allocation_Scheme_Type::DWPC:
			val = "DWPC";
			break;
		case SSD_Components::Flash_Plane_Allocation_Scheme_Type::PCDW:
			val = "PCDW";
			break;
		case SSD_Components::Flash_Plane_Allocation_Scheme_Type::PCWD:
			val = "PCWD";
			break;
		case SSD_Components::Flash_Plane_Allocation_Scheme_Type::PDCW:
			val = "PDCW";
			break;
		case SSD_Components::Flash_Plane_Allocation_Scheme_Type::PDWC:
			val = "PDWC";
			break;
		case SSD_Components::Flash_Plane_Allocation_Scheme_Type::PWCD:
			val = "PWCD";
			break;
		case SSD_Components::Flash_Plane_Allocation_Scheme_Type::PWDC:
			val = "PWDC";
			break;
		case SSD_Components::Flash_Plane_Allocation_Scheme_Type::WCDP:
			val = "WCDP";
			break;
		case SSD_Components::Flash_Plane_Allocation_Scheme_Type::WCPD:
			val = "WCPD";
			break;
		case SSD_Components::Flash_Plane_Allocation_Scheme_Type::WDCP:
			val = "WDCP";
			break;
		case SSD_Components::Flash_Plane_Allocation_Scheme_Type::WDPC:
			val = "WDPC";
			break;
		case SSD_Components::Flash_Plane_Allocation_Scheme_Type::WPCD:
			val = "WPCD";
			break;
		case SSD_Components::Flash_Plane_Allocation_Scheme_Type::WPDC:
			val = "WPDC";
			break;
		default:
			break;
	}
	xmlwriter.Write_attribute_string(attr, val);

	attr = "Transaction_Scheduling_Policy";
	switch (Transaction_Scheduling_Policy) {
		case SSD_Components::Flash_Scheduling_Type::OUT_OF_ORDER:
			val = "OUT_OF_ORDER";
			break;
		case SSD_Components::Flash_Scheduling_Type::PRIORITY_OUT_OF_ORDER:
			val = "PRIORITY_OUT_OF_ORDER";
			break;
		case SSD_Components::Flash_Scheduling_Type::FLIN:
			val = "FLIN";
			break;
		default:
			break;
	}
	xmlwriter.Write_attribute_string(attr, val);

	attr = "Overprovisioning_Ratio";
	val = std::to_string(Overprovisioning_Ratio);
	xmlwriter.Write_attribute_string(attr, val);

	attr = "GC_Exec_Threshold";
	val = std::to_string(GC_Exec_Threshold);
	xmlwriter.Write_attribute_string(attr, val);

	attr = "GC_Block_Selection_Policy";
	switch (GC_Block_Selection_Policy) {
		case SSD_Components::GC_Block_Selection_Policy_Type::GREEDY:
			val = "GREEDY";
			break;
		case SSD_Components::GC_Block_Selection_Policy_Type::RGA:
			val = "RGA";
			break;
		case SSD_Components::GC_Block_Selection_Policy_Type::RANDOM:
			val = "RANDOM";
			break;
		case SSD_Components::GC_Block_Selection_Policy_Type::RANDOM_P:
			val = "RANDOM_P";
			break;
		case SSD_Components::GC_Block_Selection_Policy_Type::RANDOM_PP:
			val = "RANDOM_PP";
			break;
		case SSD_Components::GC_Block_Selection_Policy_Type::FIFO:
			val = "FIFO";
			break;
		default:
			break;
	}
	xmlwriter.Write_attribute_string(attr, val);
	
	attr = "Use_Copyback_for_GC";
	val = (Use_Copyback_for_GC ? "true" : "false");
	xmlwriter.Write_attribute_string(attr, val);

	attr = "Preemptible_GC_Enabled";
	val = (Preemptible_GC_Enabled ? "true" : "false");
	xmlwriter.Write_attribute_string(attr, val);

	attr = "GC_Hard_Threshold";
	val = std::to_string(GC_Hard_Threshold);
	xmlwriter.Write_attribute_string(attr, val);

	attr = "Dynamic_Wearleveling_Enabled";
	val = (Dynamic_Wearleveling_Enabled ? "true" : "false");
	xmlwriter.Write_attribute_string(attr, val);

	attr = "Static_Wearleveling_Enabled";
	val = (Static_Wearleveling_Enabled ? "true" : "false");
	xmlwriter.Write_attribute_string(attr, val);
	
	attr = "Static_Wearleveling_Threshold";
	val = std::to_string(Static_Wearleveling_Threshold);
	xmlwriter.Write_attribute_string(attr, val);
	
	attr = "Preferred_suspend_erase_time_for_read";
	val = std::to_string(Preferred_suspend_erase_time_for_read);
	xmlwriter.Write_attribute_string(attr, val);

	attr = "Preferred_suspend_erase_time_for_write";
	val = std::to_string(Preferred_suspend_erase_time_for_write);
	xmlwriter.Write_attribute_string(attr, val);

	attr = "Preferred_suspend_write_time_for_read";
	val = std::to_string(Preferred_suspend_write_time_for_read);
	xmlwriter.Write_attribute_string(attr, val);

	attr = "Flash_Channel_Count";
	val = std::to_string(Flash_Channel_Count);
	xmlwriter.Write_attribute_string(attr, val);

	attr = "Flash_Channel_Width";
	val = std::to_string(Flash_Channel_Width);
	xmlwriter.Write_attribute_string(attr, val);

	attr = "Channel_Transfer_Rate";
	val = std::to_string(Channel_Transfer_Rate);
	xmlwriter.Write_attribute_string(attr, val);

	attr = "Chip_No_Per_Channel";
	val = std::to_string(Chip_No_Per_Channel);
	xmlwriter.Write_attribute_string(attr, val);

	attr = "Flash_Comm_Protocol";
	switch (Flash_Comm_Protocol) {
		case SSD_Components::ONFI_Protocol::NVDDR2:
			val = "NVDDR2";
			break;
		default:
			break;
	}
	xmlwriter.Write_attribute_string(attr, val);

	Flash_Parameters.XML_serialize(xmlwriter);

	xmlwriter.Write_close_tag();
}

void Device_Parameter_Set::XML_deserialize(rapidxml::xml_node<> *node)
{
	try
	{
		for (auto param = node->first_node(); param; param = param->next_sibling()) {
			if (strcmp(param->name(), "Seed") == 0) {
				std::string val = param->value();
				Seed = std::stoi(val);
			} else if (strcmp(param->name(), "Enabled_Preconditioning") == 0) {
				std::string val = param->value();
				std::transform(val.begin(), val.end(), val.begin(), ::toupper);
				Enabled_Preconditioning = (val.compare("FALSE") == 0 ? false : true);
			} else if (strcmp(param->name(), "Memory_Type") == 0) {
				std::string val = param->value();
				std::transform(val.begin(), val.end(), val.begin(), ::toupper);
				if (strcmp(val.c_str(), "FLASH") == 0)
					Memory_Type = NVM::NVM_Type::FLASH;
				else PRINT_ERROR("Unknown NVM type specified in the SSD configuration file")
			} else if (strcmp(param->name(), "HostInterface_Type") == 0) {
				std::string val = param->value();
				std::transform(val.begin(), val.end(), val.begin(), ::toupper);
				if (strcmp(val.c_str(), "NVME") == 0) {
					HostInterface_Type = HostInterface_Types::NVME;
				} else if (strcmp(val.c_str(), "SATA") == 0) {
					HostInterface_Type = HostInterface_Types::SATA;
				} else {
					PRINT_ERROR("Unknown host interface type specified in the SSD configuration file")
				}
			} else if (strcmp(param->name(), "IO_Queue_Depth") == 0) {
				std::string val = param->value();
				IO_Queue_Depth = (uint16_t) std::stoull(val);
			} else if (strcmp(param->name(), "Queue_Fetch_Size") == 0) {
				std::string val = param->value();
				Queue_Fetch_Size = (uint16_t) std::stoull(val);
			} else if (strcmp(param->name(), "Caching_Mechanism") == 0) {
				std::string val = param->value();
				std::transform(val.begin(), val.end(), val.begin(), ::toupper);
				if (strcmp(val.c_str(), "SIMPLE") == 0) {
					Caching_Mechanism = SSD_Components::Caching_Mechanism::SIMPLE;
				} else if (strcmp(val.c_str(), "ADVANCED") == 0) {
					Caching_Mechanism = SSD_Components::Caching_Mechanism::ADVANCED;
				} else {
					PRINT_ERROR("Unknown data caching mechanism specified in the SSD configuration file")
				}
			} else if (strcmp(param->name(), "Data_Cache_Sharing_Mode") == 0) {
				std::string val = param->value();
				std::transform(val.begin(), val.end(), val.begin(), ::toupper);
				if (strcmp(val.c_str(), "SHARED") == 0) {
					Data_Cache_Sharing_Mode = SSD_Components::Cache_Sharing_Mode::SHARED;
				} else if (strcmp(val.c_str(), "EQUAL_PARTITIONING") == 0) {
					Data_Cache_Sharing_Mode = SSD_Components::Cache_Sharing_Mode::EQUAL_PARTITIONING;
				} else {
					PRINT_ERROR("Unknown data cache sharing mode specified in the SSD configuration file")
				}
			} else if (strcmp(param->name(), "Data_Cache_Capacity") == 0) {
				std::string val = param->value();
				Data_Cache_Capacity = std::stoul(val);
			} else if (strcmp(param->name(), "Data_Cache_DRAM_Row_Size") == 0) {
				std::string val = param->value();
				Data_Cache_DRAM_Row_Size = std::stoul(val);
			} else if (strcmp(param->name(), "Data_Cache_DRAM_Data_Rate") == 0) {
				std::string val = param->value();
				Data_Cache_DRAM_Data_Rate = std::stoul(val);
			} else if (strcmp(param->name(), "Data_Cache_DRAM_Data_Busrt_Size") == 0) {
				std::string val = param->value();
				Data_Cache_DRAM_Data_Busrt_Size = std::stoul(val);
			} else if (strcmp(param->name(), "Data_Cache_DRAM_tRCD") == 0) {
				std::string val = param->value();
				Data_Cache_DRAM_tRCD = std::stoul(val);
			} else if (strcmp(param->name(), "Data_Cache_DRAM_tCL") == 0) {
				std::string val = param->value();
				Data_Cache_DRAM_tCL = std::stoul(val);
			} else if (strcmp(param->name(), "Data_Cache_DRAM_tRP") == 0) {
				std::string val = param->value();
				Data_Cache_DRAM_tRP = std::stoul(val);
			} else if (strcmp(param->name(), "Address_Mapping") == 0) {
				std::string val = param->value();
				std::transform(val.begin(), val.end(), val.begin(), ::toupper);
				if (strcmp(val.c_str(), "PAGE_LEVEL") == 0) {
					Address_Mapping = SSD_Components::Flash_Address_Mapping_Type::PAGE_LEVEL;
				} else if (strcmp(val.c_str(), "HYBRID") == 0) {
					Address_Mapping = SSD_Components::Flash_Address_Mapping_Type::HYBRID;
				} else {
					PRINT_ERROR("Unknown address mapping type specified in the SSD configuration file")
				}
			}
			else if (strcmp(param->name(), "Ideal_Mapping_Table") == 0) {
				std::string val = param->value();
				std::transform(val.begin(), val.end(), val.begin(), ::toupper);
				Ideal_Mapping_Table = (val.compare("FALSE") == 0 ? false : true);
			} else if (strcmp(param->name(), "CMT_Capacity") == 0) {
				std::string val = param->value();
				CMT_Capacity = std::stoul(val);
			} else if (strcmp(param->name(), "CMT_Sharing_Mode") == 0) {
				std::string val = param->value();
				std::transform(val.begin(), val.end(), val.begin(), ::toupper);
				if (strcmp(val.c_str(), "SHARED") == 0) {
					CMT_Sharing_Mode = SSD_Components::CMT_Sharing_Mode::SHARED;
				} else if (strcmp(val.c_str(), "EQUAL_PARTITIONING") == 0) {
					CMT_Sharing_Mode = SSD_Components::CMT_Sharing_Mode::EQUAL_SIZE_PARTITIONING;
				} else {
					PRINT_ERROR("Unknown CMT sharing mode specified in the SSD configuration file")
				}
			} else if (strcmp(param->name(), "Plane_Allocation_Scheme") == 0) {
				std::string val = param->value();
				std::transform(val.begin(), val.end(), val.begin(), ::toupper);
				if (strcmp(val.c_str(), "CDPW") == 0) {
					Plane_Allocation_Scheme = SSD_Components::Flash_Plane_Allocation_Scheme_Type::CDPW;
				} else if (strcmp(val.c_str(), "CDWP") == 0) {
					Plane_Allocation_Scheme = SSD_Components::Flash_Plane_Allocation_Scheme_Type::CDWP;
				} else if (strcmp(val.c_str(), "CPDW") == 0) {
					Plane_Allocation_Scheme = SSD_Components::Flash_Plane_Allocation_Scheme_Type::CPDW;
				} else if (strcmp(val.c_str(), "CPWD") == 0) {
					Plane_Allocation_Scheme = SSD_Components::Flash_Plane_Allocation_Scheme_Type::CPWD;
				} else if (strcmp(val.c_str(), "CWDP") == 0) {
					Plane_Allocation_Scheme = SSD_Components::Flash_Plane_Allocation_Scheme_Type::CWDP;
				} else if (strcmp(val.c_str(), "CWPD") == 0) {
					Plane_Allocation_Scheme = SSD_Components::Flash_Plane_Allocation_Scheme_Type::CWPD;
				} else if (strcmp(val.c_str(), "DCPW") == 0) {
					Plane_Allocation_Scheme = SSD_Components::Flash_Plane_Allocation_Scheme_Type::DCPW;
				} else if (strcmp(val.c_str(), "DCWP") == 0) {
					Plane_Allocation_Scheme = SSD_Components::Flash_Plane_Allocation_Scheme_Type::DCWP;
				} else if (strcmp(val.c_str(), "DPCW") == 0) {
					Plane_Allocation_Scheme = SSD_Components::Flash_Plane_Allocation_Scheme_Type::DPCW;
				} else if (strcmp(val.c_str(), "DPWC") == 0) {
					Plane_Allocation_Scheme = SSD_Components::Flash_Plane_Allocation_Scheme_Type::DPWC;
				} else if (strcmp(val.c_str(), "DWCP") == 0) {
					Plane_Allocation_Scheme = SSD_Components::Flash_Plane_Allocation_Scheme_Type::DWCP;
				} else if (strcmp(val.c_str(), "DWPC") == 0) {
					Plane_Allocation_Scheme = SSD_Components::Flash_Plane_Allocation_Scheme_Type::DWPC;
				} else if (strcmp(val.c_str(), "PCDW") == 0) {
					Plane_Allocation_Scheme = SSD_Components::Flash_Plane_Allocation_Scheme_Type::PCDW;
				} else if (strcmp(val.c_str(), "PCWD") == 0) {
					Plane_Allocation_Scheme = SSD_Components::Flash_Plane_Allocation_Scheme_Type::PCWD;
				} else if (strcmp(val.c_str(), "PDCW") == 0) {
					Plane_Allocation_Scheme = SSD_Components::Flash_Plane_Allocation_Scheme_Type::PDCW;
				} else if (strcmp(val.c_str(), "PDWC") == 0) {
					Plane_Allocation_Scheme = SSD_Components::Flash_Plane_Allocation_Scheme_Type::PDWC;
				} else if (strcmp(val.c_str(), "PWCD") == 0) {
					Plane_Allocation_Scheme = SSD_Components::Flash_Plane_Allocation_Scheme_Type::PWCD;
				} else if (strcmp(val.c_str(), "PWDC") == 0) {
					Plane_Allocation_Scheme = SSD_Components::Flash_Plane_Allocation_Scheme_Type::PWDC;
				} else if (strcmp(val.c_str(), "WCDP") == 0) {
					Plane_Allocation_Scheme = SSD_Components::Flash_Plane_Allocation_Scheme_Type::WCDP;
				} else if (strcmp(val.c_str(), "WCPD") == 0) {
					Plane_Allocation_Scheme = SSD_Components::Flash_Plane_Allocation_Scheme_Type::WCPD;
				} else if (strcmp(val.c_str(), "WDCP") == 0) {
					Plane_Allocation_Scheme = SSD_Components::Flash_Plane_Allocation_Scheme_Type::WDCP;
				} else if (strcmp(val.c_str(), "WDPC") == 0) {
					Plane_Allocation_Scheme = SSD_Components::Flash_Plane_Allocation_Scheme_Type::WDPC;
				} else if (strcmp(val.c_str(), "WPCD") == 0) {
					Plane_Allocation_Scheme = SSD_Components::Flash_Plane_Allocation_Scheme_Type::WPCD;
				} else if (strcmp(val.c_str(), "WPDC") == 0) {
					Plane_Allocation_Scheme = SSD_Components::Flash_Plane_Allocation_Scheme_Type::WPDC;
				} else {
					PRINT_ERROR("Unknown plane allocation scheme type specified in the SSD configuration file")
				}
			} else if (strcmp(param->name(), "Transaction_Scheduling_Policy") == 0) {
				std::string val = param->value();
				std::transform(val.begin(), val.end(), val.begin(), ::toupper);
				if (strcmp(val.c_str(), "OUT_OF_ORDER") == 0) {
					Transaction_Scheduling_Policy = SSD_Components::Flash_Scheduling_Type::OUT_OF_ORDER;
				}
				else if (strcmp(val.c_str(), "PRIORITY_OUT_OF_ORDER") == 0)
				{
					Transaction_Scheduling_Policy = SSD_Components::Flash_Scheduling_Type::PRIORITY_OUT_OF_ORDER;
				}
				else if (strcmp(val.c_str(), "FLIN") == 0)
				{
					Transaction_Scheduling_Policy = SSD_Components::Flash_Scheduling_Type::FLIN;
				} else {
					PRINT_ERROR("Unknown transaction scheduling type specified in the SSD configuration file")
				}
			} else if (strcmp(param->name(), "Overprovisioning_Ratio") == 0) {
				std::string val = param->value();
				Overprovisioning_Ratio = std::stod(val);
				if(Overprovisioning_Ratio < 0.05) {
					PRINT_MESSAGE("The specified overprovisioning ratio is too small. The simluation may not run correctly.")
				}
			} else if (strcmp(param->name(), "GC_Exec_Threshold") == 0) {
				std::string val = param->value();
				GC_Exec_Threshold = std::stod(val);
			} else if (strcmp(param->name(), "GC_Block_Selection_Policy") == 0) {
				std::string val = param->value();
				std::transform(val.begin(), val.end(), val.begin(), ::toupper);
				if (strcmp(val.c_str(), "GREEDY") == 0) {
					GC_Block_Selection_Policy = SSD_Components::GC_Block_Selection_Policy_Type::GREEDY;
				} else if (strcmp(val.c_str(), "RGA") == 0) {
					GC_Block_Selection_Policy = SSD_Components::GC_Block_Selection_Policy_Type::RGA;
				} else if (strcmp(val.c_str(), "RANDOM") == 0) {
					GC_Block_Selection_Policy = SSD_Components::GC_Block_Selection_Policy_Type::RANDOM;
				} else if (strcmp(val.c_str(), "RANDOM_P") == 0) {
					GC_Block_Selection_Policy = SSD_Components::GC_Block_Selection_Policy_Type::RANDOM_P;
				} else if (strcmp(val.c_str(), "RANDOM_PP") == 0) {
					GC_Block_Selection_Policy = SSD_Components::GC_Block_Selection_Policy_Type::RANDOM_PP;
				} else if (strcmp(val.c_str(), "FIFO") == 0) {
					GC_Block_Selection_Policy = SSD_Components::GC_Block_Selection_Policy_Type::FIFO;
				} else {
					PRINT_ERROR("Unknown GC block selection policy specified in the SSD configuration file")
				}
			} else if (strcmp(param->name(), "Use_Copyback_for_GC") == 0) {
					std::string val = param->value();
					std::transform(val.begin(), val.end(), val.begin(), ::toupper);
					Use_Copyback_for_GC = (val.compare("FALSE") == 0 ? false : true);
			} else if (strcmp(param->name(), "Preemptible_GC_Enabled") == 0) {
				std::string val = param->value();
				std::transform(val.begin(), val.end(), val.begin(), ::toupper);
				Preemptible_GC_Enabled = (val.compare("FALSE") == 0? false : true);
			} else if (strcmp(param->name(), "GC_Hard_Threshold") == 0) {
				std::string val = param->value();
				GC_Hard_Threshold = std::stod(val);
			} else if (strcmp(param->name(), "Dynamic_Wearleveling_Enabled") == 0) {
				std::string val = param->value();
				std::transform(val.begin(), val.end(), val.begin(), ::toupper);
				Dynamic_Wearleveling_Enabled = (val.compare("FALSE") == 0 ? false : true);
			} else if (strcmp(param->name(), "Static_Wearleveling_Enabled") == 0) {
				std::string val = param->value();
				std::transform(val.begin(), val.end(), val.begin(), ::toupper);
				Static_Wearleveling_Enabled = (val.compare("FALSE") == 0 ? false : true);
			} else if (strcmp(param->name(), "Static_Wearleveling_Threshold") == 0) {
				std::string val = param->value();
				Static_Wearleveling_Threshold = std::stoul(val);
			} else if (strcmp(param->name(), "Prefered_suspend_erase_time_for_read") == 0) {
				std::string val = param->value();
				Preferred_suspend_erase_time_for_read = std::stoull(val);
			} else if (strcmp(param->name(), "Preferred_suspend_erase_time_for_write") == 0) {
				std::string val = param->value();
				Preferred_suspend_erase_time_for_write = std::stoull(val);
			} else if (strcmp(param->name(), "Preferred_suspend_write_time_for_read") == 0) {
				std::string val = param->value();
				Preferred_suspend_write_time_for_read = std::stoull(val);
			} else if (strcmp(param->name(), "Flash_Channel_Count") == 0) {
				std::string val = param->value();
				Flash_Channel_Count = std::stoul(val);
			} else if (strcmp(param->name(), "Flash_Channel_Width") == 0) {
				std::string val = param->value();
				Flash_Channel_Width = std::stoul(val);
			} else if (strcmp(param->name(), "Channel_Transfer_Rate") == 0) {
				std::string val = param->value();
				Channel_Transfer_Rate = std::stoul(val);
			} else if (strcmp(param->name(), "Chip_No_Per_Channel") == 0) {
				std::string val = param->value();
				Chip_No_Per_Channel = std::stoul(val);
			} else if (strcmp(param->name(), "Flash_Comm_Protocol") == 0) {
				std::string val = param->value();
				std::transform(val.begin(), val.end(), val.begin(), ::toupper);
				if (strcmp(val.c_str(), "NVDDR2") == 0) {
					Flash_Comm_Protocol = SSD_Components::ONFI_Protocol::NVDDR2;
				} else {
					PRINT_ERROR("Unknown flash communication protocol type specified in the SSD configuration file")
				}
			}
			else if (strcmp(param->name(), "Flash_Parameter_Set") == 0)
			{
				Flash_Parameters.XML_deserialize(param);
			}
		}
	}
	catch (...)
	{
		PRINT_ERROR("Error in Device_Parameter_Set!")
	}
}
