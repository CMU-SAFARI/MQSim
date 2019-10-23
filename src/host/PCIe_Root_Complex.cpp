#include "PCIe_Root_Complex.h"


namespace Host_Components
{
	PCIe_Root_Complex::PCIe_Root_Complex(PCIe_Link* pcie_link, HostInterface_Types SSD_device_type, SATA_HBA* sata_hba, std::vector<Host_Components::IO_Flow_Base*>* IO_flows) :
		pcie_link(pcie_link), SSD_device_type(SSD_device_type), sata_hba(sata_hba), IO_flows(IO_flows) {}

	void PCIe_Root_Complex::Write_to_memory(const uint64_t address, const void* payload)
	{
		//This is a request to write back a read request data into memory (in modern systems the write is done to LLC)
		if (address >= DATA_MEMORY_REGION) {
			//nothing to do
		} else {
			switch (SSD_device_type) {
				case HostInterface_Types::NVME:
				{
					unsigned int flow_id = QUEUE_ID_TO_FLOW_ID(((Completion_Queue_Entry*)payload)->SQ_ID);
					((*IO_flows)[flow_id])->NVMe_consume_io_request((Completion_Queue_Entry*)payload);
					break;
				}
				case HostInterface_Types::SATA:
					sata_hba->SATA_consume_io_request((Completion_Queue_Entry*)payload);
					break;
				default:
					PRINT_ERROR("Uknown Host Interface type in PCIe_Root_Complex")
			}
		}
	}

	void PCIe_Root_Complex::Write_to_device(uint64_t address, uint16_t write_value)
	{
		PCIe_Message* pcie_message = new Host_Components::PCIe_Message;
		pcie_message->Type = PCIe_Message_Type::WRITE_REQ;
		pcie_message->Destination = Host_Components::PCIe_Destination_Type::DEVICE;
		pcie_message->Address = address;
		pcie_message->Payload = (void*)(intptr_t)write_value;
		pcie_message->Payload_size = sizeof(write_value);
		pcie_link->Deliver(pcie_message);
	}

	void PCIe_Root_Complex::Read_from_memory(const uint64_t address, const unsigned int read_size)
	{
		PCIe_Message* new_pcie_message = new Host_Components::PCIe_Message;
		new_pcie_message->Type = PCIe_Message_Type::READ_COMP;
		new_pcie_message->Destination = Host_Components::PCIe_Destination_Type::DEVICE;
		new_pcie_message->Address = address;

		//This is a request to read the data of a write request
		if (address >= DATA_MEMORY_REGION) {
			//nothing to do
			new_pcie_message->Payload_size = read_size;
			new_pcie_message->Payload = NULL;//No need to transfer data in the standalone mode of MQSim
		} else {
			switch (SSD_device_type) {
				case HostInterface_Types::NVME:
				{
					uint16_t flow_id = QUEUE_ID_TO_FLOW_ID(uint16_t(address >> NVME_COMP_Q_MEMORY_REGION));
					new_pcie_message->Payload = (*IO_flows)[flow_id]->NVMe_read_sqe(address);
					new_pcie_message->Payload_size = sizeof(Submission_Queue_Entry);
					break;
				}
				case HostInterface_Types::SATA:
					new_pcie_message->Payload = sata_hba->Read_ncq_entry(address);
					new_pcie_message->Payload_size = sizeof(Submission_Queue_Entry);
					break;
			}
		}

		pcie_link->Deliver(new_pcie_message);
	}
	
	void PCIe_Root_Complex::Set_io_flows(std::vector<Host_Components::IO_Flow_Base*>* IO_flows)
	{
		this->IO_flows = IO_flows;
	}
}
