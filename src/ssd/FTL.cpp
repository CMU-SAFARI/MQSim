#include <stdexcept>
#include <cmath>
#include <vector>
#include <random>
#include <map>
#include <functional>
#include <iterator>
#include "../sim/Sim_Defs.h"
#include "../utils/DistributionTypes.h"
#include "../utils/Helper_Functions.h"
#include "FTL.h"
#include "Stats.h"

namespace SSD_Components
{
	FTL::FTL(const sim_object_id_type& id, Data_Cache_Manager_Base* data_cache_manager,
		unsigned int channel_no, unsigned int chip_no_per_channel, unsigned int die_no_per_chip, unsigned int plane_no_per_die,
		unsigned int block_no_per_plane, unsigned int page_no_per_block, unsigned int page_size_in_sectors, 
		sim_time_type avg_flash_read_latency, sim_time_type avg_flash_program_latency, 
		double over_provisioning_ratio, unsigned int max_allowed_block_erase_count, int seed) :
		NVM_Firmware(id, data_cache_manager), random_generator(seed),
		channel_no(channel_no), chip_no_per_channel(chip_no_per_channel), die_no_per_chip(die_no_per_chip), plane_no_per_die(plane_no_per_die),
		block_no_per_plane(block_no_per_plane), page_no_per_block(page_no_per_block), page_size_in_sectors(page_size_in_sectors), 
		avg_flash_read_latency(avg_flash_read_latency), avg_flash_program_latency(avg_flash_program_latency),
		over_provisioning_ratio(over_provisioning_ratio), max_allowed_block_erase_count(max_allowed_block_erase_count)
	{
		Stats::Init_stats(channel_no, chip_no_per_channel, die_no_per_chip, plane_no_per_die, block_no_per_plane, page_no_per_block, max_allowed_block_erase_count);
	}

	FTL::~FTL()
	{
		Stats::Clear_stats(channel_no, chip_no_per_channel, die_no_per_chip, plane_no_per_die, block_no_per_plane, page_no_per_block, max_allowed_block_erase_count);
	}

