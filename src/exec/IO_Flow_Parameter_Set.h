#ifndef IO_FLOW_PARAMETER_SET_H
#define IO_FLOW_PARAMETER_SET_H

#include <string>
#include "../nvm_chip/flash_memory/FlashTypes.h"
#include "../ssd/Host_Interface_Defs.h"
#include "../host/IO_Flow_Synthetic.h"
#include "Parameter_Set_Base.h"

enum class Flow_Type { SYNTHETIC, TRACE };
class IO_Flow_Parameter_Set : public Parameter_Set_Base
{
public:
	SSD_Components::Caching_Mode Device_Level_Data_Caching_Mode;
	Flow_Type Type;
	IO_Flow_Priority_Class Priority_Class;//The priority class is only considered when the SSD device uses NVMe host interface
	flash_channel_ID_type* Channel_IDs;//Resource partitioning: which channel ids are allocated to this flow
	flash_chip_ID_type* Chip_IDs;//Resource partitioning: which chip ids are allocated to this flow
	flash_die_ID_type* Die_IDs;//Resource partitioning: which die ids are allocted to this flow
	flash_plane_ID_type* Plane_IDs;//Resource partitioning: which plane ids are allocated to this flow
	int Channel_No, Chip_No, Die_No, Plane_No;
	void XML_serialize(Utils::XmlWriter& xmlwrite);
	void XML_deserialize(rapidxml::xml_node<> *node);
private:
};

class IO_Flow_Parameter_Set_Synthetic : public IO_Flow_Parameter_Set
{
public:
	char Read_Percentage;
	Host_Components::Address_Distribution_Type Address_Distribution;
	char Percentage_of_Hot_Region;//This parameters used if the address distribution type is hot/cold (i.e., (100-H)% of the whole I/O requests are going to a H% hot region of the storage space)
	Host_Components::Request_Size_Distribution_Type Request_Size_Distribution;
	unsigned int Average_Request_Size;//Average request size in sectors
	unsigned int Variance_Request_Size;//Variance of request size in sectors
	//Host_Components::Request_Generator_Type Generator_Type;//Request generator could be time-based 
	int Seed;
	unsigned int Average_No_of_Reqs_in_Queue;//Average number of I/O requests from this flow in the 
	sim_time_type Stop_Time;//Defines when to stop generating I/O requests
	unsigned int Total_Requests_To_Generate;//If Stop_Time is equal to zero, then requst generator considers Total_Requests_To_Generate to decide when to stop generating I/O requests
	
	void XML_serialize(Utils::XmlWriter& xmlwriter);
	void XML_deserialize(rapidxml::xml_node<> *node);
};

class IO_Flow_Parameter_Set_Trace_Based : public IO_Flow_Parameter_Set
{
public:
	std::string File_Path;
	int Percentage_To_Be_Executed;
	
	void XML_serialize(Utils::XmlWriter& xmlwriter);
	void XML_deserialize(rapidxml::xml_node<> *node);
};

#endif // !IO_FLOW_PARAMETER_SET_H
