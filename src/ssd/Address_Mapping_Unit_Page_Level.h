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
	enum class CMTEntryStatus {FREE, WAITING, VALID};

	typedef uint32_t MVPN_type;
	typedef uint32_t MPPN_type;
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
		PPA_type Rerieve_ppa(const stream_id_type streamID, const LPA_type lpa);
		void Update_mapping_info(const stream_id_type streamID, const LPA_type lpa, const PPA_type ppa, const page_status_type pageWriteState);
		void Insert_new_mapping_info(const stream_id_type streamID, const LPA_type lpa, const PPA_type ppa, const unsigned long long pageWriteState);
		page_status_type Get_bitmap_vector_of_written_sectors(const stream_id_type streamID, const LPA_type lpa);
		
		bool Is_slot_reserved_for_lpn_and_waiting(const stream_id_type streamID, const LPA_type lpa);
		bool Check_free_slot_availability();
		void Reserve_slot_for_lpn(const stream_id_type streamID, const LPA_type lpa);
		CMTSlotType Evict_one_slot();
		
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

		
		std::multimap<LPA_type, NVM_Transaction_Flash*> Waiting_unmapped_read_transactions;
		std::multimap<LPA_type, NVM_Transaction_Flash*> Waiting_unmapped_program_transactions;
		std::multimap<MVPN_type, LPA_type> ArrivingMappingEntries;
		std::set<MVPN_type> DepartingMappingEntries;

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

		unsigned int STAT_CMT_hits, STAT_readTR_CMT_hits, STAT_writeTR_CMT_hits;
		unsigned int STAT_CMT_miss, STAT_readTR_CMT_miss, STAT_writeTR_CMT_miss;
		unsigned int STAT_total_CMT_queries, STAT_total_readTR_CMT_queries, STAT_total_writeTR_CMT_queries;
	};

	class Address_Mapping_Unit_Page_Level : public Address_Mapping_Unit_Base
	{
	public:
		Address_Mapping_Unit_Page_Level(const sim_object_id_type& id, FTL* ftl, NVM_PHY_ONFI* flash_controller, Flash_Block_Manager_Base* BlockManager,
			unsigned int cmt_capacity_in_byte, Flash_Plane_Allocation_Scheme_Type PlaneAllocationScheme,
			unsigned int ConcurrentStreamNo,
			unsigned int ChannelCount, unsigned int ChipNoPerChannel, unsigned int DieNoPerChip, unsigned int PlaneNoPerDie,
			unsigned int Block_no_per_plane, unsigned int Page_no_per_block, unsigned int SectorsPerPage, unsigned int PageSizeInBytes,
			double Overprovisioning_ratio, CMT_Sharing_Mode SharingMode = CMT_Sharing_Mode::SHARED, bool fold_large_addresses = true);
		void Setup_triggers();
		void Start_simulation();
		void Validate_simulation_config();
		void Execute_simulator_event(MQSimEngine::Sim_Event*);

		void Translate_lpa_to_ppa_and_dispatch(const std::list<NVM_Transaction*>& transactionList);
		bool Get_bitmap_vector_of_written_sectors_of_lpn(const stream_id_type streamID, const LPA_type lpn, page_status_type& pageState);
		bool Check_address_range(const stream_id_type streamID, const LPA_type lsn, const unsigned int size);

		PPA_type Online_create_entry_for_reads(LPA_type lpa, const stream_id_type stream_id, NVM::FlashMemory::Physical_Page_Address& read_address, uint64_t read_sectors_bitmap);
	private:
		static Address_Mapping_Unit_Page_Level* _myInstance;
		CMT_Sharing_Mode SharingMode;
		unsigned int cmt_capacity;
		AddressMappingDomain** domains;
		unsigned int CMT_entry_size, GTD_entry_size;//In CMT MQSim stores (lpn, ppn, page status bits) but in GTD it only stores (ppn, page status bits)
		void allocate_plane_for_user_write(NVM_Transaction_Flash_WR* transaction);
		void allocate_page_in_plane_for_user_write(NVM_Transaction_Flash_WR* transaction);
		void allocate_plane_for_translation_write(NVM_Transaction_Flash* transaction);
		void allocate_page_in_plane_for_translation_write(NVM_Transaction_Flash* transaction, MVPN_type mvpn);
		bool request_mapping_entry_for_lpn(const stream_id_type streamID, const LPA_type lpn);
		static void handle_transaction_serviced_signal(NVM_Transaction_Flash* transaction);
		void translate_lpa_to_ppa(stream_id_type streamID, NVM_Transaction_Flash* transaction);

		void generate_flash_read_request_for_mapping_data(const stream_id_type streamID, const LPA_type lpn);
		void generate_flash_writeback_request_for_mapping_data(const stream_id_type streamID, const LPA_type lpn);

		unsigned int no_of_translation_entries_per_page;
		MVPN_type get_MVPN(const LPA_type lpn, stream_id_type stream_id);
		LPA_type get_start_LPN_MVP(const MVPN_type);
		LPA_type get_end_LPN_in_MVP(const MVPN_type);

		bool check_and_translate(NVM_Transaction_Flash* transaction);
		NVM::FlashMemory::Physical_Page_Address convert_ppa_to_address(const PPA_type ppn);
		void convert_ppa_to_address(const PPA_type ppn, NVM::FlashMemory::Physical_Page_Address& address);
		PPA_type convert_ppa_to_address(const NVM::FlashMemory::Physical_Page_Address& pageAddress);
		void prepare_mapping_table();
	};

}

#endif // !ADDRESS_MAPPING_UNIT_PAGE_LEVEL