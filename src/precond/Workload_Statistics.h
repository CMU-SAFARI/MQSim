#ifndef WORKLOAD_STATISTICS_H
#define WORKLOAD_STATISTICS_H

#include <map>
#include <vector>
#include <set>
#include <cstdint>
#include "../sim/Sim_Defs.h"
#include "../ssd/SSD_Defs.h"
#include "../utils/DistributionTypes.h"

namespace Preconditioning
{
#define MAX_SIZE_HISTOGRAM 1024
#define MAX_ARRIVAL_TIME_HISTOGRAM (1000000)
#define MAX_OCCUPANCY 0.80

	//Parameters defined in: B. Van Houdt, "On the necessity of hot and cold data identification to reduce the write amplification in flash-based SSDs", Perf. Eval., 2014.
	struct Workload_Statistics
	{
		Utils::Workload_Type Type;
		stream_id_type Stream_id;
		double Occupancy;//Ratio of the logical storage space that is fill with data in steady-state
		unsigned int Replay_no;
		unsigned int Total_generated_reqeusts;

		int random_request_type_generator_seed;
		double Read_ratio;
		unsigned int Request_queue_depth;
		std::vector<sim_time_type> Write_arrival_time, Read_arrival_time;//Histogram with 1us resolution

		Utils::Address_Distribution_Type Address_distribution_type;
		unsigned int Total_accessed_lbas;
		double Hot_region_ratio;
		int random_address_generator_seed;
		int random_hot_cold_generator_seed;
		int random_hot_address_generator_seed;
		std::map<LSA_type, unsigned int> Write_address_access_pattern, Read_address_access_pattern;
		std::set<LSA_type> Write_read_shared_addresses;
		LSA_type First_Accessed_Address, Last_Accessed_Address, Smallest_Accessed_Address, Largest_Accessed_Address;


		Utils::Address_Distribution_Type address_distribution;
		LSA_type hot_region_end_lsa;
		LSA_type streaming_next_address;
		int random_time_interval_generator_seed;


		Utils::Request_Size_Distribution_Type Request_size_distribution_type;
		int random_request_size_generator_seed;
		unsigned int Average_request_size;
		unsigned int STDEV_reuqest_size;
		std::vector<unsigned int> Write_size_histogram, Read_size_histogram;//Histogram with 1 sector resolution
	};
}
#endif// !WORKLOAD_STATISTICS_H