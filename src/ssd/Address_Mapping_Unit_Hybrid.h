#ifndef ADDRESS_MAPPING_UNIT_HYBRID_H
#define ADDRESS_MAPPING_UNIT_HYBRID_H

#include "Address_Mapping_Unit_Base.h"

namespace SSD_Components
{
	class Address_Mapping_Unit_Hybrid : public Address_Mapping_Unit_Base
	{
	public:
		Address_Mapping_Unit_Hybrid(sim_object_id_type id, FTL* ftl, NVM_PHY_ONFI* flash_controller, Flash_Block_Manager_Base* BlockManager,
			bool ideal_mapping_table, unsigned int ConcurrentStreamNo,
			unsigned int ChannelCount, unsigned int ChipNoPerChannel, unsigned int DieNoPerChip, unsigned int PlaneNoPerDie,
			unsigned int Block_no_per_plane, unsigned int Page_no_per_block, unsigned int SectorsPerPage, unsigned int PageSizeInBytes,
			double Overprovisioning_ratio, bool fold_large_addresses = true);
		void Setup_triggers();
		void Start_simulation();
		void Validate_simulation_config();
		void Execute_simulator_event(MQSimEngine::Sim_Event*);

		void Allocate_address_for_preconditioning(const stream_id_type stream_id, const std::vector<LPA_type> lpa_list, const std::vector<unsigned int> size, std::vector<NVM::FlashMemory::Physical_Page_Address>& address);
		void Touch_address_for_preconditioning(stream_id_type stream_id, LPA_type lpa);
		void Translate_lpa_to_ppa_and_dispatch(const std::list<NVM_Transaction*>& transactionList);
		void Get_data_mapping_info_for_gc(const stream_id_type stream_id, const LPA_type lpa, PPA_type& ppa, page_status_type& page_state);
		void Get_translation_mapping_info_for_gc(const stream_id_type stream_id, const MVPN_type mvpn, MPPN_type& mppa, sim_time_type& timestamp);
		bool Check_address_range(const stream_id_type streamID, const LPA_type lsn, const unsigned int size);
		void Allocate_new_page_for_gc(NVM_Transaction_Flash_WR* transaction, bool is_translation_page);

		LSA_type Get_logical_sectors_count(stream_id_type stream_id);
		unsigned int Get_logical_pages_count(stream_id_type stream_id);
		NVM::FlashMemory::Physical_Page_Address Convert_ppa_to_address(const PPA_type ppa);
		void Convert_ppa_to_address(const PPA_type ppn, NVM::FlashMemory::Physical_Page_Address& address);
		PPA_type Convert_address_to_ppa(const NVM::FlashMemory::Physical_Page_Address& pageAddress);

		void Lock_lpa(stream_id_type stream_id, LPA_type lpa);
		void Unlock_lpa(stream_id_type stream_id, LPA_type lpa);
		void Lock_mvpn(stream_id_type stream_id, MVPN_type mpvn);
		void Unlock_mvpn(stream_id_type stream_id, MVPN_type mpvn);
		bool Is_lpa_locked(stream_id_type stream_id, LPA_type lpa);
		bool Is_mvpn_locked(stream_id_type stream_id, MVPN_type mvpn);
	private:
		bool query_cmt(NVM_Transaction_Flash* transaction);
		void prepare_mapping_table();
		PPA_type online_create_entry_for_reads(LPA_type lpa, const stream_id_type stream_id, NVM::FlashMemory::Physical_Page_Address& read_address, uint64_t read_sectors_bitmap);
	};
}

#endif // !ADDRESS_MAPPING_UNIT_HYBRID_H
