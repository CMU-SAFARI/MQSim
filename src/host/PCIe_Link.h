#ifndef PCIE_LINK_H
#define PCIE_LINK_H

#include "queue"
#include "../sim/Sim_Defs.h"
#include "../sim/Sim_Object.h"
#include "../sim/Sim_Event.h"
#include "PCIe_Message.h"
#include "PCIe_Root_Complex.h"
#include "PCIe_Switch.h"

namespace Host_Components
{
	class PCIe_Switch;
	class PCIe_Root_Complex;
	enum class PCIe_Link_Event_Type {DELIVER};
	
	class PCIe_Link : public MQSimEngine::Sim_Object
	{
	public:
		PCIe_Link(const sim_object_id_type& id, PCIe_Root_Complex* root_complex, PCIe_Switch* pcie_switch,
			double lane_bandwidth_GBPs = 1, int lane_count = 4,
			int tlp_header_size = 20,//tlp header size in a 64-bit machine
			int tlp_max_payload_size = 128,
			int dllp_ovehread = 6,
			int ph_overhead = 2);
		void Deliver(PCIe_Message*);
		void Start_simulation();
		void Validate_simulation_config();
		void Execute_simulator_event(MQSimEngine::Sim_Event*);
		void Set_root_complex(PCIe_Root_Complex*);
		void Set_pcie_switch(PCIe_Switch*);
	private:
		PCIe_Root_Complex* root_complex;
		PCIe_Switch* pcie_switch;
		double lane_bandwidth_GBPs;//GB/s
		int lane_count;
		int tlp_header_size, tlp_max_payload_size;
		int dllp_ovehread, ph_overhead;
		//sim_time_type byte_transfer_delay_per_lane;//Since the transfer delay of one byte may take lower than one nano-second, we use 8-byte metric 
		int packet_overhead;
		std::queue<PCIe_Message*> Message_buffer_toward_ssd_device;
		
		sim_time_type estimate_transfer_time(PCIe_Message* message)
		{
			switch (message->Type) {
				case PCIe_Message_Type::READ_COMP:
				case PCIe_Message_Type::WRITE_REQ:
				{
					int total_transfered_bytes = (message->Payload_size / tlp_max_payload_size) * (tlp_max_payload_size + packet_overhead)
						+ (message->Payload_size % tlp_max_payload_size == 0 ? 0 : message->Payload_size % tlp_max_payload_size + packet_overhead);
					return (sim_time_type)(((double)((total_transfered_bytes / lane_count) + (total_transfered_bytes % lane_count == 0 ? 0 : 1))) / lane_bandwidth_GBPs);
				}
				case PCIe_Message_Type::READ_REQ:
					return (sim_time_type)((((packet_overhead + 4) / lane_count) + ((packet_overhead + 4) % lane_count == 0 ? 0 : 1)) / lane_bandwidth_GBPs);
			}
			
			return 0;
		}
		
		std::queue<PCIe_Message*> Message_buffer_toward_root_complex;
	};
}

#endif
