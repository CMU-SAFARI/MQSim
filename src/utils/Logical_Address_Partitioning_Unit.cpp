#include "Logical_Address_Partitioning_Unit.h"


namespace Utils
{
	int**** Logical_Address_Partitioning_Unit::resource_list;
	std::vector<std::vector<flash_channel_ID_type>> Logical_Address_Partitioning_Unit::stream_channel_ids;
	std::vector<std::vector<flash_chip_ID_type>> Logical_Address_Partitioning_Unit::stream_chip_ids;
	std::vector<std::vector<flash_die_ID_type>> Logical_Address_Partitioning_Unit::stream_die_ids;
	std::vector<std::vector<flash_plane_ID_type>> Logical_Address_Partitioning_Unit::stream_plane_ids;
	HostInterface_Types Logical_Address_Partitioning_Unit::hostinterface_type;
	bool Logical_Address_Partitioning_Unit::initialized = false;
	std::vector<LHA_type> Logical_Address_Partitioning_Unit::pdas_per_flow;
	std::vector<LHA_type> Logical_Address_Partitioning_Unit::start_lhas_per_flow;
	std::vector<LHA_type> Logical_Address_Partitioning_Unit::end_lhas_per_flow;
	unsigned int Logical_Address_Partitioning_Unit::channel_count;
	unsigned int Logical_Address_Partitioning_Unit::chip_no_per_channel;
	unsigned int Logical_Address_Partitioning_Unit::die_no_per_chip;
	unsigned int Logical_Address_Partitioning_Unit::plane_no_per_die;
	LHA_type Logical_Address_Partitioning_Unit::total_pda_no = 0;
	LHA_type Logical_Address_Partitioning_Unit::total_lha_no = 0;

	void Logical_Address_Partitioning_Unit::Reset()
	{
		initialized = false;
		pdas_per_flow.clear();
		start_lhas_per_flow.clear();
		end_lhas_per_flow.clear();
		stream_channel_ids.clear();
		stream_chip_ids.clear();
		stream_die_ids.clear();
		stream_plane_ids.clear();

		for (flash_channel_ID_type channel_id = 0; channel_id < channel_count; channel_id++) {
			for (flash_chip_ID_type chip_id = 0; chip_id < chip_no_per_channel; chip_id++) {
				for (flash_die_ID_type die_id = 0; die_id < die_no_per_chip; die_id++) {
					delete[] resource_list[channel_id][chip_id][die_id];
				}
				delete[] resource_list[channel_id][chip_id];
			}
			delete[] resource_list[channel_id];
		}

		delete[] resource_list;
	}

