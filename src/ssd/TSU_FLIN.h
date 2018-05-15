#ifndef TSU_FLIN_H
#define TSU_FLIN_H
/*
#include <list>
#include <set>
#include "TSU_Base.h"
#include "NVM_Transaction_Flash.h"
#include "NVM_PHY_ONFI_NVDDR2.h"
#include "FTL.h"


namespace SSD_Components
{
	class FTL;

	struct FLIN_Flow_Monitoring_Unit
	{
		unsigned int Serviced_read_requests_history;//FLIN, in its current implementation, does not use history!
		unsigned int Serviced_read_requests_recent;
		unsigned int Serviced_read_requests_total;
		unsigned int Serviced_write_requests_history;//FLIN, in its current implementation, does not use history!
		unsigned int Serviced_write_requests_recent;
		unsigned int Serviced_write_requests_total;
		double Sum_read_slowdown;
		double Sum_write_slowdown;
	};

	*
	* This class implements a transaction scheduling unit which supports:
	* 1. Out-of-order execution of flash transactions, similar to the Sprinkler proposal
	*    described in "Jung et al., Sprinkler: Maximizing resource utilization in many-chip
	*    solid state disks, HPCA, 2014".
	* 2. Program and erase suspension, similar to the proposal described in "G. Wu and X. He,
	*    Reducing SSD read latency via NAND flash program and erase suspension, FAST 2012".
	*
	class TSU_FLIN : public TSU_Base
	{
	public:
		TSU_FLIN(const sim_object_id_type& id, FTL* ftl, NVM_PHY_ONFI_NVDDR2* NVMController, 
			const unsigned int Channel_no, const unsigned int chip_no_per_channel, const unsigned int die_no_per_chip, const unsigned int plane_no_per_die, unsigned int flash_page_size,
			const sim_time_type flow_classification_epoch, const unsigned int alpha_read, const unsigned int alpha_write,
			const unsigned int no_of_priority_classes, const stream_id_type max_flow_id, const unsigned int* stream_count_per_priority_class, stream_id_type** stream_ids_per_priority_class,
			const double f_thr,
			const sim_time_type WriteReasonableSuspensionTimeForRead,
			const sim_time_type EraseReasonableSuspensionTimeForRead,
			const sim_time_type EraseReasonableSuspensionTimeForWrite,
			const bool EraseSuspensionEnabled, const bool ProgramSuspensionEnabled);
		~TSU_FLIN();
		void Prepare_for_transaction_submit();
		void Submit_transaction(NVM_Transaction_Flash* transaction);
		void Schedule();

		void Start_simulation();
		void Validate_simulation_config();
		void Execute_simulator_event(MQSimEngine::Sim_Event*);
		void Report_results_in_XML(std::string name_prefix, Utils::XmlWriter& xmlwriter);
	private:
		unsigned int* stream_count_per_priority_class;
		stream_id_type** stream_ids_per_priority_class;
		FLIN_Flow_Monitoring_Unit**** flow_activity_info;
		sim_time_type flow_classification_epoch;
		unsigned int alpha_read_for_epoch, alpha_write_for_epoch;
		unsigned int no_of_priority_classes;
		double F_thr;//Fairness threshold for high intensity flows, as described in Alg 1 of the FLIN paper
		std::set<stream_id_type> ***low_intensity_class_read, *** low_intensity_class_write;//As described in Alg 1 of the FLIN paper
		std::list<NVM_Transaction_Flash*>::iterator*** head_high_read;//Due to programming limitations, for read queues, MQSim keeps Head_high instread of Tail_low which is described in Alg 1 of the FLIN paper
		std::list<NVM_Transaction_Flash*>::iterator*** head_high_write;//Due to programming limitations, for write queues, MQSim keeps Head_high instread of Tail_low which is described in Alg 1 of the FLIN paper
		Flash_Transaction_Queue*** UserReadTRQueue;
		NVM_Transaction_Flash_RD**** Read_slot;
		Flash_Transaction_Queue*** UserWriteTRQueue;
		NVM_Transaction_Flash_WR**** Write_slot;
		Flash_Transaction_Queue** GCReadTRQueue;
		Flash_Transaction_Queue** GCWriteTRQueue;
		Flash_Transaction_Queue** GCEraseTRQueue;
		Flash_Transaction_Queue** MappingReadTRQueue;
		Flash_Transaction_Queue** MappingWriteTRQueue;

		void reorder_for_fairness(Flash_Transaction_Queue* queue, std::list<NVM_Transaction_Flash*>::iterator start, std::list<NVM_Transaction_Flash*>::iterator end);
		void estimate_alone_waiting_time(Flash_Transaction_Queue* queue, std::list<NVM_Transaction_Flash*>::iterator position);
		double fairness_based_on_average_slowdown(unsigned int channel_id, unsigned int chip_id, unsigned int priority_class, bool is_read, stream_id_type& flow_with_max_average_slowdown);
		void move_forward(Flash_Transaction_Queue* queue, std::list<NVM_Transaction_Flash*>::iterator TRnew_pos, std::list<NVM_Transaction_Flash*>::iterator ultimate_posistion);
		bool service_read_transaction(NVM::FlashMemory::Flash_Chip* chip);
		bool service_write_transaction(NVM::FlashMemory::Flash_Chip* chip);
		bool service_erase_transaction(NVM::FlashMemory::Flash_Chip* chip);


		void initialize_scheduling_turns();
		std::vector<unsigned int> scheduling_turn_assignments_read, scheduling_turn_assignments_write;
		int current_turn_read, current_turn_write;
	};
}
*/

#endif //!TSU_FLIN