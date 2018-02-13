#include "GC_and_WL_Unit_Page_Level.h"
#include "FTL.h"

namespace SSD_Components
{
	GC_and_WL_Unit_Page_Level::GC_and_WL_Unit_Page_Level(FTL* ftl, Flash_Block_Manager_Base* BlockManager, double GCThreshold,
		GC_Block_Selection_Policy_Type BlockSelectionPolicy,
		bool PreemptibleGCEnabled, double GCHardThreshold,
		unsigned int ChannelCount, unsigned int ChipNoPerChannel, unsigned int DieNoPerChip, unsigned int PlaneNoPerDie,
		unsigned int Block_no_per_plane, unsigned int Page_no_per_block, unsigned int SectorsPerPage)
		: GC_and_WL_Unit_Base(ftl, BlockManager, GCThreshold, PreemptibleGCEnabled, GCHardThreshold,
		ChannelCount, ChipNoPerChannel, DieNoPerChip, PlaneNoPerDie, Block_no_per_plane, Page_no_per_block, SectorsPerPage),
		blockSelectionPolicy(BlockSelectionPolicy)
	{}

	void GC_and_WL_Unit_Page_Level::Setup_triggers()
	{
		flash_controller->ConnectToTransactionServicedSignal(handle_transaction_serviced_signal_from_PHY);
	}

	bool GC_and_WL_Unit_Page_Level::GC_is_in_urgent_mode(const NVM::FlashMemory::Chip* chip)
	{
		NVM::FlashMemory::Physical_Page_Address addr;
		addr.ChannelID = chip->ChannelID; addr.ChipID = chip->ChipID;
		for (unsigned int die_id = 0; die_id < die_no_per_chip; die_id++)
			for (unsigned int plane_id = 0; plane_id < plane_no_per_die; plane_id++)
			{
				addr.DieID = die_id; addr.PlaneID = plane_id;
				if (block_manager->Get_pool_size(addr) < gc_hard_threshold)
					return true;
			}
		return false;
	}

	void GC_and_WL_Unit_Page_Level::CheckGCRequired(const unsigned int BlockPoolSize, const NVM::FlashMemory::Physical_Page_Address& planeAddress)
	{
	}

	void GC_and_WL_Unit_Page_Level::CheckWLRequired(const double staticWLFactor, const NVM::FlashMemory::Physical_Page_Address planeAddress)
	{}

	void GC_and_WL_Unit_Page_Level::handle_transaction_serviced_signal_from_PHY(NVM_Transaction_Flash* transaction)
	{
	}

}