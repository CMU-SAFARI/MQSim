#ifndef PHYSICAL_PAGE_ADDRESS_H
#define PHYSICAL_PAGE_ADDRESS_H

#include "../NVM_Memory_Address.h"
#include "FlashTypes.h"

namespace NVM
{
	namespace FlashMemory
	{
		class Physical_Page_Address : public NVM_Memory_Address
		{
		private:
			static bool block_address_constraint_for_multiplane;//Block address of the commands to neighbor planes must be identical for multiplane command execution
		public:
			flash_channel_ID_type ChannelID;
			flash_chip_ID_type ChipID;        //The flashchip ID inside its channel
			flash_die_ID_type DieID;
			flash_plane_ID_type PlaneID;
			flash_block_ID_type BlockID;
			flash_page_ID_type PageID;

			Physical_Page_Address(const flash_channel_ID_type channel_id = 0, const flash_chip_ID_type chip_id = 0, const flash_die_ID_type die_id = 0,
				const flash_plane_ID_type plane_id = 0, const flash_block_ID_type block_id = 0, const flash_page_ID_type page_id = 0)
			{
				ChannelID = channel_id;
				ChipID = chip_id;
				DieID = die_id;
				PlaneID = plane_id;
				BlockID = block_id;
				PageID = page_id;
			}

			Physical_Page_Address(const Physical_Page_Address& addressToCopy)
			{
				ChannelID = addressToCopy.ChannelID;
				ChipID = addressToCopy.ChipID;
				DieID = addressToCopy.DieID;
				PlaneID = addressToCopy.PlaneID;
				BlockID = addressToCopy.BlockID;
				PageID = addressToCopy.PageID;
			}

			static void SetBlockAddressConstraint(const bool BAConstraint)
			{
				block_address_constraint_for_multiplane = BAConstraint;
			}
		};
	}
}
#endif // !PageAddress_H
