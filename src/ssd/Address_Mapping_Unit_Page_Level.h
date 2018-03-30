#ifndef ADDRESS_MAPPING_UNIT_PAGE_LEVEL
#define ADDRESS_MAPPING_UNIT_PAGE_LEVEL

#include <unordered_map>
#include <map>
#include <queue>
#include <set>
#include <list>
#include "Address_Mapping_Unit_Base.h"
#include "Flash_Block_Manager_Base.h"
#include "SSD_Defs.h"
#include "NVM_Transaction_Flash_RD.h"
#include "NVM_Transaction_Flash_WR.h"

namespace SSD_Components
{
#define MAKE_TABLE_INDEX(LPN,STREAM)

	enum class CMTEntryStatus {FREE, WAITING, VALID};

	struct GTDEntryType //Entry type for the Global Translation Directory
	{
		MPPN_type MPPN;
		data_timestamp_type TimeStamp;
	};
	struct CMTSlotType
	{
		PPA_type PPA;
		unsigned long long WrittenStateBitmap;
		bool Dirty;
		CMTEntryStatus Status;
		std::list<std::pair<LPA_type, CMTSlotType*>>::iterator listPtr;//used for fast implementation of LRU
	};
	struct GMTEntryType//Entry type for the Global Mapping Table
	{
		PPA_type PPA;
		uint64_t WrittenStateBitmap;
		data_timestamp_type TimeStamp;
	};
	
	class Cached_Mapping_Table
	{
	public:
		Cached_Mapping_Table(unsigned int capacity);
		bool Exists(const stream_id_type streamID, const LPA_type lpa);
		PPA_type Retrieve_ppa(const stream_id_type streamID, const LPA_type lpa);
		void Update_mapping_info(const stream_id_type streamID, const LPA_type lpa, const PPA_type ppa, const page_status_type pageWriteState);
		void Insert_new_mapping_info(const stream_id_type streamID, const LPA_type lpa, const PPA_type ppa, const unsigned long long pageWriteState);
		page_status_type Get_bitmap_vector_of_written_sectors(const stream_id_type streamID, const LPA_type lpa);

		bool Is_slot_reserved_for_lpn_and_waiting(const stream_id_type streamID, const LPA_type lpa);
		bool Check_free_slot_availability();
		void Reserve_slot_for_lpn(const stream_id_type streamID, const LPA_type lpa);
		CMTSlotType Evict_one_slot(LPA_type& lpa);
		
		bool Is_dirty(const stream_id_type streamID, const LPA_type lpa);
		void Make_clean(const stream_id_type streamID, const LPA_type lpa);
	private:
		std::unordered_map<LPA_type, CMTSlotType*> addressMap;
		std::list<std::pair<LPA_type, CMTSlotType*>> lruList;
		unsigned int capacity;
	};

	/* Each stream has its own address mapping domain. It helps isolation of GC interference
	* (e.g., multi-streamed SSD HotStorage 2014, and OPS isolation in FAST 2015)
	* However, CMT is shared among concurrent streams in two ways: 1) each address mapping domain
	* shares the whole CMT space with other domains, and 2) each address mapping domain has
	* its own share of CMT (equal partitioning of CMT space among concurrent streams).*/
	class AddressMappingDomain
	{
	public:
		AddressMappingDomain(unsigned int cmt_capacity, unsigned int cmt_entry_size,  unsigned int no_of_translation_entries_per_page,
			Cached_Mapping_Table* CMT, 
			Flash_Plane_Allocation_Scheme_Type PlaneAllocationScheme,
			flash_channel_ID_type* ChannelIDs, unsigned int ChannelNo, flash_chip_ID_type* ChipIDs, unsigned int ChipNo,
			flash_die_ID_type* DieIDs, unsigned int DieNo, flash_plane_ID_type* PlaneIDs, unsigned int PlaneNo,
			double Share_per_plane,
			unsigned int Block_no_per_plane, unsigned int Page_no_per_block, unsigned int Sectors_no_per_page,
			double Overprovisioning_ratio);


		/*Stores the mapping of Virtual Translation Page Number (MVPN) to Physical Translation Page Number (MPPN).
		* It is always kept in volatile memory.*/
		GTDEntryType* GlobalTranslationDirectory;

		/*The cached mapping table that is implemented based on the DFLT (Gupta et al., ASPLOS 2009) proposal.
		* It is always stored in volatile memory.*/
		unsigned int CMT_entry_size;
		unsigned int Translation_entries_per_page;
		Cached_Mapping_Table* CMT;


		/*The logical to physical address mapping of all data pages that is implemented based on the DFTL (Gupta et al., ASPLOS 2009(
		* proposal. It is always stored in non-volatile flash memory.*/
		GMTEntryType* GlobalMappingTable;
		void Update_mapping_info(const bool ideal_mapping, const stream_id_type stream_id, const LPA_type lpa, const PPA_type ppa, const page_status_type page_status_bitmap);
		page_status_type Get_page_status(const bool ideal_mapping, const stream_id_type stream_id, const LPA_type lpa);
		PPA_type Get_ppa(const bool ideal_mapping, const stream_id_type stream_id, const LPA_type lpa);
		bool Mapping_entry_accessible(const bool ideal_mapping, const stream_id_type stream_id, const LPA_type lpa);

		
		std::multimap<LPA_type, NVM_Transaction_Flash*> Waiting_unmapped_read_transactions;
		std::multimap<LPA_type, NVM_Transaction_Flash*> Waiting_unmapped_program_transactions;
		std::multimap<MVPN_type, LPA_type> ArrivingMappingEntries;
		std::set<MVPN_type> DepartingMappingEntries;
		std::set<LPA_type> Locked_LPAs;//Used to manage race conditions, i.e. a user request accesses and LPA while GC is moving that LPA 
		std::set<MVPN_type> Locked_MVPNs;//Used to manage race conditions

