#include <stdexcept>
#include <cmath>
#include <vector>
#include "FTL.h"
#include "Stats.h"
#include "../utils/DistributionTypes.h"

namespace SSD_Components
{
	FTL::FTL(const sim_object_id_type& id, Data_Cache_Manager_Base* data_cache_manager, int seed) :
		NVM_Firmware(id, data_cache_manager), random_generator(seed)
	{
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

	void FTL::Perform_precondition(std::vector<Preconditioning::Workload_Statistics*> workload_stats)
	{
		return;
		for (auto stat : workload_stats)
		{
			//First generate enough number of LPAs that are accessed in the steady-state
			std::vector<LPA_type> accessed_lpas;
			bool is_read = false;
			unsigned int size = 0;
			LSA_type start_LBA = 0, streaming_next_address = 0, hot_region_end_lsa = 0;
			Utils::RandomGenerator* random_request_type_generator = new Utils::RandomGenerator(stat->random_request_type_generator_seed);
			Utils::RandomGenerator* random_address_generator = new Utils::RandomGenerator(stat->random_address_generator_seed);
			Utils::RandomGenerator* random_hot_address_generator = NULL;
			Utils::RandomGenerator* random_hot_cold_generator = NULL;
			Utils::RandomGenerator* random_request_size_generator = NULL;
			unsigned int no_of_logical_pages_in_staedystate = (unsigned int) (stat->Occupancy * Address_Mapping_Unit->Get_logical_pages_count(stat->Stream_id));
			unsigned int total_accessed_logical_pages = stat->Write_address_access_pattern.size() + stat->Read_address_access_pattern.size() - stat->Write_address_access_pattern.size();
		
			while (total_accessed_logical_pages < no_of_logical_pages_in_staedystate)
			{
				if (stat->Type == Utils::Workload_Type::SYNTHETIC)
				{
					if (stat->address_distribution == Utils::Address_Distribution_Type::HOTCOLD_RANDOM)
					{
						random_hot_address_generator = new Utils::RandomGenerator(stat->random_hot_address_generator_seed);
						random_hot_cold_generator = new Utils::RandomGenerator(stat->random_hot_cold_generator_seed);
						hot_region_end_lsa = stat->Smallest_Accessed_Address + (LSA_type)((double)(stat->Largest_Accessed_Address - stat->Smallest_Accessed_Address) * stat->Hot_region_ratio);
					}
					else if (stat->address_distribution == Utils::Address_Distribution_Type::STREAMING)
						streaming_next_address = random_address_generator->Uniform_ulong(stat->Smallest_Accessed_Address, stat->Largest_Accessed_Address);

					if (stat->Request_size_distribution_type == Utils::Request_Size_Distribution_Type::NORMAL)
					{
						random_request_size_generator = new Utils::RandomGenerator(stat->random_request_size_generator_seed);
					}

					if (random_request_type_generator->Uniform(0, 1) <= stat->Read_ratio)
						is_read = true;

					switch (stat->Request_size_distribution_type)
					{
						case Utils::Request_Size_Distribution_Type::FIXED:
							size = stat->Average_request_size;
							break;
						case Utils::Request_Size_Distribution_Type::NORMAL:
						{
							double temp_request_size = random_request_size_generator->Normal(stat->Average_request_size, stat->STDEV_reuqest_size);
							size = (unsigned int)(std::ceil(temp_request_size));
							if (size <= 0)
								size = 1;
							break;
						}
					}

					switch (stat->Address_distribution_type)
					{
					case Utils::Address_Distribution_Type::STREAMING:
						start_LBA = streaming_next_address;
						if (start_LBA + size > stat->Largest_Accessed_Address)
							start_LBA = stat->Smallest_Accessed_Address;
						streaming_next_address += size;
						if (streaming_next_address > stat->Largest_Accessed_Address)
							streaming_next_address = stat->Largest_Accessed_Address;
						break;
					case Utils::Address_Distribution_Type::HOTCOLD_RANDOM:
						if (random_hot_cold_generator->Uniform(0, 1) < stat->Hot_region_ratio)// (100-hot)% of requests going to hot% of the address space
						{
							start_LBA = random_hot_address_generator->Uniform_ulong(hot_region_end_lsa + 1, stat->Largest_Accessed_Address);
							if (start_LBA < hot_region_end_lsa + 1 || start_LBA > stat->Largest_Accessed_Address)
								PRINT_ERROR("Out of range address is generated in IO_Flow_Synthetic!\n")
								if (start_LBA + size > stat->Largest_Accessed_Address)
									start_LBA = hot_region_end_lsa + 1;
						}
						else
						{
							start_LBA = random_hot_address_generator->Uniform_ulong(stat->Smallest_Accessed_Address, hot_region_end_lsa);
							if (start_LBA < stat->Smallest_Accessed_Address || start_LBA > hot_region_end_lsa)
								PRINT_ERROR("Out of range address is generated in IO_Flow_Synthetic!\n")
						}
						break;
					case Utils::Address_Distribution_Type::UNIFORM_RANDOM:
						start_LBA = random_address_generator->Uniform_ulong(stat->Smallest_Accessed_Address, stat->Largest_Accessed_Address);
						if (start_LBA < stat->Smallest_Accessed_Address || start_LBA > stat->Largest_Accessed_Address)
							PRINT_ERROR("Out of range address is generated in IO_Flow_Synthetic!\n")
							if (start_LBA + size > stat->Largest_Accessed_Address)
								start_LBA = stat->Smallest_Accessed_Address;
						break;
					}
				}
				else
				{

				}

				total_accessed_logical_pages++;
			}

			//Assign PPAs to LPAs based on the steady-state status of pages
			if (stat->Type == Utils::Workload_Type::SYNTHETIC)
			{
				switch (GC_and_WL_Unit->Get_gc_policy())
				{
				case GC_Block_Selection_Policy_Type::GREEDY:
					break;
				case GC_Block_Selection_Policy_Type::RGA:
					break;
				case GC_Block_Selection_Policy_Type::FIFO:
					break;
				case GC_Block_Selection_Policy_Type::RANDOM:
					break;
				case GC_Block_Selection_Policy_Type::RANDOM_P:
					break;
				case GC_Block_Selection_Policy_Type::RANDOM_PP:
					break;
				}
			}
			else
			{
				switch (GC_and_WL_Unit->Get_gc_policy())
				{
				case GC_Block_Selection_Policy_Type::GREEDY:
					break;
				case GC_Block_Selection_Policy_Type::RGA:
					break;
				case GC_Block_Selection_Policy_Type::FIFO:
					break;
				case GC_Block_Selection_Policy_Type::RANDOM:
					break;
				case GC_Block_Selection_Policy_Type::RANDOM_P:
					break;
				case GC_Block_Selection_Policy_Type::RANDOM_PP:
					break;
				}
			}

			//Touch the LPAs to warmup CMT
		}
	}
	
	/*	
	static unsigned long Total_flash_reads_for_mapping_per_stream[MAX_SUPPORT_STREAMS], Total_flash_writes_for_mapping_per_stream[MAX_SUPPORT_STREAMS];

	static unsigned int CMT_hits_per_stream[MAX_SUPPORT_STREAMS], readTR_CMT_hits_per_stream[MAX_SUPPORT_STREAMS], writeTR_CMT_hits_per_stream[MAX_SUPPORT_STREAMS];
	static unsigned int CMT_miss_per_stream[MAX_SUPPORT_STREAMS], readTR_CMT_miss_per_stream[MAX_SUPPORT_STREAMS], writeTR_CMT_miss_per_stream[MAX_SUPPORT_STREAMS];
	static unsigned int total_CMT_queries_per_stream[MAX_SUPPORT_STREAMS], total_readTR_CMT_queries_per_stream[MAX_SUPPORT_STREAMS], total_writeTR_CMT_queries_per_stream[MAX_SUPPORT_STREAMS];

	*/
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

		xmlwriter.Write_end_element_tag();
	}

	void FTL::Start_simulation() {}
	void FTL::Execute_simulator_event(MQSimEngine::Sim_Event*) {}
}
