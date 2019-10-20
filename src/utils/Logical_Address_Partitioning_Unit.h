#ifndef LOGICAL_ADDRESS_PARTITIONING_UNIT_H
#define LOGICAL_ADDRESS_PARTITIONING_UNIT_H

#include <vector>
#include "../ssd/SSD_Defs.h"
#include "../ssd/Host_Interface_Defs.h"

/* MQSim requires the logical address space of the SSD device to be partitioned among the concurrent flows.
In fact, two different storage traces may access the same logical address, but this logical address
should not be assumed to be identical when the traces are executed together.
*/

namespace Utils
{
	class Logical_Address_Partitioning_Unit
	{
	public:
		static void Reset();
		static void Allocate_logical_address_for_flows(HostInterface_Types hostinterface_type, unsigned int concurrent_stream_no,
			unsigned int channel_count, unsigned int chip_no_per_channel, unsigned int die_no_per_chip, unsigned int plane_no_per_die,
			std::vector<std::vector<flash_channel_ID_type>> stream_channel_ids, std::vector<std::vector<flash_chip_ID_type>> stream_chip_ids,
			std::vector<std::vector<flash_die_ID_type>> stream_die_ids, std::vector<std::vector<flash_plane_ID_type>> stream_plane_ids,
			unsigned int block_no_per_plane, unsigned int page_no_per_block, unsigned int sector_no_per_page, double overprovisioning_ratio);
		static LHA_type Start_lha_available_to_flow(stream_id_type stream_id);
		static LHA_type End_lha_available_to_flow(stream_id_type stream_id);
		static LHA_type LHA_count_allocate_to_flow_from_host_view(stream_id_type stream_id);
		static LHA_type LHA_count_allocate_to_flow_from_device_view(stream_id_type stream_id);
		static PDA_type PDA_count_allocate_to_flow(stream_id_type stream_id);
		static double Get_share_of_physcial_pages_in_plane(flash_channel_ID_type channel_id, flash_chip_ID_type chip_id, flash_die_ID_type die_id, flash_plane_ID_type plane_id);
		static LHA_type Get_total_device_lha_count();
	private:
		static HostInterface_Types hostinterface_type;
		static int****resource_list;
		static std::vector<std::vector<flash_channel_ID_type>> stream_channel_ids;
		static std::vector<std::vector<flash_chip_ID_type>> stream_chip_ids;
		static std::vector<std::vector<flash_die_ID_type>> stream_die_ids;
		static std::vector<std::vector<flash_plane_ID_type>> stream_plane_ids;
		static bool initialized;
		static std::vector<LHA_type> pdas_per_flow;
		static std::vector<LHA_type> start_lhas_per_flow;
		static std::vector<LHA_type> end_lhas_per_flow;
		static LHA_type total_pda_no;
		static LHA_type total_lha_no;
		static unsigned int channel_count;
		static unsigned int chip_no_per_channel;
		static unsigned int die_no_per_chip;
		static unsigned int plane_no_per_die;
	};
}

#endif // !LOGICAL_ADDRESS_PARTITIONING_UNIT_H
