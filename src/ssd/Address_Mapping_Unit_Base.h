#ifndef ADDRESS_MAPPING_UNIT_BASE_H
#define ADDRESS_MAPPING_UNIT_BASE_H

#include "../sim/Sim_Object.h"
#include "../nvm_chip/flash_memory/Physical_Page_Address.h"
#include "../nvm_chip/flash_memory/FlashTypes.h"
#include "SSD_Defs.h"
#include "NVM_Transaction_Flash.h"
#include "NVM_PHY_ONFI_NVDDR2.h"
#include "FTL.h"
#include "Flash_Block_Manager_Base.h"

namespace SSD_Components
{
	class FTL;
	class Flash_Block_Manager_Base;

	typedef uint32_t MVPN_type;
	typedef uint32_t MPPN_type;

	enum class Flash_Address_Mapping_Type {PAGE_LEVEL, HYBRID};
	enum class Flash_Plane_Allocation_Scheme_Type
	{
		CWDP, CWPD, CDWP, CDPW, CPWD, CPDW,
		WCDP, WCPD, WDCP, WDPC, WPCD, WPDC,
		DCWP, DCPW, DWCP, DWPC, DPCW, DPWC,
		PCWD, PCDW, PWCD, PWDC, PDCW, PDWC,
		F//Fully dynamic
	};
	enum class CMT_Sharing_Mode { SHARED, EQUAL_SIZE_PARTITIONING };

	enum class Moving_LPA_Status { GC_IS_READING_PHYSICAL_BLOCK, GC_IS_READING_DATA, GC_IS_WRITING_DATA, 
		GC_IS_READING_PHYSICAL_BLOCK_AND_THERE_IS_USER_READ, GC_IS_READING_DATA_AND_THERE_IS_USER_READ,
	    GC_IS_READING_PHYSICAL_BLOCK_AND_PAGE_IS_INVALIDATED, GC_IS_READING_DATA_AND_PAGE_IS_INVALIDATED, GC_IS_WRITING_DATA_AND_PAGE_IS_INVALIDATED};

	class Address_Mapping_Unit_Base : public MQSimEngine::Sim_Object
	{
	public:
		Address_Mapping_Unit_Base(const sim_object_id_type& id, FTL* ftl, NVM_PHY_ONFI* FlashController, Flash_Block_Manager_Base* BlockManager,
			bool ideal_mapping_table, unsigned int no_of_input_streams,
			unsigned int ChannelCount, unsigned int chip_no_per_channel, unsigned int DieNoPerChip, unsigned int PlaneNoPerDie,
			unsigned int Block_no_per_plane, unsigned int Page_no_per_block, unsigned int SectorsPerPage, unsigned int PageSizeInBytes,
			double Overprovisioning_ratio, CMT_Sharing_Mode sharing_mode = CMT_Sharing_Mode::SHARED, bool fold_large_addresses = true);
		virtual ~Address_Mapping_Unit_Base();

		virtual void Allocate_address_for_preconditioning(const stream_id_type stream_id, const std::map<LPA_type, page_status_type>& lpa_list, const std::vector<double>& steady_state_distribution) = 0;
		virtual int Bring_to_CMT_for_preconditioning(stream_id_type stream_id, LPA_type lpa) = 0;
		virtual unsigned int Get_cmt_capacity() = 0;
		virtual unsigned int Get_current_cmt_occupancy_for_stream(stream_id_type stream_id) = 0;
		virtual void Translate_lpa_to_ppa_and_dispatch(const std::list<NVM_Transaction*>& transactionList) = 0;
		virtual void Get_data_mapping_info_for_gc(const stream_id_type stream_id, const LPA_type lpa, PPA_type& ppa, page_status_type& page_state) = 0;
		virtual void Get_translation_mapping_info_for_gc(const stream_id_type stream_id, const MVPN_type mvpn, MPPN_type& mppa, sim_time_type& timestamp) = 0;
		virtual void Allocate_new_page_for_gc(NVM_Transaction_Flash_WR* transaction, bool is_translation_page) = 0;
		unsigned int Get_no_of_input_streams() { return no_of_input_streams; }
		unsigned int Get_device_physical_pages_count();
		bool Is_ideal_mapping_table();
		CMT_Sharing_Mode Get_CMT_sharing_mode();

		virtual void Store_mapping_table_on_flash_at_start() = 0;
		virtual LHA_type Get_logical_sectors_count(stream_id_type stream_id) = 0;
		virtual LPA_type Get_logical_pages_count(stream_id_type stream_id) = 0;
		virtual PPA_type Get_physical_pages_count(stream_id_type stream_id) = 0;
		virtual NVM::FlashMemory::Physical_Page_Address Convert_ppa_to_address(const PPA_type ppa) = 0;
		virtual void Convert_ppa_to_address(const PPA_type ppa, NVM::FlashMemory::Physical_Page_Address& address) = 0;
		virtual PPA_type Convert_address_to_ppa(const NVM::FlashMemory::Physical_Page_Address& pageAddress) = 0;
		virtual void Lock_physical_block_for_gc(const NVM::FlashMemory::Physical_Page_Address& block_address) = 0;
		virtual void Lock_lpa_for_gc(const stream_id_type stream_id, const LPA_type lpa) = 0;
		virtual void Lock_mvpn_for_gc(const stream_id_type stream_id, const MVPN_type mvpn) = 0;
		virtual void Unlock_lpa_after_gc(const stream_id_type stream_id, const LPA_type lpa) = 0;
		virtual void Unlock_mvpn_after_gc(const stream_id_type stream_id, const MVPN_type mvpn) = 0;
	protected:
		FTL* ftl;
		NVM_PHY_ONFI* flash_controller;
		Flash_Block_Manager_Base* BlockManager;
		CMT_Sharing_Mode sharing_mode;
		bool ideal_mapping_table;//If mapping is ideal, then all the mapping entries are found in the DRAM and there is no need to read mapping entries from flash
		unsigned int no_of_input_streams;
		LHA_type max_logical_sector_address;
		unsigned int total_logical_pages_no = 0;

		unsigned int channel_count;
		unsigned int chip_no_per_channel;
		unsigned int die_no_per_chip;
		unsigned int plane_no_per_die;
		unsigned int block_no_per_plane;
		unsigned int pages_no_per_block;
		unsigned int sector_no_per_page;
		unsigned int page_size_in_byte;
		unsigned int total_physical_pages_no = 0;
		unsigned int page_no_per_channel = 0;
		unsigned int page_no_per_chip = 0;
		unsigned int page_no_per_die = 0;
		unsigned int page_no_per_plane = 0;
		double overprovisioning_ratio;
		bool fold_large_addresses;
		bool mapping_table_stored_on_flash;

		virtual bool query_cmt(NVM_Transaction_Flash* transaction) = 0;
		virtual PPA_type online_create_entry_for_reads(LPA_type lpa, const stream_id_type stream_id, NVM::FlashMemory::Physical_Page_Address& read_address, uint64_t read_sectors_bitmap) = 0;
		virtual void manage_user_transaction_with_locked_lpa(NVM_Transaction_Flash* transaction) = 0;
		virtual void manage_mapping_transaction_with_locked_mvpn(stream_id_type stream_id, MVPN_type mvpn, bool read) = 0;
		virtual bool is_lpa_locked_for_gc(stream_id_type stream_id, LPA_type lpa) = 0;
		virtual bool is_mvpn_locked_for_gc(stream_id_type stream_id, MVPN_type mvpn) = 0;
	};
}

#endif // !ADDRESS_MAPPING_UNIT_BASE_H
