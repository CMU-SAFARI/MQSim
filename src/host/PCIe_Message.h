#ifndef PCIE_MESSAGE_H
#define PCIE_MESSAGE_H

#include<cstdint>

namespace Host_Components
{
	enum class PCIe_Destination_Type {HOST, DEVICE};
	enum class PCIe_Message_Type {READ_REQ, WRITE_REQ, READ_COMP};
	
	class PCIe_Message
	{
	public:
		PCIe_Destination_Type Destination;
		PCIe_Message_Type Type;
		void* Payload;
		unsigned int Payload_size;
		uint64_t Address;
	};
}

#endif //!PCIE_MESSAGE_H
