#ifndef HOST_INTERFACE_SATA_H
#define HOST_INTERFACE_SATA_H

#include "Host_Interface_Base.h"

namespace SSD_Components
{
	class Host_Interface_SATA : public Host_Interface_Base
	{
		void Report_results_in_XML(std::string name_prefix, Utils::XmlWriter& xmlwriter);
	};
}

#endif // !HOST_INTERFACE_SATA_H
