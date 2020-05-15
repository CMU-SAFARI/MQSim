#ifndef WORKLOAD_STATISTICS_H
#define WORKLOAD_STATISTICS_H

#include <map>
#include <vector>
#include <set>
#include <cstdint>
#include "../sim/Sim_Defs.h"
#include "../ssd/SSD_Defs.h"
#include "../utils/DistributionTypes.h"

namespace Utils
{
#define MAX_REQSIZE_HISTOGRAM_ITEMS 1024
#define MAX_ARRIVAL_TIME_HISTOGRAM (1000000)
#define MIN_HOT_REGION_TRAFFIC_RATIO 0.5
#define HOT_REGION_METRIC 2.5
#define STATISTICALLY_SUFFICIENT_WRITES_FOR_PRECONDITIONING 10000

	struct Address_Histogram_Unit
	{
		int Access_count;
		uint64_t Accessed_sub_units;
	};

	//Parameters defined in: B. Van Houdt, "On the necessity of hot and cold data identification to reduce the write amplification in flash-based SSDs", Perf. Eval., 2014.
	struct Workload_Statistics
	{
		Utils::Workload_Type Type;
		stream_id_type Stream_id;
		double Initial_occupancy_ratio;//Ratio of the logical storage space that is fill with data in steady-state
		unsigned int Replay_no;
		unsigned int Total_generated_requests;

		int random_request_type_generator_seed;
		double Read_ratio;
		std::vector<sim_time_type> Write_arrival_time, Read_arrival_time;//Histogram with 1us resolution

		Utils::Address_Distribution_Type Address_distribution_type;
		double Working_set_ratio;
		unsigned int Total_accessed_lbas;
		/*Rosenblum hot/cold model: 
		- M. Rosenblum and J. K. Ousterhout, "The design and implementation of a log-structured file system", ACM CSUR, 1992.
		A fraction f of the complete address space corresponds to hot data and the remaining fraction to cold data. The fraction
		of write operations to the hot data is denoted as r*/
		double Ratio_of_hot_addresses_to_whole_working_set;//The f parameter in the Rosenblum hot/cold model
		double Ratio_of_traffic_accessing_hot_region;//The r parameter in the Rosenblum hot/cold model
		int random_address_generator_seed;
		int random_hot_cold_generator_seed;
		int random_hot_address_generator_seed;
		std::map<LPA_type, Address_Histogram_Unit> Write_address_access_pattern, Read_address_access_pattern;
		std::set<LPA_type> Write_read_shared_addresses;
		LHA_type First_Accessed_Address, Last_Accessed_Address, Min_LHA, Max_LHA;
		LHA_type hot_region_end_lsa;
		LHA_type streaming_next_address;
		bool generate_aligned_addresses;
		unsigned int alignment_value;;

		Utils::Request_Generator_Type generator_type;
		unsigned int Request_queue_depth;
		int random_time_interval_generator_seed;
		sim_time_type Average_inter_arrival_time_nano_sec;

		Utils::Request_Size_Distribution_Type Request_size_distribution_type;
		int random_request_size_generator_seed;
		unsigned int Average_request_size_sector;
		unsigned int STDEV_reuqest_size;
		std::vector<unsigned int> Write_size_histogram, Read_size_histogram;//Histogram with 1 sector resolution
	};
}

#endif// !WORKLOAD_STATISTICS_H
