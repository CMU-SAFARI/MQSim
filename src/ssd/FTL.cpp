#include <stdexcept>
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
			unsigned int required_logical_pages = (unsigned int) (stat->Occupancy * Address_Mapping_Unit->Get_logical_pages_count(stat->Stream_id));
			unsigned int total_accessed_logical_pages = stat->Write_address_access_pattern.size() + stat->Read_address_access_pattern.size() - stat->Write_address_access_pattern.size();
			while (total_accessed_logical_pages < required_logical_pages)
			{
				if (stat->Type == Utils::Workload_Type::SYNTHETIC)
				{
					switch (stat->Address_distribution_type)
					{
					case Utils::Address_Distribution_Type::HOTCOLD_RANDOM:
						break;
					case Utils::Address_Distribution_Type::STREAMING:
						break;
					case Utils::Address_Distribution_Type::UNIFORM_RANDOM:
						break;
					}
				}
				else
				{

				}

				required_logical_pages++;
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
