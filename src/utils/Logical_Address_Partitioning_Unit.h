#ifndef LOGICAL_ADDRESS_PARTITIONING_UNIT_H
#define LOGICAL_ADDRESS_PARTITIONING_UNIT_H

#include <vector>
#include "../ssd/SSD_Defs.h"

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
		static void Allocate_logical_address_for_flows(unsigned int concurrent_stream_no,
			unsigned int channel_count, unsigned int chip_no_per_channel, unsigned int die_no_per_chip, unsigned int plane_no_per_die,
			std::vector<std::vector<flash_channel_ID_type>> stream_channel_ids, std::vector<std::vector<flash_chip_ID_type>> stream_chip_ids,
			std::vector<std::vector<flash_die_ID_type>> stream_die_ids, std::vector<std::vector<flash_plane_ID_type>> stream_plane_ids,
			unsigned int block_no_per_plane, unsigned int page_no_per_block, unsigned int sector_no_per_page, double overprovisioning_ratio);
		//static LHA_type LHA_available_to_flow(unsigned int total_stream_count, stream_id_type stream_id);
		static LHA_type Start_lha_available_to_flow(stream_id_type stream_id);
		static LHA_type End_lha_available_to_flow(stream_id_type stream_id);
		static LHA_type LHA_count_allocate_to_flow(stream_id_type stream_id);
		static PDA_type PDA_count_allocate_to_flow(stream_id_type stream_id);
	private:
		static bool initialized;
		static std::vector<LHA_type> pdas_per_flow;
		static std::vector<LHA_type> start_lhas_per_flow;
		static std::vector<LHA_type> end_lhas_per_flow;
	};
}
#endif // !LOGICAL_ADDRESS_PARTITIONING_UNIT_H
