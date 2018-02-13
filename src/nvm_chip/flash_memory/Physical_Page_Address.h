#ifndef PHYSICAL_PAGE_ADDRESS_H
#define PHYSICAL_PAGE_ADDRESS_H

#include "FlashTypes.h"

namespace NVM
{
	namespace FlashMemory
	{
		class Physical_Page_Address
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

			Physical_Page_Address(flash_channel_ID_type channelID = 0, flash_chip_ID_type localFlashChipID = 0, flash_die_ID_type dieID = 0,
				flash_plane_ID_type planeID = 0, flash_block_ID_type blockID = 0, flash_page_ID_type pageID = 0)
			{
				ChannelID = channelID;
				ChipID = localFlashChipID;
				DieID = dieID;
				PlaneID = planeID;
				BlockID = blockID;
				PageID = pageID;
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

			static void SetBlockAddressConstraint(bool BAConstraint)
			{
				block_address_constraint_for_multiplane = BAConstraint;
			}
		};
	}
}
#endif // !PageAddress_H
