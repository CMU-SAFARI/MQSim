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
	static std::vector<IO_Flow_Parameter_Set*> IO_Flow_Definitions;

	void XML_serialize(Utils::XmlWriter& xmlwriter);
	void XML_deserialize(rapidxml::xml_node<> *node);
};

#endif // !HOST_PARAMETER_SEt_H
