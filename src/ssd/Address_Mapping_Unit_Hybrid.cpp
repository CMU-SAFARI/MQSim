#include "Address_Mapping_Unit_Hybrid.h"

namespace SSD_Components
{
	Address_Mapping_Unit_Hybrid::Address_Mapping_Unit_Hybrid(sim_object_id_type id, FTL* ftl, NVM_PHY_ONFI* flash_controller, Flash_Block_Manager_Base* block_manager,
		unsigned int concurrent_streams_no,
		unsigned int channel_count, unsigned int chip_no_per_channel, unsigned int die_no_per_chip, unsigned int plane_no_in_die,
		unsigned int block_no_per_plane, unsigned int page_no_per_block, unsigned int sectors_per_page, unsigned int page_size_in_byte,
		double overprovisioning_ratio, bool fold_out_of_range_addresses) : 
		Address_Mapping_Unit_Base(id, ftl, flash_controller, block_manager,
			concurrent_streams_no, channel_count, chip_no_per_channel, die_no_per_chip, plane_no_in_die,
			block_no_per_plane, page_no_per_block, sectors_per_page, page_size_in_byte, overprovisioning_ratio, fold_out_of_range_addresses) {}
	void Address_Mapping_Unit_Hybrid::Setup_triggers() {}
	void Address_Mapping_Unit_Hybrid::Start_simulation() {}
	void Address_Mapping_Unit_Hybrid::Validate_simulation_config() {}
	void Address_Mapping_Unit_Hybrid::Execute_simulator_event(MQSimEngine::Sim_Event* event) {}

	void Address_Mapping_Unit_Hybrid::Translate_lpa_to_ppa_and_dispatch(const std::list<NVM_Transaction*>& transaction_list) {}
	bool Address_Mapping_Unit_Hybrid::Get_bitmap_vector_of_written_sectors_of_lpn(const stream_id_type streamID, const LPA_type lpn, page_status_type& pageState) { return false; }
	bool Address_Mapping_Unit_Hybrid::Check_address_range(const stream_id_type streamID, const LPA_type lsn, const unsigned int size) { return false; }

	PPA_type Address_Mapping_Unit_Hybrid::Online_create_entry_for_reads(LPA_type lpa, const stream_id_type stream_id, NVM::FlashMemory::Physical_Page_Address& read_address, uint64_t read_sectors_bitmap) { return 0; }

	bool Address_Mapping_Unit_Hybrid::check_and_translate(NVM_Transaction_Flash* transaction) { return true; }
	NVM::FlashMemory::Physical_Page_Address Address_Mapping_Unit_Hybrid::convert_ppa_to_address(const PPA_type ppn) \
	{
		NVM::FlashMemory::Physical_Page_Address pa;
		return pa;
	}
	void Address_Mapping_Unit_Hybrid::convert_ppa_to_address(const PPA_type ppn, NVM::FlashMemory::Physical_Page_Address& address) {}
	PPA_type Address_Mapping_Unit_Hybrid::convert_ppa_to_address(const NVM::FlashMemory::Physical_Page_Address& pageAddress) { return 0; }
	void Address_Mapping_Unit_Hybrid::prepare_mapping_table() {}
}