#ifndef HOST_PARAMETER_SEt_H
#define HOST_PARAMETER_SEt_H

#include <vector>
#include "Parameter_Set_Base.h"
#include "IO_Flow_Parameter_Set.h"

class Host_Parameter_Set : public Parameter_Set_Base
{
public:
	static double PCIe_Lane_Bandwidth;//uint is GB/s
	static unsigned int PCIe_Lane_Count;
	static sim_time_type SATA_Processing_Delay;//The overall hardware and software processing delay to send/receive a SATA message in nanoseconds
	static bool Enable_ResponseTime_Logging;
	static sim_time_type ResponseTime_Logging_Period_Length;
	static std::vector<IO_Flow_Parameter_Set*> IO_Flow_Definitions;
	static std::string Input_file_path;//This parameter is not serialized. This is used to inform the Host_System class about the input file path.

	void XML_serialize(Utils::XmlWriter& xmlwriter);
	void XML_deserialize(rapidxml::xml_node<> *node);
};

#endif // !HOST_PARAMETER_SEt_H
