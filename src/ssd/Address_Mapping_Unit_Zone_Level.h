#ifndef ADDRESS_MAPPING_UNIT_ZONE_LEVEL
#define ADDRESS_MAPPING_UNIT_ZONE_LEVEL

#include <unordered_map>
#include <map>
#include <queue>
#include <set>
#include <list>
#include "Address_Mapping_Unit_Base.h"
#include "Address_Mapping_Unit_Page_Level.h"
#include "Flash_Block_Manager_Base.h"
#include "SSD_Defs.h"
#include "NVM_Transaction_Flash_RD.h"
#include "NVM_Transaction_Flash_WR.h"

namespace SSD_Components
{
	
	class Address_Mapping_Unit_Zone_Level : public Address_Mapping_Unit_Base
	{
		friend class GC_and_WL_Unit_Page_Level;
		friend class GC_and_WL_Unit_Zone_Level;
		
	public:
		Address_Mapping_Unit_Zone_Level(const sim_object_id_type& id, FTL* ftl, 
			NVM_PHY_ONFI* flash_controller, Flash_Block_Manager_Base* block_manager, Flash_Zone_Manager_Base* zone_manager, bool Support_Zone, 
			bool ideal_mapping_table, unsigned int cmt_capacity_in_byte, 
			Flash_Plane_Allocation_Scheme_Type PlaneAllocationScheme,
			Zone_Allocation_Scheme_Type ZoneAllocationScheme,
			SubZone_Allocation_Scheme_Type SubZoneAllocationScheme,
			unsigned int ConcurrentStreamNo,
			unsigned int ChannelCount, unsigned int chip_no_per_channel, 
			unsigned int DieNoPerChip, unsigned int PlaneNoPerDie,
			unsigned int ChannelNoPerZone, unsigned int ChipNoPerZone,
			unsigned int DieNoPerZone, unsigned int PlaneNoPerZone,
			std::vector<std::vector<flash_channel_ID_type>> stream_channel_ids, 
			std::vector<std::vector<flash_chip_ID_type>> stream_chip_ids,
			std::vector<std::vector<flash_die_ID_type>> stream_die_ids, 
			std::vector<std::vector<flash_plane_ID_type>> stream_plane_ids,
			unsigned int Block_no_per_plane, unsigned int Page_no_per_block, 
			unsigned int SectorsPerPage, unsigned int PageSizeInBytes,
			double Overprovisioning_ratio, CMT_Sharing_Mode sharing_mode = CMT_Sharing_Mode::SHARED, 
			bool fold_large_addresses = true);
		~Address_Mapping_Unit_Zone_Level();
		
		void Setup_triggers();
		void Start_simulation();
		void Validate_simulation_config();
		void Execute_simulator_event(MQSimEngine::Sim_Event*);

		Zone_ID_type translate_lpa_to_zoneID_for_gc(std::list<NVM_Transaction*> transaction_list);
		Zone_ID_type get_zone_block_list(std::list<NVM_Transaction*> transaction_list, std::list<NVM::FlashMemory::Physical_Page_Address*> &list);
		void Translate_lpa_to_ppa_and_dispatch(const std::list<NVM_Transaction*>& transactionList);

		void Allocate_address_for_preconditioning(const stream_id_type stream_id, std::map<LPA_type, page_status_type>& lpa_list, std::vector<double>& steady_state_distribution);
		int Bring_to_CMT_for_preconditioning(stream_id_type stream_id, LPA_type lpa);
		void Store_mapping_table_on_flash_at_start();
		unsigned int Get_cmt_capacity();//Returns the maximum number of entries that could be stored in the cached mapping table
		unsigned int Get_current_cmt_occupancy_for_stream(stream_id_type stream_id);
		LPA_type Get_logical_pages_count(stream_id_type stream_id); //Returns the number of logical pages allocated to an I/O stream