		Flash_Plane_Allocation_Scheme_Type PlaneAllocationScheme;
		flash_channel_ID_type* ChannelIDs;
		unsigned int ChannelNo;
		flash_chip_ID_type* ChipIDs;
		unsigned int ChipNo;
		flash_die_ID_type* DieIDs;
		unsigned int DieNo;
		flash_plane_ID_type* PlaneIDs;
		unsigned int PlaneNo;

		LSA_type max_logical_sector_address;
		unsigned int Total_logical_pages_no;
		unsigned int Pages_no_per_channel; //Number of pages in a channel
		unsigned int Pages_no_per_chip;
		unsigned int Pages_no_per_die;
		unsigned int Pages_no_per_plane;
		unsigned int Total_plane_no;
		unsigned int Total_physical_pages_no;
		unsigned int Total_translation_pages_no;

		double Share_per_plane;
		unsigned int Block_no_per_plane;
		unsigned int Page_no_per_block;
		unsigned int Sectors_no_per_page;
		double Overprovisioning_ratio;
		
	};

	class Address_Mapping_Unit_Page_Level : public Address_Mapping_Unit_Base
	{
		friend class GC_and_WL_Unit_Page_Level;
	public:
		Address_Mapping_Unit_Page_Level(const sim_object_id_type& id, FTL* ftl, NVM_PHY_ONFI* flash_controller, Flash_Block_Manager_Base* BlockManager,
			bool ideal_mapping_table, unsigned int cmt_capacity_in_byte, Flash_Plane_Allocation_Scheme_Type PlaneAllocationScheme,
			unsigned int ConcurrentStreamNo,
			unsigned int ChannelCount, unsigned int ChipNoPerChannel, unsigned int DieNoPerChip, unsigned int PlaneNoPerDie,
			std::vector<std::vector<flash_channel_ID_type>> stream_channel_ids, std::vector<std::vector<flash_chip_ID_type>> stream_chip_ids,
			std::vector<std::vector<flash_die_ID_type>> stream_die_ids, std::vector<std::vector<flash_plane_ID_type>> stream_plane_ids,
			unsigned int Block_no_per_plane, unsigned int Page_no_per_block, unsigned int SectorsPerPage, unsigned int PageSizeInBytes,
			double Overprovisioning_ratio, CMT_Sharing_Mode SharingMode = CMT_Sharing_Mode::SHARED, bool fold_large_addresses = true);
		void Setup_triggers();
		void Start_simulation();
		void Validate_simulation_config();
		void Execute_simulator_event(MQSimEngine::Sim_Event*);

		void Translate_lpa_to_ppa_and_dispatch(const std::list<NVM_Transaction*>& transactionList);
		void Get_data_mapping_info_for_gc(const stream_id_type stream_id, const LPA_type lpa, PPA_type& ppa, page_status_type& page_state);
		void Get_translation_mapping_info_for_gc(const stream_id_type stream_id, const MVPN_type mvpn, MPPN_type& mppa, sim_time_type& timestamp);
		bool Check_address_range(const stream_id_type streamID, const LPA_type lsn, const unsigned int size);
		void Allocate_new_page_for_gc(NVM_Transaction_Flash_WR* transaction, bool is_translation_page);

		LSA_type Get_logical_sectors_count_allocated_to_stream(stream_id_type stream_id);
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
		static Address_Mapping_Unit_Page_Level* _my_instance;
		CMT_Sharing_Mode SharingMode;
		unsigned int cmt_capacity;
		AddressMappingDomain** domains;
		unsigned int CMT_entry_size, GTD_entry_size;//In CMT MQSim stores (lpn, ppn, page status bits) but in GTD it only stores (ppn, page status bits)
		void allocate_plane_for_user_write(NVM_Transaction_Flash_WR* transaction);
		void allocate_page_in_plane_for_user_write(NVM_Transaction_Flash_WR* transaction, bool is_for_gc);
		void allocate_plane_for_translation_write(NVM_Transaction_Flash* transaction);
		void allocate_page_in_plane_for_translation_write(NVM_Transaction_Flash* transaction, MVPN_type mvpn, bool is_for_gc);
		bool request_mapping_entry_for_lpn(const stream_id_type streamID, const LPA_type lpn);
		static void handle_transaction_serviced_signal_from_PHY(NVM_Transaction_Flash* transaction);
		void translate_lpa_to_ppa(stream_id_type streamID, NVM_Transaction_Flash* transaction);

		void generate_flash_read_request_for_mapping_data(const stream_id_type streamID, const LPA_type lpn);
		void generate_flash_writeback_request_for_mapping_data(const stream_id_type streamID, const LPA_type lpn);

		unsigned int no_of_translation_entries_per_page;
		MVPN_type get_MVPN(const LPA_type lpn, stream_id_type stream_id);
		LPA_type get_start_LPN_MVP(const MVPN_type);
		LPA_type get_end_LPN_in_MVP(const MVPN_type);

		bool check_and_translate(NVM_Transaction_Flash* transaction);
		void prepare_mapping_table();
		PPA_type online_create_entry_for_reads(LPA_type lpa, const stream_id_type stream_id, NVM::FlashMemory::Physical_Page_Address& read_address, uint64_t read_sectors_bitmap);
		void manage_transaction_with_locked_lpa(NVM_Transaction_Flash* transaction);
	};

}

#endif // !ADDRESS_MAPPING_UNIT_PAGE_LEVEL