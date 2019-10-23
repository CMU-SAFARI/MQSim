#include "PCIe_Switch.h"

namespace Host_Components
{
	PCIe_Switch::PCIe_Switch(PCIe_Link* pcie_link, SSD_Components::Host_Interface_Base* host_interface) :
		pcie_link(pcie_link), host_interface(host_interface)
	{
	}

	void PCIe_Switch::Deliver_to_device(PCIe_Message* message)
	{
		host_interface->Consume_pcie_message(message);
	}

	void PCIe_Switch::Send_to_host(PCIe_Message* message)
	{
		pcie_link->Deliver(message);
	}
	
	void PCIe_Switch::Attach_ssd_device(SSD_Components::Host_Interface_Base* host_interface)
	{
		this->host_interface = host_interface;
	}

	bool PCIe_Switch::Is_ssd_connected()
	{
		return this->host_interface != NULL;
	}
}
