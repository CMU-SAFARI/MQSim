#include "Address_Mapping_Unit_Hybrid.h"

namespace SSD_Components
{
	Address_Mapping_Unit_Hybrid::Address_Mapping_Unit_Hybrid(sim_object_id_type id, FTL* ftl, NVM_PHY_ONFI* flash_controller, Flash_Block_Manager_Base* block_manager,
		bool ideal_mapping_table, unsigned int concurrent_streams_no,
		unsigned int channel_count, unsigned int chip_no_per_channel, unsigned int die_no_per_chip, unsigned int plane_no_in_die,
		unsigned int block_no_per_plane, unsigned int page_no_per_block, unsigned int sectors_per_page, unsigned int page_size_in_byte,
		double overprovisioning_ratio, bool fold_out_of_range_addresses) : 
		Address_Mapping_Unit_Base(id, ftl, flash_controller, block_manager, ideal_mapping_table,
			concurrent_streams_no, channel_count, chip_no_per_channel, die_no_per_chip, plane_no_in_die,
			block_no_per_plane, page_no_per_block, sectors_per_page, page_size_in_byte, overprovisioning_ratio, fold_out_of_range_addresses) {}
	void Address_Mapping_Unit_Hybrid::Setup_triggers() {}
	void Address_Mapping_Unit_Hybrid::Start_simulation() {}
	void Address_Mapping_Unit_Hybrid::Validate_simulation_config() {}
	void Address_Mapping_Unit_Hybrid::Execute_simulator_event(MQSimEngine::Sim_Event* event) {}

	void Address_Mapping_Unit_Hybrid::Allocate_address_or_preconditioning(const stream_id_type stream_id, const std::vector<LPA_type> lpa_list, const std::vector<unsigned int> size, std::vector<NVM::FlashMemory::Physical_Page_Address>& address) {}
	void Address_Mapping_Unit_Hybrid::Translate_lpa_to_ppa_and_dispatch(const std::list<NVM_Transaction*>& transaction_list) {}
	void Address_Mapping_Unit_Hybrid::Get_data_mapping_info_for_gc(const stream_id_type stream_id, const LPA_type lpa, PPA_type& ppa, page_status_type& page_state) {}
	void Address_Mapping_Unit_Hybrid::Get_translation_mapping_info_for_gc(const stream_id_type stream_id, const MVPN_type mvpn, MPPN_type& mppa, sim_time_type& timestamp) {}
	bool Address_Mapping_Unit_Hybrid::Check_address_range(const stream_id_type streamID, const LPA_type lsn, const unsigned int size) { return false; }

	PPA_type Address_Mapping_Unit_Hybrid::online_create_entry_for_reads(LPA_type lpa, const stream_id_type stream_id, NVM::FlashMemory::Physical_Page_Address& read_address, uint64_t read_sectors_bitmap) { return 0; }

	bool Address_Mapping_Unit_Hybrid::query_cmt(NVM_Transaction_Flash* transaction) { return true; }
	NVM::FlashMemory::Physical_Page_Address Address_Mapping_Unit_Hybrid::Convert_ppa_to_address(const PPA_type ppa)
	{
		NVM::FlashMemory::Physical_Page_Address pa;
		return pa;
	}
	LSA_type Address_Mapping_Unit_Hybrid::Get_logical_sectors_count_allocated_to_stream(stream_id_type stream_id)
	{
		return 0;
	}
	void Address_Mapping_Unit_Hybrid::Convert_ppa_to_address(const PPA_type ppa, NVM::FlashMemory::Physical_Page_Address& address) {}
	PPA_type Address_Mapping_Unit_Hybrid::Convert_address_to_ppa(const NVM::FlashMemory::Physical_Page_Address& pageAddress) { return 0; }
	void Address_Mapping_Unit_Hybrid::prepare_mapping_table() {}
	void Address_Mapping_Unit_Hybrid::Allocate_new_page_for_gc(NVM_Transaction_Flash_WR* transaction, bool is_translation_page) {}
	void Address_Mapping_Unit_Hybrid::Lock_lpa(stream_id_type stream_id, LPA_type lpa) {}
	void Address_Mapping_Unit_Hybrid::Unlock_lpa(stream_id_type stream_id, LPA_type lpa) {}
	void Address_Mapping_Unit_Hybrid::Lock_mvpn(stream_id_type stream_id, MVPN_type mpvn) {}
	void Address_Mapping_Unit_Hybrid::Unlock_mvpn(stream_id_type stream_id, MVPN_type mpvn) {}
	bool Address_Mapping_Unit_Hybrid::Is_lpa_locked(stream_id_type stream_id, LPA_type lpa) { return false; }
	bool Address_Mapping_Unit_Hybrid::Is_mvpn_locked(stream_id_type stream_id, MVPN_type mvpn) { return false; }
}