	void FTL::Validate_simulation_config()
	{
		NVM_Firmware::Validate_simulation_config();
		if (this->Data_cache_manager == NULL)
			throw std::logic_error("The cache manager is not set for FTL!");
		if (this->Address_Mapping_Unit == NULL)
			throw std::logic_error("The mapping module is not set for FTL!");
		if (this->BlockManager == NULL)
			throw std::logic_error("The block manager is not set for FTL!");
		if (this->GC_and_WL_Unit == NULL)
			throw std::logic_error("The garbage collector is not set for FTL!");
	}
	void FTL::Perform_precondition(std::vector<Utils::Workload_Statistics*> workload_stats)
	{
		Address_Mapping_Unit->Store_mapping_table_on_flash_at_start();

		double overall_rate = 0;
		for (auto const &stat : workload_stats)
		{
			if (stat->Type == Utils::Workload_Type::SYNTHETIC)
			{
				switch (stat->generator_type)
				{
				case Utils::Request_Generator_Type::BANDWIDTH:
					overall_rate += 1.0 / double(stat->Average_inter_arrival_time_nano_sec) * SIM_TIME_TO_SECONDS_COEFF * stat->Average_request_size_sector;
					break;
				case Utils::Request_Generator_Type::QUEUE_DEPTH:
				{
					sim_time_type max_arrival_time = sim_time_type(stat->Read_ratio * double(avg_flash_read_latency) + (1 - stat->Read_ratio) * double(avg_flash_program_latency));
					double avg_arrival_time = double(max_arrival_time) / double(stat->Request_queue_depth);
					overall_rate += 1.0 / avg_arrival_time * SIM_TIME_TO_SECONDS_COEFF * stat->Average_request_size_sector;
					break;
				}
				default:
					PRINT_ERROR("Unknown request type generator in the FTL preconditioning function.")
				}
			}
			else
			{
				overall_rate += 1.0 / double(stat->Average_inter_arrival_time_nano_sec) * SIM_TIME_TO_SECONDS_COEFF * stat->Average_request_size_sector;
			}
		}

		unsigned int total_accessed_cmt_entries = 0;

		for (auto &stat : workload_stats)
		{
			LPA_type no_of_logical_pages_in_steadystate = (LPA_type)(stat->Initial_occupancy_ratio * Address_Mapping_Unit->Get_logical_pages_count(stat->Stream_id));

			//Step 1: generate LPAs that are accessed in the steady-state
			Utils::Address_Distribution_Type decision_dist_type = stat->Address_distribution_type;
			std::map<LPA_type, page_status_type> lpa_set_for_preconditioning;//Stores the accessed LPAs
			std::multimap<int, LPA_type, std::greater<int>> trace_lpas_sorted_histogram;//only used for trace workloads
			unsigned int hot_region_last_index_in_histogram = 0;//only used for trace workloads to detect hot addresses
			LHA_type min_lha = stat->Min_LHA;
			LHA_type max_lha = stat->Max_LHA - 1;
			LPA_type min_lpa = Convert_host_logical_address_to_device_address(min_lha);
			if (stat->generate_aligned_addresses)
			{
				if (min_lha % stat->alignment_value != 0)
					min_lha += stat->alignment_value - (min_lha % stat->alignment_value);
				if (max_lha % stat->alignment_value != 0)
					max_lha -= min_lha % stat->alignment_value;
			}

			LPA_type max_lpa = Convert_host_logical_address_to_device_address(max_lha) - 1;
			total_accessed_cmt_entries += (unsigned int)(Convert_host_logical_address_to_device_address(max_lha) / page_size_in_sectors - Convert_host_logical_address_to_device_address(min_lha) / page_size_in_sectors) + 1;
			bool hot_range_finished = false;//Used for fast address generation in hot/cold traffic mode
			LHA_type hot_region_end_lsa = 0, hot_lha_used_for_generation = 0;//Used for fast address generation in hot/cold traffic mode
			LPA_type last_hot_lpa = 0;


			if (stat->Type == Utils::Workload_Type::SYNTHETIC)
			{
				bool is_read = false;
				unsigned int size = 0;
				LHA_type start_LBA = 0, streaming_next_address = 0;
				Utils::RandomGenerator* random_request_type_generator = new Utils::RandomGenerator(stat->random_request_type_generator_seed);
				Utils::RandomGenerator* random_address_generator = new Utils::RandomGenerator(stat->random_address_generator_seed);
				Utils::RandomGenerator* random_hot_address_generator = NULL;
				Utils::RandomGenerator* random_hot_cold_generator = NULL;
				Utils::RandomGenerator* random_request_size_generator = NULL;
				bool fully_include_hot_addresses = false;

				if (stat->Address_distribution_type == Utils::Address_Distribution_Type::RANDOM_HOTCOLD)//treat a workload with very low hot/cold values as a uniform random workload
					if (stat->Ratio_of_hot_addresses_to_whole_working_set > 0.3)
						decision_dist_type = Utils::Address_Distribution_Type::RANDOM_UNIFORM;


				//Preparing address generation parameters
				switch (decision_dist_type)
				{
				case Utils::Address_Distribution_Type::RANDOM_HOTCOLD:
				{
					random_hot_address_generator = new Utils::RandomGenerator(stat->random_hot_address_generator_seed);
					random_hot_cold_generator = new Utils::RandomGenerator(stat->random_hot_cold_generator_seed);
					hot_region_end_lsa = min_lha + (LHA_type)((double)(max_lha - min_lha) * stat->Ratio_of_hot_addresses_to_whole_working_set);
					last_hot_lpa = Convert_host_logical_address_to_device_address(hot_region_end_lsa) - 1;//Be conservative and not include the last_hot_address itself
					hot_lha_used_for_generation = min_lha;
					//Check if enough LPAs could be generated within the working set of the flow
					if ((last_hot_lpa - min_lpa) < no_of_logical_pages_in_steadystate)
						fully_include_hot_addresses = true;
					if ((max_lpa - last_hot_lpa) < 1.1 * (no_of_logical_pages_in_steadystate - (last_hot_lpa - min_lpa)))
					{
						PRINT_MESSAGE("The specified initial occupancy value could not be satisfied as the working set of workload #" << stat->Stream_id << " is small. MQSim made some adjustments!");
						max_lha = min_lha + LHA_type(double(max_lha - min_lha) / stat->Working_set_ratio);
						if (stat->generate_aligned_addresses)
							if (max_lha % stat->alignment_value != 0)
								max_lha -= min_lha % stat->alignment_value;
						
						max_lpa = Convert_host_logical_address_to_device_address(max_lha);
					}
					break;
				}
				case Utils::Address_Distribution_Type::STREAMING:
				{
					streaming_next_address = random_address_generator->Uniform_ulong(min_lha, max_lha);
					stat->First_Accessed_Address = streaming_next_address;
					//Check if enough LPAs could be generated within the working set of the flow
					if ((max_lpa - min_lpa) < no_of_logical_pages_in_steadystate)
					{
						PRINT_MESSAGE("The specified initial occupancy value could not be satisfied as the working set of workload #" << stat->Stream_id << " is small. MQSim made some adjustments!");
						max_lha = min_lha + LHA_type(double(max_lha - min_lha) / stat->Working_set_ratio);
						if (stat->generate_aligned_addresses)
							if (max_lha % stat->alignment_value != 0)
								max_lha -= min_lha % stat->alignment_value;
						max_lpa = Convert_host_logical_address_to_device_address(max_lha);

						if ((max_lpa - min_lpa) < no_of_logical_pages_in_steadystate)
							no_of_logical_pages_in_steadystate = max_lpa - min_lpa + 1;
					}
					break;
				}
				case Utils::Address_Distribution_Type::RANDOM_UNIFORM:
				{
					//Check if enough LPAs could be generated within the working set of the flow
					if ((max_lpa - min_lpa) < 1.1 * no_of_logical_pages_in_steadystate)
					{
						PRINT_MESSAGE("The specified initial occupancy value could not be satisfied as the working set of workload #" << stat->Stream_id << " is small. MQSim made some adjustments!");
						max_lha = min_lha + LHA_type(double(max_lha - min_lha) / stat->Working_set_ratio);
						if (stat->generate_aligned_addresses)
							if (max_lha % stat->alignment_value != 0)
								max_lha -= min_lha % stat->alignment_value;
						max_lpa = Convert_host_logical_address_to_device_address(max_lha);

						if ((max_lpa - min_lpa) < 1.1 * no_of_logical_pages_in_steadystate)
						{
							no_of_logical_pages_in_steadystate = (unsigned int)(double(max_lpa - min_lpa) * 0.9);
						}
					}
					break;
				}
				}

				if (stat->Request_size_distribution_type == Utils::Request_Size_Distribution_Type::NORMAL)
				{
					random_request_size_generator = new Utils::RandomGenerator(stat->random_request_size_generator_seed);
				}

				while (lpa_set_for_preconditioning.size() < no_of_logical_pages_in_steadystate)
				{
					if (random_request_type_generator->Uniform(0, 1) <= stat->Read_ratio)
						is_read = true;

					switch (stat->Request_size_distribution_type)
					{
					case Utils::Request_Size_Distribution_Type::FIXED:
						size = stat->Average_request_size_sector;
						break;
					case Utils::Request_Size_Distribution_Type::NORMAL:
					{
						double temp_request_size = random_request_size_generator->Normal(stat->Average_request_size_sector, stat->STDEV_reuqest_size);
						size = (unsigned int)(std::ceil(temp_request_size));
						if (size <= 0)
							size = 1;
						break;
					}
					}

					bool is_hot_address = false;

					switch (decision_dist_type)
					{
					case Utils::Address_Distribution_Type::STREAMING:
						start_LBA = streaming_next_address;
						if (start_LBA + size > max_lha)
							start_LBA = min_lha;
						streaming_next_address += size;
						if (streaming_next_address > max_lha)
							streaming_next_address = min_lha;
						if (stat->generate_aligned_addresses)
							if (streaming_next_address % stat->alignment_value != 0)
								streaming_next_address += stat->alignment_value - (streaming_next_address % stat->alignment_value);
						break;
					case Utils::Address_Distribution_Type::RANDOM_HOTCOLD:
					{
						if (fully_include_hot_addresses)//just to speedup address generation
						{
							if (!hot_range_finished)
							{
								start_LBA = hot_lha_used_for_generation;
								hot_lha_used_for_generation += size;
								if (start_LBA > hot_region_end_lsa)
									hot_range_finished = true;
								is_hot_address = true;
							}
							else
							{
								start_LBA = random_hot_address_generator->Uniform_ulong(hot_region_end_lsa + 1, max_lha);
								if (start_LBA < hot_region_end_lsa + 1 || start_LBA > max_lha)
									PRINT_ERROR("Out of range address is generated in IO_Flow_Synthetic!\n")
									if (start_LBA + size > max_lha)
										start_LBA = hot_region_end_lsa + 1;
								is_hot_address = false;
							}
						}							
						else
						{
							if (random_hot_cold_generator->Uniform(0, 1) < stat->Ratio_of_hot_addresses_to_whole_working_set)// (100-hot)% of requests going to hot% of the address space
							{
								start_LBA = random_hot_address_generator->Uniform_ulong(hot_region_end_lsa + 1, max_lha);
								if (start_LBA < hot_region_end_lsa + 1 || start_LBA > max_lha)
									PRINT_ERROR("Out of range address is generated in IO_Flow_Synthetic!\n")
									if (start_LBA + size > max_lha)
										start_LBA = hot_region_end_lsa + 1;
								is_hot_address = false;
							}
							else
							{
								start_LBA = random_hot_address_generator->Uniform_ulong(min_lha, hot_region_end_lsa);
								if (start_LBA < min_lha || start_LBA > hot_region_end_lsa)
									PRINT_ERROR("Out of range address is generated in IO_Flow_Synthetic!\n")
								is_hot_address = true;
							}
						}
					}
						break;
					case Utils::Address_Distribution_Type::RANDOM_UNIFORM:
						start_LBA = random_address_generator->Uniform_ulong(min_lha, max_lha);
						if (start_LBA < min_lha || max_lha < start_LBA)
							PRINT_ERROR("Out of range address is generated in IO_Flow_Synthetic!\n")
						if (start_LBA + size > max_lha)
							start_LBA = min_lha;
						break;
					}

					if (stat->generate_aligned_addresses)
						start_LBA -= start_LBA % stat->alignment_value;


					unsigned int handled_sectors_count = 0;
					LHA_type lsa = start_LBA - min_lha;
					unsigned int transaction_size = 0;
					page_status_type access_status_bitmap = 0;
					LPA_type max_lpa_within_device = Convert_host_logical_address_to_device_address(stat->Max_LHA) - Convert_host_logical_address_to_device_address(stat->Min_LHA);
					while (handled_sectors_count < size)
					{
						transaction_size = page_size_in_sectors - (unsigned int)(lsa % page_size_in_sectors);
						if (handled_sectors_count + transaction_size >= size)
						{
							transaction_size = size - handled_sectors_count;
						}
						LPA_type lpa = Convert_host_logical_address_to_device_address(lsa);
						page_status_type access_status_bitmap = Find_NVM_subunit_access_bitmap(lsa);

						lsa = lsa + transaction_size;
						handled_sectors_count += transaction_size;

						if (lpa_set_for_preconditioning.find(lpa) == lpa_set_for_preconditioning.end()) {
							lpa_set_for_preconditioning[lpa] = access_status_bitmap;
							//The lpas in trace_lpas_sorted_histogram are those that are actually accessed by the application
							if (lpa <= max_lpa_within_device) {
								if (!is_hot_address && hot_region_last_index_in_histogram == 0) {
									hot_region_last_index_in_histogram = (unsigned int)(trace_lpas_sorted_histogram.size());
								}
								std::pair<int, LPA_type> entry((is_hot_address ? 1000 : 1), lpa);
								trace_lpas_sorted_histogram.insert(entry);
							}
						} else {
							lpa_set_for_preconditioning[lpa] = access_status_bitmap | lpa_set_for_preconditioning[lpa];
						}
					}
				}
			} else {
				//Step 1-1: Read LPAs are preferred for steady-state since each read should be written before the actual access
				for (auto itr = stat->Write_read_shared_addresses.begin(); itr != stat->Write_read_shared_addresses.end(); itr++) {
					LPA_type lpa = (*itr);
					if (lpa_set_for_preconditioning.size() < no_of_logical_pages_in_steadystate) {
						lpa_set_for_preconditioning[lpa] = stat->Write_address_access_pattern[lpa].Accessed_sub_units | stat->Read_address_access_pattern[lpa].Accessed_sub_units;
					} else {
						break;
					}
				}

				for (auto itr = stat->Read_address_access_pattern.begin(); itr != stat->Read_address_access_pattern.end(); itr++) {
					LPA_type lpa = (*itr).first;
					if (lpa_set_for_preconditioning.size() < no_of_logical_pages_in_steadystate) {
						if (lpa_set_for_preconditioning.find(lpa) == lpa_set_for_preconditioning.end()) {
							lpa_set_for_preconditioning[lpa] = stat->Read_address_access_pattern[lpa].Accessed_sub_units;
						}
					}
					else {
						break;
					}
				}

				//Step 1-2: if the read LPAs are not enough for steady-state, then fill the lpa_set_for_preconditioning using write LPAs
				for (auto itr = stat->Write_address_access_pattern.begin(); itr != stat->Write_address_access_pattern.end(); itr++) {
					LPA_type lpa = (*itr).first;
					if (lpa_set_for_preconditioning.size() < no_of_logical_pages_in_steadystate) {
						if (lpa_set_for_preconditioning.find(lpa) == lpa_set_for_preconditioning.end()) {
							lpa_set_for_preconditioning[lpa] = stat->Write_address_access_pattern[lpa].Accessed_sub_units;
						}
					}
					std::pair<int, LPA_type> entry((*itr).second.Access_count, lpa);
					trace_lpas_sorted_histogram.insert(entry);
				}

				//Step 1-3: Determine the address distribution type of the input trace
				stat->Address_distribution_type = Utils::Address_Distribution_Type::RANDOM_HOTCOLD;//Initially assume that the trace has hot/cold access pattern
				
				//First check if there are enough number of write requests in the workload to make a statistically correct decision, if not, MQSim assumes the workload has a uniform access pattern
				if (stat->Write_address_access_pattern.size() > STATISTICALLY_SUFFICIENT_WRITES_FOR_PRECONDITIONING) {
					int hot_region_write_count = 0;
					int prev_value = (*trace_lpas_sorted_histogram.begin()).first;
					double f_temp = 0;
					double r_temp = 0;
					double step = 0.01;
					double next_milestone = 0.01;
					double prev_r = 0.0;
					for (auto itr = trace_lpas_sorted_histogram.begin(); itr != trace_lpas_sorted_histogram.end(); itr++) {
						hot_region_last_index_in_histogram++;
						hot_region_write_count += (*itr).first;

						f_temp = double(hot_region_last_index_in_histogram) / double(trace_lpas_sorted_histogram.size());
						r_temp = double(hot_region_write_count) / double(stat->Total_accessed_lbas);

						if (f_temp >= next_milestone) {
							if ((r_temp - prev_r) < step) {
								r_temp = prev_r;
								f_temp = next_milestone - step;
								break;
							}
							prev_r = r_temp;
							next_milestone += step;
						}
							
						prev_value = (*itr).first;
					}

					if ((r_temp > MIN_HOT_REGION_TRAFFIC_RATIO) && ((r_temp / f_temp) > HOT_REGION_METRIC)) {
						stat->Ratio_of_hot_addresses_to_whole_working_set = f_temp;
						stat->Ratio_of_traffic_accessing_hot_region = r_temp;
					} else {
						stat->Address_distribution_type = Utils::Address_Distribution_Type::RANDOM_UNIFORM;
					}
				} else {
					stat->Address_distribution_type = Utils::Address_Distribution_Type::RANDOM_UNIFORM;
				}

				Utils::RandomGenerator* random_address_generator = new Utils::RandomGenerator(preconditioning_seed++);
				unsigned int size = stat->Average_request_size_sector;
				LHA_type start_LHA = 0;
				
				//Step 1-4: If both read and write LPAs are not enough for preconditioning flash storage space, then fill the remaining space
				while (lpa_set_for_preconditioning.size() < no_of_logical_pages_in_steadystate) {
					start_LHA = random_address_generator->Uniform_ulong(min_lha, max_lha);
					unsigned int handled_sectors_count = 0;
					LHA_type lsa = start_LHA;
					unsigned int transaction_size = 0;
					while (handled_sectors_count < size) {
						if (lsa < min_lha || lsa > max_lha) {
							lsa = min_lha + (lsa % (max_lha - min_lha + 1));
						}
						LHA_type internal_lsa = lsa - min_lha;

						transaction_size = page_size_in_sectors - (unsigned int)(internal_lsa % page_size_in_sectors);
						if (handled_sectors_count + transaction_size >= size) {
							transaction_size = size - handled_sectors_count;
						}

						LPA_type lpa = Convert_host_logical_address_to_device_address(internal_lsa);
						page_status_type access_status_bitmap = Find_NVM_subunit_access_bitmap(internal_lsa);

						if (lpa_set_for_preconditioning.find(lpa) != lpa_set_for_preconditioning.end()) {
							lpa_set_for_preconditioning[lpa] = access_status_bitmap;
						} else {
							lpa_set_for_preconditioning[lpa] = access_status_bitmap | lpa_set_for_preconditioning[lpa];
						}

						lsa = lsa + transaction_size;
						handled_sectors_count += transaction_size;
					}
				}
			}//else of if (stat->Type == Utils::Workload_Type::SYNTHETIC)
			
			//Step 2: Determine the probability distribution function of valid pages in blocks, in the steady-state.
			//Note: if hot/cold separation is required, then the following estimations should be changed according to Van Houtd's paper in Performance Evaluation 2014.
			std::vector<double> steadystate_block_status_probability;//The probability distribution function of the number of valid pages in a block in the steadystate
			double rho = stat->Initial_occupancy_ratio * (1 - over_provisioning_ratio) / (1 - double(GC_and_WL_Unit->Get_minimum_number_of_free_pages_before_GC()) / block_no_per_plane);
			switch (decision_dist_type) {
			case Utils::Address_Distribution_Type::RANDOM_HOTCOLD://Estimate the steady-state of the hot/cold traffic based on the steady-state of the uniform traffic
			{
				double r_to_f_ratio = std::sqrt(double(stat->Ratio_of_traffic_accessing_hot_region) / double(stat->Ratio_of_hot_addresses_to_whole_working_set));
				switch (GC_and_WL_Unit->Get_gc_policy()) {
					case GC_Block_Selection_Policy_Type::GREEDY://Based on: B. Van Houdt, "A mean field model for a class of garbage collection algorithms in flash-based solid state drives", SIGMETRICS 2013.
					case GC_Block_Selection_Policy_Type::FIFO://Could be estimated with greedy for large page_no_per_block values, as mentioned in //Based on: B. Van Houdt, "A mean field model for a class of garbage collection algorithms in flash-based solid state drives", SIGMETRICS 2013.
					{
						for (unsigned int i = 0; i <= page_no_per_block; i++) {
							steadystate_block_status_probability.push_back(Utils::Combination_count(page_no_per_block, i) * std::pow(rho, i) * std::pow(1 - rho, page_no_per_block - i));
						}
						Utils::Euler_estimation(steadystate_block_status_probability, page_no_per_block, rho, 30, 0.001, 0.0000000001, 10000);//As specified in the SIGMETRICS 2013 paper, a larger value for d-choices (the name of RGA in Van Houdt's paper) will lead to results close to greedy. We use d=30 to estimate steady-state of the greedy policy with that of d-chioces.
						break;
					}
					case GC_Block_Selection_Policy_Type::RGA://Based on: B. Van Houdt, "A mean field model for a class of garbage collection algorithms in flash-based solid state drives", SIGMETRICS 2013.
					{
						for (unsigned int i = 0; i <= page_no_per_block; i++) {
							steadystate_block_status_probability.push_back(Utils::Combination_count(page_no_per_block, i) * std::pow(rho, i) * std::pow(1 - rho, page_no_per_block - i));
						}
						Utils::Euler_estimation(steadystate_block_status_probability, page_no_per_block, rho, GC_and_WL_Unit->Get_GC_policy_specific_parameter(), 0.001, 0.0000000001, 10000);
						break;
					}
					case GC_Block_Selection_Policy_Type::RANDOM:
					case GC_Block_Selection_Policy_Type::RANDOM_P://Based on: B. Van Houdt, "A mean field model for a class of garbage collection algorithms in flash-based solid state drives", SIGMETRICS 2013.
					{
						for (unsigned int i = 0; i <= page_no_per_block; i++) {
							steadystate_block_status_probability.push_back(rho / (rho + std::pow(1 - rho, i)));
							for (unsigned int j = i + 1; j <= page_no_per_block; j++) {
								steadystate_block_status_probability[i] *= ((1 - rho) * j) / (rho + (1 - rho) * j);
							}
						}
						for (int i = page_no_per_block; i > 0; i--) {
							steadystate_block_status_probability[i] = steadystate_block_status_probability[i] - steadystate_block_status_probability[i - 1];
						}
						break;
					}
					case GC_Block_Selection_Policy_Type::RANDOM_PP://Based on: B. Van Houdt, "A mean field model for a class of garbage collection algorithms in flash-based solid state drives", SIGMETRICS 2013.
					{
						//initialize the pdf values 
						for (unsigned int i = 0; i <= page_no_per_block; i++) {
							steadystate_block_status_probability.push_back(0);
						}

						double rho = stat->Initial_occupancy_ratio * (1 - over_provisioning_ratio);
						double S_rho_b = 0;
						for (unsigned int j = GC_and_WL_Unit->Get_GC_policy_specific_parameter() + 1; j <= page_no_per_block; j++) {
							S_rho_b += 1.0 / double(j);
						}
						double a_r = page_no_per_block - GC_and_WL_Unit->Get_GC_policy_specific_parameter() - page_no_per_block * S_rho_b;
						double b_r = rho * S_rho_b + 1 - rho;
						double c_r = -1 * rho / page_no_per_block;
						double mu_b = (-1 * b_r + std::sqrt(b_r * b_r - 4 * a_r * c_r)) / (2 * a_r);//assume always rho < 1 - 1/b
						for (int i = page_no_per_block; i >= 0; i--) {
							if (i <= int(GC_and_WL_Unit->Get_GC_policy_specific_parameter())) {
								steadystate_block_status_probability[i] = ((i + 1) * steadystate_block_status_probability[i + 1])
									/ (i + (rho / (1 - rho - mu_b * (page_no_per_block * S_rho_b - page_no_per_block + GC_and_WL_Unit->Get_GC_policy_specific_parameter()))));
							}
							else if (i < int(page_no_per_block)) {
								steadystate_block_status_probability[i] = double(page_no_per_block * mu_b) / double(i);
							} else {
								steadystate_block_status_probability[i] = mu_b;
							}
						}
						break;
					}
				}

				double average_page_no_per_block = 0;

				std::vector<double> steadystate_block_status_probability_temp;
				for (int i = 0; i < int(page_no_per_block) + 1; i++) {
					steadystate_block_status_probability_temp.push_back(steadystate_block_status_probability[i]);
					average_page_no_per_block += steadystate_block_status_probability[i] * i;
				}

				for (int i = 0; i < int(page_no_per_block) + 1; i++) {
					int phi_index = int((double(i) - average_page_no_per_block) * r_to_f_ratio);
					if (phi_index < 0 || phi_index >= int(page_no_per_block)) {
						steadystate_block_status_probability[i] = 0;
					}
					else {
						steadystate_block_status_probability[i] = r_to_f_ratio * steadystate_block_status_probability_temp[phi_index];
					}
				}
				double sum = 0;
				for (unsigned int i = 0; i <= page_no_per_block; i++) {
					sum += steadystate_block_status_probability[i];
				}
				for (unsigned int i = 0; i <= page_no_per_block; i++) {
					steadystate_block_status_probability[i] /= sum;
				}

				break;
			}
			case Utils::Address_Distribution_Type::STREAMING://A simple estimation based on: "Stochastic Modeling of Large-Scale Solid-State Storage Systems: Analysis, Design Tradeoffs and Optimization", SIGMETRICS 2013
				for (unsigned int i = 0; i <= page_no_per_block; i++) {
					if( i == 0) {
						steadystate_block_status_probability.push_back(1 - rho);
					} else if(i == page_no_per_block) {
						steadystate_block_status_probability.push_back(rho);
					} else {
						steadystate_block_status_probability.push_back(0);
					}
				}

				//None of the GC policies change the the status of blocks in the steady state assuming over-provisioning ratio is always greater than 0
				switch (GC_and_WL_Unit->Get_gc_policy()) {
					case GC_Block_Selection_Policy_Type::GREEDY:
					case GC_Block_Selection_Policy_Type::RANDOM_PP:
					case GC_Block_Selection_Policy_Type::RGA:
					case GC_Block_Selection_Policy_Type::FIFO:
					case GC_Block_Selection_Policy_Type::RANDOM:
					case GC_Block_Selection_Policy_Type::RANDOM_P:
						break;
				}
				break;
			case Utils::Address_Distribution_Type::RANDOM_UNIFORM:
			{
				switch (GC_and_WL_Unit->Get_gc_policy()) {
					case GC_Block_Selection_Policy_Type::GREEDY://Based on: B. Van Houdt, "A mean field model for a class of garbage collection algorithms in flash-based solid state drives", SIGMETRICS 2013.
					case GC_Block_Selection_Policy_Type::FIFO://Could be estimated with greedy for large page_no_per_block values, as mentioned in //Based on: B. Van Houdt, "A mean field model for a class of garbage collection algorithms in flash-based solid state drives", SIGMETRICS 2013.
					{
						for (unsigned int i = 0; i <= page_no_per_block; i++) {
							steadystate_block_status_probability.push_back(Utils::Combination_count(page_no_per_block, i) * std::pow(rho, i) * std::pow(1 - rho, page_no_per_block - i));
						}
						Utils::Euler_estimation(steadystate_block_status_probability, page_no_per_block, rho, 30, 0.001, 0.0000000001, 10000);//As specified in the SIGMETRICS 2013 paper, a larger value for d-choices (the name of RGA in Van Houdt's paper) will lead to results close to greedy. We use d=30 to estimate steady-state of the greedy policy with that of d-chioces.
						break;
					}
					case GC_Block_Selection_Policy_Type::RGA://Based on: B. Van Houdt, "A mean field model for a class of garbage collection algorithms in flash-based solid state drives", SIGMETRICS 2013.
					{
						for (unsigned int i = 0; i <= page_no_per_block; i++) {
							steadystate_block_status_probability.push_back(Utils::Combination_count(page_no_per_block, i) * std::pow(rho, i) * std::pow(1 - rho, page_no_per_block - i));
						}
						Utils::Euler_estimation(steadystate_block_status_probability, page_no_per_block, rho, GC_and_WL_Unit->Get_GC_policy_specific_parameter(), 0.001, 0.0000000001, 10000);
						break;
					}
					case GC_Block_Selection_Policy_Type::RANDOM:
					case GC_Block_Selection_Policy_Type::RANDOM_P://Based on: B. Van Houdt, "A mean field model for a class of garbage collection algorithms in flash-based solid state drives", SIGMETRICS 2013.
					{
						for (unsigned int i = 0; i <= page_no_per_block; i++) {
							steadystate_block_status_probability.push_back(rho / (rho + std::pow(1 - rho, i)));
							for (unsigned int j = i + 1; j <= page_no_per_block; j++) {
								steadystate_block_status_probability[i] *= ((1 - rho) * j) / (rho + (1 - rho) * j);
							}
						}
						for (int i = page_no_per_block; i > 0; i--) {
							steadystate_block_status_probability[i] = steadystate_block_status_probability[i] - steadystate_block_status_probability[i - 1];
						}
						break;
					}
					case GC_Block_Selection_Policy_Type::RANDOM_PP://Based on: B. Van Houdt, "A mean field model for a class of garbage collection algorithms in flash-based solid state drives", SIGMETRICS 2013.
					{
						//initialize the pdf values 
						for (unsigned int i = 0; i <= page_no_per_block; i++) {
							steadystate_block_status_probability.push_back(0);
						}
						
						double rho = stat->Initial_occupancy_ratio * (1 - over_provisioning_ratio);
						double S_rho_b = 0;
						for (unsigned int j = GC_and_WL_Unit->Get_GC_policy_specific_parameter() + 1; j <= page_no_per_block; j++) {
							S_rho_b += 1.0 / double(j);
						}
						double a_r = page_no_per_block - GC_and_WL_Unit->Get_GC_policy_specific_parameter() - page_no_per_block * S_rho_b;
						double b_r = rho * S_rho_b + 1 - rho;
						double c_r = -1 * rho / page_no_per_block;
						double mu_b = (-1 * b_r + std::sqrt(b_r * b_r - 4 * a_r * c_r)) / (2 * a_r);//assume always rho < 1 - 1/b
						for (int i = page_no_per_block; i >= 0 ; i--) {
							if (i <= int(GC_and_WL_Unit->Get_GC_policy_specific_parameter())) {
								steadystate_block_status_probability[i] = ((i + 1) * steadystate_block_status_probability[i + 1])
									/ (i + (rho / (1 - rho - mu_b * (page_no_per_block * S_rho_b - page_no_per_block + GC_and_WL_Unit->Get_GC_policy_specific_parameter()))));
							}
							else if (i < int(page_no_per_block)) {
								steadystate_block_status_probability[i] = double(page_no_per_block * mu_b) / double(i);
							} else {
								steadystate_block_status_probability[i] = mu_b;
							}
						}
						break;
					}
				}
				break;
			}
			default:
				PRINT_ERROR("Unhandled address distribution type in FTL's preconditioning function.")
			}

			//Step 3: Distribute LPAs over the entire flash space
			//MQSim assigns PPAs to LPAs based on the estimated steadystate status of blocks, assuming that there is no hot/cold data separation.
			double sum = 0;

			//Check if probability distribution is correct
			for (unsigned int i = 0; i <= page_no_per_block; i++) {
				sum += steadystate_block_status_probability[i];
			}

			//Due to some precision errors the sum may not be exactly equal to 1
			if (sum > 1.001 || sum < 0.99) {
				PRINT_ERROR("Wrong probability distribution function for the number of valid pages in flash blocks in the steady-state! It is not safe to continue preconditioning!")
			}
			Address_Mapping_Unit->Allocate_address_for_preconditioning(stat->Stream_id, lpa_set_for_preconditioning, steadystate_block_status_probability);

			//Step 4: Touch the LPAs and bring them to CMT to warmup address mapping unit
			if (!Address_Mapping_Unit->Is_ideal_mapping_table()) {
				//Step 4-1: Determine how much share of the entire CMT should be filled based on the flow arrival rate and access pattern
				unsigned int no_of_entries_in_cmt = 0;
				LPA_type min_LPA = Convert_host_logical_address_to_device_address(stat->Min_LHA);
				LPA_type max_LPA = Convert_host_logical_address_to_device_address(stat->Max_LHA);

				switch (Address_Mapping_Unit->Get_CMT_sharing_mode()) {
					case CMT_Sharing_Mode::SHARED: {
						double flow_rate = 0;
						if (stat->Type == Utils::Workload_Type::SYNTHETIC) {
							switch (stat->generator_type) {
								case Utils::Request_Generator_Type::BANDWIDTH:
									flow_rate = 1.0 / double(stat->Average_inter_arrival_time_nano_sec) * SIM_TIME_TO_SECONDS_COEFF * stat->Average_request_size_sector;
									break;
								case Utils::Request_Generator_Type::QUEUE_DEPTH:
								{
									sim_time_type max_arrival_time = sim_time_type(stat->Read_ratio * double(avg_flash_read_latency) + (1 - stat->Read_ratio) * double(avg_flash_program_latency));
									double avg_arrival_time = double(max_arrival_time) / double(stat->Request_queue_depth);
									flow_rate = 1.0 / avg_arrival_time * SIM_TIME_TO_SECONDS_COEFF * stat->Average_request_size_sector;
									break;
								}
								default:
									PRINT_ERROR("Unknown request type generator in the FTL preconditioning function.")
							}
						} else {
							flow_rate = 1.0 / double(stat->Average_inter_arrival_time_nano_sec) * SIM_TIME_TO_SECONDS_COEFF * stat->Average_request_size_sector;
						}

						no_of_entries_in_cmt = (unsigned int)(double(flow_rate) / double(overall_rate) * Address_Mapping_Unit->Get_cmt_capacity());
						break;
					}
					case CMT_Sharing_Mode::EQUAL_SIZE_PARTITIONING:
						no_of_entries_in_cmt = (unsigned int)(1.0 / double(workload_stats.size()) * Address_Mapping_Unit->Get_cmt_capacity());
						if (max_LPA - min_LPA + 1 < LPA_type(no_of_entries_in_cmt)) {
							no_of_entries_in_cmt = (unsigned int)(max_LPA - min_LPA + 1);
						}
						break;
					default:
						PRINT_ERROR("Unknown mapping table sharing mode in the FTL preconditioning function.")
				}

				if (max_LPA - min_LPA + 1 < LPA_type(trace_lpas_sorted_histogram.size())) {
					no_of_entries_in_cmt = (unsigned int)(trace_lpas_sorted_histogram.size());
				}

				//Step 4-2: Bring the LPAs into CMT based on the flow access pattern
				switch (decision_dist_type) {
					case Utils::Address_Distribution_Type::RANDOM_HOTCOLD:
					{
						//First bring hot addresses to CMT
						unsigned int required_no_of_hot_cmt_entries = (unsigned int)(stat->Ratio_of_hot_addresses_to_whole_working_set * no_of_entries_in_cmt);
						unsigned int entries_to_bring_into_cmt = required_no_of_hot_cmt_entries;
						if (required_no_of_hot_cmt_entries > hot_region_last_index_in_histogram) {
							entries_to_bring_into_cmt = hot_region_last_index_in_histogram;
						}
						auto itr = trace_lpas_sorted_histogram.begin();
						while (Address_Mapping_Unit->Get_current_cmt_occupancy_for_stream(stat->Stream_id) < entries_to_bring_into_cmt) {
							Address_Mapping_Unit->Bring_to_CMT_for_preconditioning(stat->Stream_id, (*itr).second);
							trace_lpas_sorted_histogram.erase(itr++);
						}

						//If there is free space in the CMT, then bring the remaining addresses to CMT
						no_of_entries_in_cmt -= entries_to_bring_into_cmt;
						auto itr2 = trace_lpas_sorted_histogram.begin();
						std::advance(itr2, hot_region_last_index_in_histogram);
						while (Address_Mapping_Unit->Get_current_cmt_occupancy_for_stream(stat->Stream_id) < no_of_entries_in_cmt) {
							Address_Mapping_Unit->Bring_to_CMT_for_preconditioning(stat->Stream_id, (*itr2++).second);
							if (itr2 == trace_lpas_sorted_histogram.end()) {
								break;
							}
						}
						break;
					}
					case Utils::Address_Distribution_Type::STREAMING:
					{
						LPA_type lpa;
						LPA_type first_lpa_streaming = Convert_host_logical_address_to_device_address(stat->First_Accessed_Address);
						auto itr = lpa_set_for_preconditioning.find(lpa);
						if (itr != lpa_set_for_preconditioning.begin()) {
							itr--;
						}
						while (Address_Mapping_Unit->Get_current_cmt_occupancy_for_stream(stat->Stream_id) < no_of_entries_in_cmt) {
							Address_Mapping_Unit->Bring_to_CMT_for_preconditioning(stat->Stream_id, (*itr).first);
							if (itr == lpa_set_for_preconditioning.begin()) {
								itr = lpa_set_for_preconditioning.end();
								itr--;
							} else {
								itr--;
							}
						}
						break;
					}
					case Utils::Address_Distribution_Type::RANDOM_UNIFORM:
					{
						int random_walker = int(random_generator.Uniform(0, uint32_t(trace_lpas_sorted_histogram.size()) - 2));
						int random_step = random_generator.Uniform_uint(0, (uint32_t)(trace_lpas_sorted_histogram.size()) / no_of_entries_in_cmt);
						auto itr = trace_lpas_sorted_histogram.begin();
						while (Address_Mapping_Unit->Get_current_cmt_occupancy_for_stream(stat->Stream_id) < no_of_entries_in_cmt) {
							std::advance(itr, random_step);
							Address_Mapping_Unit->Bring_to_CMT_for_preconditioning(stat->Stream_id, (*itr).second);
							if (trace_lpas_sorted_histogram.size() > 1) {
								trace_lpas_sorted_histogram.erase(itr++);
								if (random_walker + random_step >= int(trace_lpas_sorted_histogram.size() - 1) || random_walker + random_step < 0) {
									random_step *= -1;
								}
								random_walker += random_step;
							} else {
								trace_lpas_sorted_histogram.erase(itr);
								break;
							}
						}
						break;
					}
				}
			}
		}
	}
	
