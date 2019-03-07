#ifndef SATA_HBA_H
#define SATA_HBA_H

#include <stdint.h>
#include <unordered_map>
#include <set>
#include <list>
#include "../sim/Sim_Defs.h"
#include "../sim/Sim_Object.h"
#include "Host_IO_Request.h"
#include "IO_Flow_Base.h"
#include "PCIe_Root_Complex.h"

namespace Host_Components
{
#define SATA_SQ_FULL(Q) (Q.Submission_queue_tail < Q.Submission_queue_size - 1 ? Q.Submission_queue_tail + 1 == Q.Submission_queue_head : Q.Submission_queue_head == 0)
#define SATA_UPDATE_SQ_TAIL(Q)  Q.Submission_queue_tail++;\
						if (Q.Submission_queue_tail == Q.Submission_queue_size)\
							Q.Submission_queue_tail = 0;

	struct NCQ_Control_Structure //SATA native command queue
	{
		uint16_t Submission_queue_head;
		uint16_t Submission_queue_tail;
		uint16_t Submission_queue_size;
		uint64_t Submission_tail_register_address_on_device;
		uint64_t Submission_queue_memory_base_address;
		uint16_t Completion_queue_head;
		uint16_t Completion_queue_tail;
		uint16_t Completion_queue_size;
		uint64_t Completion_head_register_address_on_device;
		uint64_t Completion_queue_memory_base_address;
		std::unordered_map<sim_time_type, Host_IO_Request*> queue;//Contains the I/O requests that are enqueued in the NCQ
	};

	enum class HBA_Sim_Events {SUBMIT_IO_REQUEST, CONSUME_IO_REQUEST};

	class IO_Flow_Base;
	class PCIe_Root_Complex;
	class SATA_HBA : MQSimEngine::Sim_Object
	{
	public:
		SATA_HBA(sim_object_id_type id, uint16_t ncq_size, sim_time_type hba_processing_delay, PCIe_Root_Complex* pcie_root_complex, std::vector<Host_Components::IO_Flow_Base*>* IO_flows);
		~SATA_HBA();
		void Start_simulation();
		void Validate_simulation_config();
		void Execute_simulator_event(MQSimEngine::Sim_Event*);
		void Submit_io_request(Host_IO_Request* request);
		void SATA_consume_io_request(Completion_Queue_Entry* cqe);
		Submission_Queue_Entry* Read_ncq_entry(uint64_t address);
		const NCQ_Control_Structure* Get_sata_ncq_info();
		void Set_io_flows(std::vector<Host_Components::IO_Flow_Base*>* IO_flows);
		void Set_root_complex(PCIe_Root_Complex*);
	private:
		uint16_t ncq_size;
		sim_time_type hba_processing_delay;//This estimates the processing delay of the whole SATA software and hardware layers to send/receive a SATA message
		PCIe_Root_Complex * pcie_root_complex;
		std::vector<Host_Components::IO_Flow_Base*>* IO_flows;
		NCQ_Control_Structure sata_ncq;
		std::set<uint16_t> available_command_ids;
		std::vector<Host_IO_Request*> request_queue_in_memory;
		std::list<Host_IO_Request*> waiting_requests_for_submission;//The I/O requests that are still waiting (since the I/O queue is full) to be enqueued in the I/O queue 
		void Update_and_submit_ncq_completion_info();
		std::queue<Completion_Queue_Entry*> consume_requests;
		std::queue<Host_IO_Request*> host_requests;
	};
}

#endif // !SATA_HBA_H
