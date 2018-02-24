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

	class Address_Mapping_Unit_Base : public MQSimEngine::Sim_Object
	{
	public:
		Address_Mapping_Unit_Base(const sim_object_id_type& id, FTL* ftl, NVM_PHY_ONFI* FlashController, Flash_Block_Manager_Base* BlockManager,
			unsigned int input_stream_no,
			unsigned int ChannelCount, unsigned int ChipNoPerChannel, unsigned int DieNoPerChip, unsigned int PlaneNoPerDie,
			unsigned int Block_no_per_plane, unsigned int Page_no_per_block, unsigned int SectorsPerPage, unsigned int PageSizeInBytes,
			double Overprovisioning_ratio, bool fold_large_addresses = true);

		virtual void Translate_lpa_to_ppa_and_dispatch(const std::list<NVM_Transaction*>& transactionList) = 0;
		virtual bool Get_bitmap_vector_of_written_sectors_of_lpn(const stream_id_type streamID, const LPA_type lpn, page_status_type& pageState) = 0;
		virtual bool Check_address_range(const stream_id_type streamID, const LPA_type lsn, const unsigned int size) = 0;

		/* Used in the standalone execution mode in which the HostInterface first preprocesses
		*  the input trace/synthetic requests and asks for creating a mapping entry for each
		*  read LPA that is not preceded by a write.
		*/
		virtual PPA_type Online_create_entry_for_reads(LPA_type lpa, const stream_id_type stream_id, NVM::FlashMemory::Physical_Page_Address& read_address, uint64_t read_sectors_bitmap) = 0;
		LSA_type Get_max_logical_sector_address();
	protected:
		FTL* ftl;
		NVM_PHY_ONFI* flash_controller;
		Flash_Block_Manager_Base* BlockManager;
		unsigned int input_stream_no;
		LSA_type max_logical_sector_address;
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
		bool lpnTablePopulated;

		virtual bool check_and_translate(NVM_Transaction_Flash* transaction) = 0;
		virtual NVM::FlashMemory::Physical_Page_Address convert_ppa_to_address(const PPA_type ppn) = 0;
		virtual void convert_ppa_to_address(const PPA_type ppn, NVM::FlashMemory::Physical_Page_Address& address) = 0;
		virtual PPA_type convert_ppa_to_address(const NVM::FlashMemory::Physical_Page_Address& pageAddress) = 0;
		virtual void prepare_mapping_table() = 0;
	};
}

#endif // !ADDRESS_MAPPING_UNIT_BASE_H
