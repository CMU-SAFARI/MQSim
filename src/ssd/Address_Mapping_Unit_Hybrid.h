#ifndef ADDRESS_MAPPING_UNIT_HYBRID_H
#define ADDRESS_MAPPING_UNIT_HYBRID_H

#include "Address_Mapping_Unit_Base.h"

namespace SSD_Components
{
	class Address_Mapping_Unit_Hybrid : public Address_Mapping_Unit_Base
	{
	public:
		Address_Mapping_Unit_Hybrid(sim_object_id_type id, FTL* ftl, NVM_PHY_ONFI* flash_controller, Flash_Block_Manager_Base* BlockManager,
			unsigned int ConcurrentStreamNo,
			unsigned int ChannelCount, unsigned int ChipNoPerChannel, unsigned int DieNoPerChip, unsigned int PlaneNoPerDie,
			unsigned int Block_no_per_plane, unsigned int Page_no_per_block, unsigned int SectorsPerPage, unsigned int PageSizeInBytes,
			double Overprovisioning_ratio, bool fold_large_addresses = true);
		void Setup_triggers();
		void Start_simulation();
		void Validate_simulation_config();
		void Execute_simulator_event(MQSimEngine::Sim_Event*);

		void Translate_lpa_to_ppa_and_dispatch(const std::list<NVM_Transaction*>& transactionList);
		bool Get_bitmap_vector_of_written_sectors_of_lpn(const stream_id_type streamID, const LPA_type lpn, page_status_type& pageState);
		bool Check_address_range(const stream_id_type streamID, const LPA_type lsn, const unsigned int size);

		PPA_type Online_create_entry_for_reads(LPA_type lpa, const stream_id_type stream_id, NVM::FlashMemory::Physical_Page_Address& read_address, uint64_t read_sectors_bitmap);
	private:
		bool check_and_translate(NVM_Transaction_Flash* transaction);
		NVM::FlashMemory::Physical_Page_Address convert_ppa_to_address(const PPA_type ppn);
		void convert_ppa_to_address(const PPA_type ppn, NVM::FlashMemory::Physical_Page_Address& address);
		PPA_type convert_ppa_to_address(const NVM::FlashMemory::Physical_Page_Address& pageAddress);
		void prepare_mapping_table();
	};
}

#endif // !ADDRESS_MAPPING_UNIT_HYBRID_H
