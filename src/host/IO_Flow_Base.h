#ifndef IO_FLOW_BASE_H
#define IO_FLOW_BASE_H

#include <string>
#include <iostream>
#include <unordered_map>
#include <set>
#include <list>
#include <vector>
#include "../sim/Sim_Defs.h"
#include "../sim/Sim_Object.h"
#include "../sim/Sim_Reporter.h"
#include "../ssd/SSD_Defs.h"
#include "../ssd/Host_Interface_Defs.h"
#include "Host_IO_Request.h"
#include "PCIe_Root_Complex.h"
#include "../precond/Workload_Statistics.h"

namespace Host_Components
{
	struct NVMe_Queue_Pair
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
	};

#define NVME_SQ_FULL(Q) ((Q.Submission_queue_tail + 1 == Q.Submission_queue_head) \
						|| \
						(Q.Submission_queue_head - Q.Submission_queue_tail == Q.Submission_queue_size))
#define UPDATE_SQ_TAIL(Q)  Q.Submission_queue_tail++;\
						if (Q.Submission_queue_tail == Q.Submission_queue_size)\
							nvme_queue_pair.Submission_queue_tail = 0;

	struct NCQ //SATA native command queue
	{
	};

	class PCIe_Root_Complex;
	class IO_Flow_Base : public MQSimEngine::Sim_Object, public MQSimEngine::Sim_Reporter
	{
	public:
		IO_Flow_Base(const sim_object_id_type& name, LHA_type start_lsa_on_device, LHA_type end_lsa_address_on_device, uint16_t io_queue_id,
			uint16_t nvme_submission_queue_size, uint16_t nvme_completion_queue_size, IO_Flow_Priority_Class priority_class,
			sim_time_type stop_time, double initial_occupancy_ratio, unsigned int total_requets_to_be_generated,
			HostInterfaceType SSD_device_type, PCIe_Root_Complex* pcie_root_complex,
			bool enabled_logging, sim_time_type logging_period, std::string logging_file_path);
		~IO_Flow_Base();
		void Start_simulation();
		IO_Flow_Priority_Class Priority_class() { return priority_class; }
		virtual Host_IO_Reqeust* Generate_next_request() = 0;
		virtual void NVMe_consume_io_request(Completion_Queue_Entry*);
		Submission_Queue_Entry* NVMe_read_sqe(uint64_t address);
		const NVMe_Queue_Pair* Get_nvme_queue_pair_info();
		LHA_type Get_start_lsa_on_device();
		LHA_type Get_end_lsa_address_on_device();
		uint32_t Get_generated_request_count();
		uint32_t Get_serviced_request_count();//in microseconds
		uint32_t Get_device_response_time();//in microseconds
		uint32_t Get_min_device_response_time();//in microseconds
		uint32_t Get_max_device_response_time();//in microseconds
		uint32_t Get_end_to_end_request_delay();//in microseconds
		uint32_t Get_min_end_to_end_request_delay();//in microseconds
		uint32_t Get_max_end_to_end_request_delay();//in microseconds
		void Report_results_in_XML(std::string name_prefix, Utils::XmlWriter& xmlwriter);
		virtual void Get_statistics(Preconditioning::Workload_Statistics& stats, LPA_type(*Convert_host_logical_address_to_device_address)(LHA_type lha),
			page_status_type(*Find_NVM_subunit_access_bitmap)(LHA_type lha)) = 0;
	protected:
		LHA_type start_lsa_on_device, end_lsa_on_device;
		uint16_t io_queue_id;
		uint16_t nvme_submission_queue_size;
		uint16_t nvme_completion_queue_size;
		IO_Flow_Priority_Class priority_class;
		double initial_occupancy_ratio;//The initial amount of valid logical pages when pereconditioning is performed
		sim_time_type stop_time;//The flow stops generating request when simulation time reaches stop_time
		unsigned int total_requests_to_be_generated;//If stop_time is zero, then the flow stops generating request when the number of generated requests is equal to total_req_count
		HostInterfaceType SSD_device_type;
		PCIe_Root_Complex* pcie_root_complex;

		std::unordered_map<sim_time_type, Host_IO_Reqeust*> software_request_queue;//The I/O requests that are enqueued in the I/O queue of the SSD device
		std::vector<Host_IO_Reqeust*> request_queue_in_memory;
		std::set<uint16_t> available_command_ids;
		std::list<Host_IO_Reqeust*> waiting_requests;//The I/O requests that are still waiting (since the I/O queue is full) to be enqueued in the I/O queue 
		NVMe_Queue_Pair nvme_queue_pair;
		static unsigned int last_id;
		int id;
		sim_time_type arrival_time_of_first_request;
		sim_time_type simulation_start_time_offset;
		LHA_type address_offset;
		LHA_type max_allowed_lsn;//Maximum allowed logical sector number for this input flow


		unsigned int STAT_generated_request_count, STAT_generated_read_request_count, STAT_generated_write_request_count;
		unsigned int STAT_ignored_request_count;
		unsigned int STAT_serviced_request_count, STAT_serviced_read_request_count, STAT_serviced_write_request_count;
		sim_time_type STAT_sum_device_response_time, STAT_sum_device_response_time_read, STAT_sum_device_response_time_write;
		sim_time_type STAT_min_device_response_time, STAT_min_device_response_time_read, STAT_min_device_response_time_write;
		sim_time_type STAT_max_device_response_time, STAT_max_device_response_time_read, STAT_max_device_response_time_write;
		sim_time_type STAT_sum_request_delay, STAT_sum_request_delay_read, STAT_sum_request_delay_write;
		sim_time_type STAT_min_request_delay, STAT_min_request_delay_read, STAT_min_request_delay_write;
		sim_time_type STAT_max_request_delay, STAT_max_request_delay_read, STAT_max_request_delay_write;
		sim_time_type STAT_transferred_bytes_total, STAT_transferred_bytes_read, STAT_transferred_bytes_write;
		int progress;
		int next_progress_step = 0;

		bool enabled_logging;
		sim_time_type logging_period;
		sim_time_type next_logging_milestone;
		std::string logging_file_path;
		std::ofstream log_file;
		uint32_t Get_device_response_time_short_term();//in microseconds
		uint32_t Get_end_to_end_request_delay_short_term();//in microseconds
		sim_time_type STAT_sum_device_response_time_short_term, STAT_sum_request_delay_short_term;
		unsigned int STAT_serviced_request_count_short_term;

		void submit_io_request(Host_IO_Reqeust*);
		void NVMe_update_and_submit_completion_queue_tail();
	};
}

#endif // !IO_FLOW_BASE_H
