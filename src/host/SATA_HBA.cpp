#include "SATA_HBA.h"

namespace Host_Components
{
	SATA_HBA::SATA_HBA(sim_object_id_type id, uint16_t ncq_size, sim_time_type hba_processing_delay, PCIe_Root_Complex* pcie_root_complex, std::vector<Host_Components::IO_Flow_Base*>* IO_flows) :
		MQSimEngine::Sim_Object(id), ncq_size(ncq_size), hba_processing_delay(hba_processing_delay), pcie_root_complex(pcie_root_complex), IO_flows(IO_flows)
	{
		for (uint16_t cmdid = 0; cmdid < (uint16_t)(0xffffffff); cmdid++) {
			available_command_ids.insert(cmdid);
		}
		Host_IO_Request* t = NULL;
		for (uint16_t cmdid = 0; cmdid < ncq_size; cmdid++) {
			request_queue_in_memory.push_back(t);
		}
		sata_ncq.Submission_queue_size = ncq_size;
		sata_ncq.Submission_queue_head = 0;
		sata_ncq.Submission_queue_tail = 0;
		sata_ncq.Completion_queue_size = ncq_size;
		sata_ncq.Completion_queue_head = 0;
		sata_ncq.Completion_queue_tail = 0;
		sata_ncq.Submission_queue_memory_base_address = SUBMISSION_QUEUE_MEMORY_1;
		sata_ncq.Submission_tail_register_address_on_device = NCQ_SUBMISSION_REGISTER;
		sata_ncq.Completion_queue_memory_base_address = COMPLETION_QUEUE_MEMORY_1;
		sata_ncq.Completion_head_register_address_on_device = NCQ_COMPLETION_REGISTER;
	}

	SATA_HBA::~SATA_HBA()
	{
		for (auto &req : sata_ncq.queue) {
			if (req.second) {
				delete req.second;
			}
		}
		for (auto &req : waiting_requests_for_submission) {
			if (req) {
				delete req;
			}
		}
	}

	void SATA_HBA::Start_simulation()
	{
	}

	void SATA_HBA::Validate_simulation_config()
	{
	}

	void SATA_HBA::Execute_simulator_event(MQSimEngine::Sim_Event* event)
	{
		switch ((HBA_Sim_Events)event->Type)
		{
			case HBA_Sim_Events::CONSUME_IO_REQUEST:
			{
				Completion_Queue_Entry* cqe = consume_requests.front();
				consume_requests.pop();
				//Find the request and update statistics
				Host_IO_Request* request = sata_ncq.queue[cqe->Command_Identifier];
				sata_ncq.queue.erase(cqe->Command_Identifier);
				available_command_ids.insert(cqe->Command_Identifier);
				sata_ncq.Submission_queue_head = cqe->SQ_Head;

				((*IO_flows)[request->Source_flow_id])->SATA_consume_io_request(request);

				Update_and_submit_ncq_completion_info();

				//If the submission queue is not full anymore, then enqueue waiting requests
				while (waiting_requests_for_submission.size() > 0) {
					if (!SATA_SQ_FULL(sata_ncq) && available_command_ids.size() > 0)
					{
						Host_IO_Request* new_req = waiting_requests_for_submission.front();
						waiting_requests_for_submission.pop_front();
						if (sata_ncq.queue[*available_command_ids.begin()] != NULL) {
							PRINT_ERROR("Unexpteced situation in SATA_HBA! Overwriting a waiting I/O request in the queue!")
						} else {
							new_req->IO_queue_info = *available_command_ids.begin();
							sata_ncq.queue[*available_command_ids.begin()] = new_req;
							available_command_ids.erase(available_command_ids.begin());
							request_queue_in_memory[sata_ncq.Submission_queue_tail] = new_req;
							SATA_UPDATE_SQ_TAIL(sata_ncq);
						}
						new_req->Enqueue_time = Simulator->Time();
						pcie_root_complex->Write_to_device(sata_ncq.Submission_tail_register_address_on_device, sata_ncq.Submission_queue_tail);//Based on NVMe protocol definition, the updated tail pointer should be informed to the device
					} else {
						break;
					}
				}

				delete cqe;

				if(consume_requests.size() > 0) {
					Simulator->Register_sim_event(Simulator->Time() + hba_processing_delay, this, NULL, static_cast<int>(HBA_Sim_Events::CONSUME_IO_REQUEST));
				}
				break;
			}
			case HBA_Sim_Events::SUBMIT_IO_REQUEST:
			{
				Host_IO_Request* request = host_requests.front();
				host_requests.pop();

				//If the hardware queue is full
				if (SATA_SQ_FULL(sata_ncq) || available_command_ids.size() == 0) {
					waiting_requests_for_submission.push_back(request);
				} else {
					if (sata_ncq.queue[*available_command_ids.begin()] != NULL) {
						PRINT_ERROR("Unexpteced situation in IO_Flow_Base! Overwriting an unhandled I/O request in the queue!")
					} else {
						request->IO_queue_info = *available_command_ids.begin();
						sata_ncq.queue[*available_command_ids.begin()] = request;
						available_command_ids.erase(available_command_ids.begin());
						request_queue_in_memory[sata_ncq.Submission_queue_tail] = request;
						SATA_UPDATE_SQ_TAIL(sata_ncq);
					}
					request->Enqueue_time = Simulator->Time();
					pcie_root_complex->Write_to_device(sata_ncq.Submission_tail_register_address_on_device, sata_ncq.Submission_queue_tail);//Based on NVMe protocol definition, the updated tail pointer should be informed to the device
				}

				if (host_requests.size() > 0) {
					Simulator->Register_sim_event(Simulator->Time() + hba_processing_delay, this, NULL, static_cast<int>(HBA_Sim_Events::SUBMIT_IO_REQUEST));
				}

				break;
			}
		} //switch ((HBA_Sim_Events)event->Type)
	}