	void Logical_Address_Partitioning_Unit::Allocate_logical_address_for_flows(HostInterface_Types hostinterface_type, unsigned int concurrent_stream_no,
		unsigned int channel_count, unsigned int chip_no_per_channel, unsigned int die_no_per_chip, unsigned int plane_no_per_die,
		std::vector<std::vector<flash_channel_ID_type>> stream_channel_ids, std::vector<std::vector<flash_chip_ID_type>> stream_chip_ids,
		std::vector<std::vector<flash_die_ID_type>> stream_die_ids, std::vector<std::vector<flash_plane_ID_type>> stream_plane_ids,
		unsigned int block_no_per_plane, unsigned int page_no_per_block, unsigned int sector_no_per_page, double overprovisioning_ratio) 
	{
		if (initialized) {
			return;
		}
		
		Logical_Address_Partitioning_Unit::hostinterface_type = hostinterface_type;

		Logical_Address_Partitioning_Unit::channel_count = channel_count;;
		Logical_Address_Partitioning_Unit::chip_no_per_channel = chip_no_per_channel;
		Logical_Address_Partitioning_Unit::die_no_per_chip = die_no_per_chip;
		Logical_Address_Partitioning_Unit::plane_no_per_die = plane_no_per_die;

		resource_list = new int***[channel_count];
		bool resource_sharing = false;
		for (flash_channel_ID_type channel_id = 0; channel_id < channel_count; channel_id++) {
			resource_list[channel_id] = new int**[chip_no_per_channel];
			for (flash_chip_ID_type chip_id = 0; chip_id < chip_no_per_channel; chip_id++) {
				resource_list[channel_id][chip_id] = new int*[die_no_per_chip];
				for (flash_die_ID_type die_id = 0; die_id < die_no_per_chip; die_id++) {
					resource_list[channel_id][chip_id][die_id] = new int[plane_no_per_die];
					for (flash_plane_ID_type plane_id = 0; plane_id < plane_no_per_die; plane_id++) {
						resource_list[channel_id][chip_id][die_id][plane_id] = 0;
					}
				}
			}
		}

		for (unsigned int stream_id = 0; stream_id < concurrent_stream_no; stream_id++)
		{
			for (flash_channel_ID_type channel_id = 0; channel_id < stream_channel_ids[stream_id].size(); channel_id++) {
				for (flash_chip_ID_type chip_id = 0; chip_id < stream_chip_ids[stream_id].size(); chip_id++) {
					for (flash_die_ID_type die_id = 0; die_id < stream_die_ids[stream_id].size(); die_id++) {
						for (flash_plane_ID_type plane_id = 0; plane_id < stream_plane_ids[stream_id].size(); plane_id++) {
							if (!(stream_channel_ids[stream_id][channel_id] < channel_count)) {
								PRINT_ERROR("Invalid channel ID specified for I/O flow " << stream_id);
							}

							if (!(stream_chip_ids[stream_id][chip_id] < chip_no_per_channel)) {
								PRINT_ERROR("Invalid chip ID specified for I/O flow " << stream_id);
							}

							if (!(stream_die_ids[stream_id][die_id] < die_no_per_chip)) {
								PRINT_ERROR("Invalid die ID specified for I/O flow " << stream_id);
							}

							if (!(stream_plane_ids[stream_id][plane_id] < plane_no_per_die)) {
								PRINT_ERROR("Invalid plane ID specified for I/O flow " << stream_id);
							}
							resource_list[stream_channel_ids[stream_id][channel_id]][stream_chip_ids[stream_id][chip_id]][stream_die_ids[stream_id][die_id]][stream_plane_ids[stream_id][plane_id]]++;
						}
					}
				}
			}
		}

		for (unsigned int stream_id = 0; stream_id < concurrent_stream_no; stream_id++)
		{
			std::vector<flash_channel_ID_type> channel_ids;
			Logical_Address_Partitioning_Unit::stream_channel_ids.push_back(channel_ids);
			for (flash_channel_ID_type channel_id = 0; channel_id < stream_channel_ids[stream_id].size(); channel_id++) {
				Logical_Address_Partitioning_Unit::stream_channel_ids[stream_id].push_back(stream_channel_ids[stream_id][channel_id]);
			}

			std::vector<flash_chip_ID_type> chip_ids;
			Logical_Address_Partitioning_Unit::stream_chip_ids.push_back(chip_ids);
			for (flash_chip_ID_type chip_id = 0; chip_id < stream_chip_ids[stream_id].size(); chip_id++) {
				Logical_Address_Partitioning_Unit::stream_chip_ids[stream_id].push_back(stream_chip_ids[stream_id][chip_id]);
			}

			std::vector<flash_die_ID_type> die_ids;
			Logical_Address_Partitioning_Unit::stream_die_ids.push_back(die_ids);
			for (flash_die_ID_type die_id = 0; die_id < stream_die_ids[stream_id].size(); die_id++) {
				Logical_Address_Partitioning_Unit::stream_die_ids[stream_id].push_back(stream_die_ids[stream_id][die_id]);
			}

			std::vector<flash_plane_ID_type> plane_ids;
			Logical_Address_Partitioning_Unit::stream_plane_ids.push_back(plane_ids);
			for (flash_plane_ID_type plane_id = 0; plane_id < stream_plane_ids[stream_id].size(); plane_id++) {
				Logical_Address_Partitioning_Unit::stream_plane_ids[stream_id].push_back(stream_plane_ids[stream_id][plane_id]);
			}
		}

		std::vector<LHA_type> lsa_count_per_stream;
		for (unsigned int stream_id = 0; stream_id < concurrent_stream_no; stream_id++) {
			LHA_type lsa_count = 0;
			for (flash_channel_ID_type channel_id = 0; channel_id < stream_channel_ids[stream_id].size(); channel_id++) {
				for (flash_chip_ID_type chip_id = 0; chip_id < stream_chip_ids[stream_id].size(); chip_id++) {
					for (flash_die_ID_type die_id = 0; die_id < stream_die_ids[stream_id].size(); die_id++) {
						for (flash_plane_ID_type plane_id = 0; plane_id < stream_plane_ids[stream_id].size(); plane_id++) {
							lsa_count += (LHA_type)((block_no_per_plane * page_no_per_block * sector_no_per_page * (1.0 - overprovisioning_ratio) *
								1.0 / double(resource_list[stream_channel_ids[stream_id][channel_id]][stream_chip_ids[stream_id][chip_id]][stream_die_ids[stream_id][die_id]][stream_plane_ids[stream_id][plane_id]])));
						}
					}
				}
			}
			pdas_per_flow.push_back(LHA_type(double(lsa_count) / (1.0 - overprovisioning_ratio)));
			total_pda_no += pdas_per_flow[stream_id];
			lsa_count_per_stream.push_back(lsa_count);
		}

		total_lha_no = 0;
		for (unsigned int stream_id = 0; stream_id < concurrent_stream_no; stream_id++) {
			start_lhas_per_flow.push_back(total_lha_no);
			end_lhas_per_flow.push_back(total_lha_no + lsa_count_per_stream[stream_id] - 1);
			total_lha_no += lsa_count_per_stream[stream_id];
		}

		initialized = true;
	}

