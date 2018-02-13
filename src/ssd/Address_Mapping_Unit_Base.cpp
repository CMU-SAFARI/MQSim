#include "FTL.h"
#include "Address_Mapping_Unit_Base.h"
#include "NVM_PHY_ONFI_NVDDR2.h"
#include "Flash_Block_Manager_Base.h"

namespace SSD_Components
{

	Address_Mapping_Unit_Base::Address_Mapping_Unit_Base(const sim_object_id_type& id, FTL* ftl, NVM_PHY_ONFI* flash_controller, Flash_Block_Manager_Base* BlockManager,
		unsigned int input_stream_no,
		unsigned int ChannelCount, unsigned int ChipNoPerChannel, unsigned int DieNoPerChip, unsigned int PlaneNoPerDie,
		unsigned int Block_no_per_plane, unsigned int Page_no_per_block, unsigned int SectorsPerPage, unsigned int PageSizeInBytes,
		double Overprovisioning_ratio, bool fold_large_addresses)
		: Sim_Object(id), ftl(ftl), flash_controller(flash_controller), BlockManager(BlockManager),
		input_stream_no(input_stream_no),
		channel_count(ChannelCount), chip_no_per_channel(ChipNoPerChannel), die_no_per_chip(DieNoPerChip), plane_no_per_die(PlaneNoPerDie),
		block_no_per_plane(Block_no_per_plane), pages_no_per_block(Page_no_per_block), sector_no_per_page(SectorsPerPage), page_size_in_byte(PageSizeInBytes), 
		overprovisioning_ratio(Overprovisioning_ratio), fold_large_addresses(fold_large_addresses),
		lpnTablePopulated(false)
	{
		page_no_per_plane = pages_no_per_block * block_no_per_plane;
		page_no_per_die = page_no_per_plane * plane_no_per_die;
		page_no_per_chip = page_no_per_die * die_no_per_chip;
		page_no_per_channel = page_no_per_chip * chip_no_per_channel;
		total_physical_pages_no = page_no_per_channel * ChannelCount;
		total_logical_pages_no = (unsigned int)((double)total_physical_pages_no * (1 - overprovisioning_ratio));
		max_logical_sector_address = (LSA_type)(SectorsPerPage * total_logical_pages_no);
	}

	LSA_type Address_Mapping_Unit_Base::Get_max_logical_sector_address()
	{
		return this->max_logical_sector_address;
	}
}