		void Set_barrier_for_accessing_physical_block(const NVM::FlashMemory::Physical_Page_Address& block_address);
		void Set_barrier_for_accessing_lpa(stream_id_type stream_id, LPA_type lpa);
		void Set_barrier_for_accessing_mvpn(stream_id_type stream_id, MVPN_type mpvn);
		void Remove_barrier_for_accessing_lpa(stream_id_type stream_id, LPA_type lpa);
		void Remove_barrier_for_accessing_mvpn(stream_id_type stream_id, MVPN_type mpvn);
		void Start_servicing_writes_for_overfull_plane(const NVM::FlashMemory::Physical_Page_Address plane_address);

		void Get_data_mapping_info_for_gc(const stream_id_type stream_id, const LPA_type lpa, PPA_type& ppa, page_status_type& page_state);
		void Get_translation_mapping_info_for_gc(const stream_id_type stream_id, const MVPN_type mvpn, MPPN_type& mppa, sim_time_type& timestamp);
		void Allocate_new_page_for_gc(NVM_Transaction_Flash_WR* transaction, bool is_translation_page);
		NVM::FlashMemory::Physical_Page_Address Convert_ppa_to_address(const PPA_type ppa);
		void Convert_ppa_to_address(const PPA_type ppa, NVM::FlashMemory::Physical_Page_Address& address);
		PPA_type Convert_address_to_ppa(const NVM::FlashMemory::Physical_Page_Address& pageAddress);

	//private:
		static Address_Mapping_Unit_Zone_Level* _my_instance;
		AddressMappingDomain** domains;
		Flash_Zone_Manager_Base *fzm;
		NVM::FlashMemory::Zone **zones;

		unsigned int CMT_entry_size, GTD_entry_size;
		unsigned int cmt_capacity;
		unsigned int no_of_translation_entries_per_page;
		std::set<NVM_Transaction_Flash_WR*>**** Write_transactions_for_overfull_planes;
		void allocate_plane_for_user_write(NVM_Transaction_Flash_WR* transaction);
		void allocate_page_in_plane_for_user_write(NVM_Transaction_Flash_WR* transaction, bool is_for_gc);
		void allocate_plane_for_translation_write(NVM_Transaction_Flash* transaction);
		void allocate_page_in_plane_for_translation_write(NVM_Transaction_Flash* transaction, MVPN_type mvpn, bool is_for_gc);
		void allocate_plane_for_preconditioning(stream_id_type stream_id, LPA_type lpn, NVM::FlashMemory::Physical_Page_Address& targetAddress);
		bool query_cmt(NVM_Transaction_Flash* transaction);
		bool translate_lpa_to_ppa(stream_id_type streamID, NVM_Transaction_Flash* transaction);
		bool request_mapping_entry(const stream_id_type streamID, const LPA_type lpn);
		static void handle_transaction_serviced_signal_from_PHY(NVM_Transaction_Flash* transaction);
		
		MVPN_type get_MVPN(const LPA_type lpn, stream_id_type stream_id);
		LPA_type get_start_LPN_in_MVP(const MVPN_type);
		LPA_type get_end_LPN_in_MVP(const MVPN_type);

		void generate_flash_read_request_for_mapping_data(const stream_id_type streamID, const LPA_type lpn);
		void generate_flash_writeback_request_for_mapping_data(const stream_id_type streamID, const LPA_type lpn);
		
		void mange_unsuccessful_translation(NVM_Transaction_Flash* transaction);
		void manage_mapping_transaction_facing_barrier(stream_id_type stream_id, MVPN_type mvpn, bool read);
		void manage_user_transaction_facing_barrier(NVM_Transaction_Flash* transaction);
		bool is_mvpn_locked_for_gc(stream_id_type stream_id, MVPN_type mvpn);
		bool is_lpa_locked_for_gc(stream_id_type stream_id, LPA_type lpa);
		PPA_type online_create_entry_for_reads(LPA_type lpa, const stream_id_type stream_id, NVM::FlashMemory::Physical_Page_Address& read_address, uint64_t read_sectors_bitmap);

	};

}

#endif // !ADDRESS_MAPPING_UNIT_ZONE_LEVEL