	double Logical_Address_Partitioning_Unit::Get_share_of_physcial_pages_in_plane(flash_channel_ID_type channel_id, flash_chip_ID_type chip_id, flash_die_ID_type die_id, flash_plane_ID_type plane_id)
	{
		switch (hostinterface_type) {
			case HostInterface_Types::NVME:
				return 1.0 / double(resource_list[channel_id][chip_id][die_id][plane_id]);
			case HostInterface_Types::SATA:
			default:
				break;
		}

		return 1.0;
	}

	LHA_type Logical_Address_Partitioning_Unit::Start_lha_available_to_flow(stream_id_type stream_id)
	{
		if (initialized) {
			switch (hostinterface_type) {
				case HostInterface_Types::SATA:
					return start_lhas_per_flow[stream_id];
					break;
				case HostInterface_Types::NVME:
					return start_lhas_per_flow[stream_id];
					break;
				default:
					break;
			}
		}

		PRINT_ERROR("The address partitioning unit is not initialized!")
	}

	LHA_type Logical_Address_Partitioning_Unit::End_lha_available_to_flow(stream_id_type stream_id)
	{
		if (initialized) {
			switch (hostinterface_type) {
				case HostInterface_Types::SATA:
					return end_lhas_per_flow[stream_id];
					break;
				case HostInterface_Types::NVME:
					return end_lhas_per_flow[stream_id];
					break;
				default:
					break;
			}
		}
		PRINT_ERROR("The address partitioning unit is not initialized!")
	}

	LHA_type Logical_Address_Partitioning_Unit::LHA_count_allocate_to_flow_from_host_view(stream_id_type stream_id)
	{
		if (initialized) {
			switch (hostinterface_type) {
				case HostInterface_Types::SATA:
					return (end_lhas_per_flow[stream_id] - start_lhas_per_flow[stream_id] + 1);
					break;
				case HostInterface_Types::NVME:
					return (end_lhas_per_flow[stream_id] - start_lhas_per_flow[stream_id] + 1);
					break;
				default:
					break;
			}
		}
		PRINT_ERROR("The address partitioning unit is not initialized!")
	}

	LHA_type Logical_Address_Partitioning_Unit::LHA_count_allocate_to_flow_from_device_view(stream_id_type stream_id)
	{
		if (initialized) {
			switch (hostinterface_type) {
				case HostInterface_Types::SATA://It is not possible to differentiate between streams inside the device when a SATA host interface is used, so all lha space is available to all streams
					return total_lha_no;
					break;
				case HostInterface_Types::NVME:
					return (end_lhas_per_flow[stream_id] - start_lhas_per_flow[stream_id] + 1);
					break;
				default:
					break;
			}
		}
		PRINT_ERROR("The address partitioning unit is not initialized!")
	}

	PDA_type Logical_Address_Partitioning_Unit::PDA_count_allocate_to_flow(stream_id_type stream_id)
	{
		if (initialized) {
			switch (hostinterface_type) {
				case HostInterface_Types::SATA://It is not possible to differentiate between streams inside the device when a SATA host interface is used, so all lha space is available to all streams
					return total_pda_no;
				case HostInterface_Types::NVME:
					return pdas_per_flow[stream_id];
					break;
				default:
					break;
			}
		}
		PRINT_ERROR("The address partitioning unit is not initialized!")
	}

	LHA_type Logical_Address_Partitioning_Unit::Get_total_device_lha_count()
	{
		return total_lha_no;
	}
}