	void FTL::Report_results_in_XML(std::string name_prefix, Utils::XmlWriter& xmlwriter)
	{
		std::string tmp = name_prefix + ".FTL";
		xmlwriter.Write_start_element_tag(tmp);

		std::string attr = "Issued_Flash_Read_CMD";
		std::string val = std::to_string(Stats::IssuedReadCMD);
		xmlwriter.Write_attribute_string_inline(attr, val);

		attr = "Issued_Flash_Interleaved_Read_CMD";
		val = std::to_string(Stats::IssuedInterleaveReadCMD);
		xmlwriter.Write_attribute_string_inline(attr, val);

		attr = "Issued_Flash_Multiplane_Read_CMD";
		val = std::to_string(Stats::IssuedMultiplaneReadCMD);
		xmlwriter.Write_attribute_string_inline(attr, val);

		attr = "Issued_Flash_Copyback_Read_CMD";
		val = std::to_string(Stats::IssuedCopybackReadCMD);
		xmlwriter.Write_attribute_string_inline(attr, val);

		attr = "Issued_Flash_Multiplane_Copyback_Read_CMD";
		val = std::to_string(Stats::IssuedMultiplaneCopybackReadCMD);
		xmlwriter.Write_attribute_string_inline(attr, val);

		attr = "Issued_Flash_Program_CMD";
		val = std::to_string(Stats::IssuedProgramCMD);
		xmlwriter.Write_attribute_string_inline(attr, val);

		attr = "Issued_Flash_Interleaved_Program_CMD";
		val = std::to_string(Stats::IssuedInterleaveProgramCMD);
		xmlwriter.Write_attribute_string_inline(attr, val);

		attr = "Issued_Flash_Multiplane_Program_CMD";
		val = std::to_string(Stats::IssuedMultiplaneProgramCMD);
		xmlwriter.Write_attribute_string_inline(attr, val);

		attr = "Issued_Flash_Interleaved_Multiplane_Program_CMD";
		val = std::to_string(Stats::IssuedInterleaveMultiplaneProgramCMD);
		xmlwriter.Write_attribute_string_inline(attr, val);

		attr = "Issued_Flash_Copyback_Program_CMD";
		val = std::to_string(Stats::IssuedCopybackProgramCMD);
		xmlwriter.Write_attribute_string_inline(attr, val);

		attr = "Issued_Flash_Multiplane_Copyback_Program_CMD";
		val = std::to_string(Stats::IssuedMultiplaneCopybackProgramCMD);
		xmlwriter.Write_attribute_string_inline(attr, val);

		attr = "Issued_Flash_Erase_CMD";
		val = std::to_string(Stats::IssuedEraseCMD);
		xmlwriter.Write_attribute_string_inline(attr, val);

		attr = "Issued_Flash_Interleaved_Erase_CMD";
		val = std::to_string(Stats::IssuedInterleaveEraseCMD);
		xmlwriter.Write_attribute_string_inline(attr, val);

		attr = "Issued_Flash_Multiplane_Erase_CMD";
		val = std::to_string(Stats::IssuedMultiplaneEraseCMD);
		xmlwriter.Write_attribute_string_inline(attr, val);

		attr = "Issued_Flash_Interleaved_Multiplane_Erase_CMD";
		val = std::to_string(Stats::IssuedInterleaveMultiplaneEraseCMD);
		xmlwriter.Write_attribute_string_inline(attr, val);

		attr = "Issued_Flash_Suspend_Program_CMD";
		val = std::to_string(Stats::IssuedSuspendProgramCMD);
		xmlwriter.Write_attribute_string_inline(attr, val);

		attr = "Issued_Flash_Suspend_Erase_CMD";
		val = std::to_string(Stats::IssuedSuspendEraseCMD);
		xmlwriter.Write_attribute_string_inline(attr, val);

		attr = "Issued_Flash_Read_CMD_For_Mapping";
		val = std::to_string(Stats::Total_flash_reads_for_mapping);
		xmlwriter.Write_attribute_string_inline(attr, val);

		attr = "Issued_Flash_Program_CMD_For_Mapping";
		val = std::to_string(Stats::Total_flash_writes_for_mapping);
		xmlwriter.Write_attribute_string_inline(attr, val);

		attr = "CMT_Hits";
		val = std::to_string(Stats::CMT_hits);
		xmlwriter.Write_attribute_string_inline(attr, val);

		attr = "CMT_Hits_For_Read";
		val = std::to_string(Stats::readTR_CMT_hits);
		xmlwriter.Write_attribute_string_inline(attr, val);

		attr = "CMT_Hits_For_Write";
		val = std::to_string(Stats::writeTR_CMT_hits);
		xmlwriter.Write_attribute_string_inline(attr, val);

		attr = "CMT_Misses";
		val = std::to_string(Stats::CMT_miss);
		xmlwriter.Write_attribute_string_inline(attr, val);

		attr = "CMT_Misses_For_Read";
		val = std::to_string(Stats::readTR_CMT_miss);
		xmlwriter.Write_attribute_string_inline(attr, val);

		attr = "CMT_Misses_For_Write";
		val = std::to_string(Stats::writeTR_CMT_miss);
		xmlwriter.Write_attribute_string_inline(attr, val);

		attr = "Total_CMT_Queries";
		val = std::to_string(Stats::total_CMT_queries);
		xmlwriter.Write_attribute_string_inline(attr, val);

		attr = "Total_CMT_Queries_For_Reads";
		val = std::to_string(Stats::total_readTR_CMT_queries);
		xmlwriter.Write_attribute_string_inline(attr, val);

		attr = "Total_CMT_Queries_For_Writes";
		val = std::to_string(Stats::total_writeTR_CMT_queries);
		xmlwriter.Write_attribute_string_inline(attr, val);

		attr = "Total_GC_Executions";
		val = std::to_string(Stats::Total_gc_executions);
		xmlwriter.Write_attribute_string_inline(attr, val);

		attr = "Average_Page_Movement_For_GC";
		val = std::to_string(double(Stats::Total_page_movements_for_gc) / double(Stats::Total_gc_executions));
		xmlwriter.Write_attribute_string_inline(attr, val);

		attr = "Total_WL_Executions";
		val = std::to_string(Stats::Total_wl_executions);
		xmlwriter.Write_attribute_string_inline(attr, val);

		attr = "Average_Page_Movement_For_WL";
		val = std::to_string(double(Stats::Total_page_movements_for_wl) / double(Stats::Total_wl_executions));
		xmlwriter.Write_attribute_string_inline(attr, val);

		xmlwriter.Write_end_element_tag();
	}

	void FTL::Start_simulation()
	{
	}

	void FTL::Execute_simulator_event(MQSimEngine::Sim_Event*)
	{
	}

	LPA_type FTL::Convert_host_logical_address_to_device_address(LHA_type lha)
	{
		return lha / page_size_in_sectors;
	}

	page_status_type FTL::Find_NVM_subunit_access_bitmap(LHA_type lha)
	{
		return ((page_status_type)~(0xffffffffffffffff << (int)1)) << (int)(lha % page_size_in_sectors);
	}
}
