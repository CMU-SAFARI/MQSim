#ifndef PCIE_SWITCH_H
#define PCIE_SWITCH_H

#include "PCIe_Message.h"
#include "PCIe_Link.h"
#include "../ssd/Host_Interface_Base.h"

namespace SSD_Components
{
	class Host_Interface_Base;
}

namespace Host_Components
{
	class PCIe_Link;
	class PCIe_Switch
	{
	public:
		PCIe_Switch(PCIe_Link* pcie_link, SSD_Components::Host_Interface_Base* host_interface);//, SSD_Components::Host_Interface_Base* host_interface
		void Deliver_to_device(PCIe_Message*);
		void Send_to_host(PCIe_Message*);
		void Attach_ssd_device(SSD_Components::Host_Interface_Base* host_interface);
		bool Is_ssd_connected();
	private:
		PCIe_Link* pcie_link;
		SSD_Components::Host_Interface_Base* host_interface;
	};
}
#endif //!PCIE_SWITCH_H