	void SATA_HBA::Submit_io_request(Host_IO_Request* request)
	{
		host_requests.push(request);
		if (host_requests.size() == 1) {
			Simulator->Register_sim_event(Simulator->Time() + hba_processing_delay, this, NULL, static_cast<int>(HBA_Sim_Events::SUBMIT_IO_REQUEST));
		}
	}

	void SATA_HBA::SATA_consume_io_request(Completion_Queue_Entry* cqe)
	{
		consume_requests.push(cqe);
		if (consume_requests.size() == 1) {
			Simulator->Register_sim_event(Simulator->Time() + hba_processing_delay, this, NULL, static_cast<int>(HBA_Sim_Events::CONSUME_IO_REQUEST));
		}
	}
	
	Submission_Queue_Entry* SATA_HBA::Read_ncq_entry(uint64_t address)
	{
		Submission_Queue_Entry* ncq_entry = new Submission_Queue_Entry;
		Host_IO_Request* request = request_queue_in_memory[(uint16_t)((address - sata_ncq.Submission_queue_memory_base_address) / sizeof(Submission_Queue_Entry))];
		
		if (request == NULL) {
			throw std::invalid_argument("SATA HBA: Request to access an NCQ entry that does not exist.");
		}

		ncq_entry->Command_Identifier = request->IO_queue_info;
		//For simplicity, MQSim's SATA host interface uses NVMe opcodes
		if (request->Type == Host_IO_Request_Type::READ) {
			ncq_entry->Opcode = NVME_READ_OPCODE;
			ncq_entry->Command_specific[0] = (uint32_t)request->Start_LBA;
			ncq_entry->Command_specific[1] = (uint32_t)(request->Start_LBA >> 32);
			ncq_entry->Command_specific[2] = ((uint32_t)((uint16_t)request->LBA_count)) & (uint32_t)(0x0000ffff);
			ncq_entry->PRP_entry_1 = (DATA_MEMORY_REGION);//Dummy addresses, just to emulate data read/write access
			ncq_entry->PRP_entry_2 = (DATA_MEMORY_REGION + 0x1000);//Dummy addresses
		} else {
			ncq_entry->Opcode = NVME_WRITE_OPCODE;
			ncq_entry->Command_specific[0] = (uint32_t)request->Start_LBA;
			ncq_entry->Command_specific[1] = (uint32_t)(request->Start_LBA >> 32);
			ncq_entry->Command_specific[2] = ((uint32_t)((uint16_t)request->LBA_count)) & (uint32_t)(0x0000ffff);
			ncq_entry->PRP_entry_1 = (DATA_MEMORY_REGION);//Dummy addresses, just to emulate data read/write access
			ncq_entry->PRP_entry_2 = (DATA_MEMORY_REGION + 0x1000);//Dummy addresses
		}

		return ncq_entry;
	}

	void SATA_HBA::Update_and_submit_ncq_completion_info()
	{
		sata_ncq.Completion_queue_head++;
		if (sata_ncq.Completion_queue_head == sata_ncq.Completion_queue_size) {
			sata_ncq.Completion_queue_head = 0;
		}
		pcie_root_complex->Write_to_device(sata_ncq.Completion_head_register_address_on_device, sata_ncq.Completion_queue_head);//Based on NVMe protocol definition, the updated head pointer should be informed to the device
	}

	const NCQ_Control_Structure* SATA_HBA::Get_sata_ncq_info()
	{
		return &sata_ncq;
	}

	void SATA_HBA::Set_io_flows(std::vector<Host_Components::IO_Flow_Base*>* IO_flows)
	{
		this->IO_flows = IO_flows;
	}

	void SATA_HBA::Set_root_complex(PCIe_Root_Complex* pcie_root_complex)
	{
		this->pcie_root_complex = pcie_root_complex;
	}
}
