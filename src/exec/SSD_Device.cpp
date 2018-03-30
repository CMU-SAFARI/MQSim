#include <vector>
#include <stdexcept>
#include "SSD_Device.h"
#include "../ssd/ONFI_Channel_Base.h"
#include "../ssd/Flash_Block_Manager.h"
#include "../ssd/Address_Mapping_Unit_Base.h"
#include "../ssd/Address_Mapping_Unit_Page_Level.h"
#include "../ssd/Address_Mapping_Unit_Hybrid.h"
#include "../ssd/GC_and_WL_Unit_Page_Level.h"
#include "../ssd/TSU_OutofOrder.h"
#include "../ssd/ONFI_Channel_NVDDR2.h"
#include "../ssd/NVM_PHY_ONFI_NVDDR2.h"


SSD_Device::SSD_Device(Device_Parameter_Set* parameters, std::vector<IO_Flow_Parameter_Set*>* io_flows) :
	MQSimEngine::Sim_Object("SSDDevice")
{
	SSD_Device* device = this;
	Simulator->AddObject(device);

	device->Memory_Type = parameters->Memory_Type;

	switch (Memory_Type)
	{
	case NVM::NVM_Type::FLASH:
	{
		sim_time_type * read_latencies, *write_latencies;

		//Step 1: create memory chips (flash chips in our case)
		switch (parameters->Flash_Parameters.Flash_Technology)
		{
		case Flash_Technology_Type::SLC:
			read_latencies = new sim_time_type[1];
			read_latencies[0] = parameters->Flash_Parameters.Page_Read_Latency_LSB;
			write_latencies = new sim_time_type[1];
			write_latencies[0] = parameters->Flash_Parameters.Page_Program_Latency_LSB;
			break;
		case Flash_Technology_Type::MLC:
			read_latencies = new sim_time_type[2];
			read_latencies[0] = parameters->Flash_Parameters.Page_Read_Latency_LSB;
			read_latencies[1] = parameters->Flash_Parameters.Page_Read_Latency_MSB;
			write_latencies = new sim_time_type[2];
			write_latencies[0] = parameters->Flash_Parameters.Page_Program_Latency_LSB;
			write_latencies[1] = parameters->Flash_Parameters.Page_Program_Latency_MSB;
			break;
		case Flash_Technology_Type::TLC:
			read_latencies = new sim_time_type[3];
			read_latencies[0] = parameters->Flash_Parameters.Page_Read_Latency_LSB;
			read_latencies[1] = parameters->Flash_Parameters.Page_Read_Latency_CSB;
			read_latencies[2] = parameters->Flash_Parameters.Page_Read_Latency_MSB;
			write_latencies = new sim_time_type[3];
			write_latencies[0] = parameters->Flash_Parameters.Page_Program_Latency_LSB;
			write_latencies[1] = parameters->Flash_Parameters.Page_Program_Latency_CSB;
			write_latencies[2] = parameters->Flash_Parameters.Page_Program_Latency_MSB;
			break;
		default:
			throw std::invalid_argument("The specified flash technologies is not supported");
		}

		//Step 2: create memory channels to connect chips to the controller
		switch (parameters->Flash_Comm_Protocol)
		{
		case SSD_Components::ONFI_Protocol::NVDDR2:
		{
			SSD_Components::ONFI_Channel_NVDDR2** channels = new SSD_Components::ONFI_Channel_NVDDR2*[parameters->Flash_Channel_Count];
			for (unsigned int channel_cntr = 0; channel_cntr < parameters->Flash_Channel_Count; channel_cntr++)
			{
				NVM::FlashMemory::Flash_Chip** chips = new NVM::FlashMemory::Flash_Chip*[parameters->Chip_No_Per_Channel];
				for (unsigned int chip_cntr = 0; chip_cntr < parameters->Chip_No_Per_Channel; chip_cntr++)
				{
					chips[chip_cntr] = new NVM::FlashMemory::Flash_Chip(device->ID() + ".Channel." + std::to_string(channel_cntr) + ".Chip." + std::to_string(chip_cntr),
						channel_cntr, chip_cntr, parameters->Flash_Parameters.Flash_Technology, parameters->Flash_Parameters.Die_No_Per_Chip, parameters->Flash_Parameters.Plane_No_Per_Die,
						parameters->Flash_Parameters.Block_No_Per_Plane, parameters->Flash_Parameters.Page_No_Per_Block,
						read_latencies, write_latencies, parameters->Flash_Parameters.Block_Erase_Latency,
						parameters->Flash_Parameters.Suspend_Program_Time, parameters->Flash_Parameters.Suspend_Erase_Time);
					Simulator->AddObject(chips[chip_cntr]);//Each simulation object (a child of MQSimEngine::Sim_Object) should be added to the engine
				}
				channels[channel_cntr] = new SSD_Components::ONFI_Channel_NVDDR2(channel_cntr, parameters->Chip_No_Per_Channel,
					chips, parameters->Flash_Channel_Width,
					(sim_time_type)((double)1000 / parameters->Channel_Transfer_Rate) * 2, (sim_time_type)((double)1000 / parameters->Channel_Transfer_Rate) * 2);
				device->Channels.push_back(channels[channel_cntr]);//Channels should not be added to the simulator core, they are passive object that do not handle any simulation event
			}

			//Step 3: create channel controller and connect channels to it
			device->PHY = new SSD_Components::NVM_PHY_ONFI_NVDDR2(device->ID() + ".PHY", channels, parameters->Flash_Channel_Count, parameters->Chip_No_Per_Channel,
				parameters->Flash_Parameters.Die_No_Per_Chip, parameters->Flash_Parameters.Plane_No_Per_Die);
			Simulator->AddObject(device->PHY);
			break;
		}
		default:
			throw std::invalid_argument("No implementation is available for the specified flash communication protocol");
		}

		//Steps 4 - 8: create FTL components and connect them together
		SSD_Components::FTL* ftl = new SSD_Components::FTL(device->ID() + ".FTL", NULL);
		Simulator->AddObject(ftl);
		device->Firmware = ftl;

		//Step 5: create TSU
		SSD_Components::TSU_Base* tsu;
		bool erase_suspension = false, program_suspension = false;
		if (parameters->Flash_Parameters.CMD_Suspension_Support == NVM::FlashMemory::Command_Suspension_Mode::PROGRAM)
			program_suspension = true;
		if (parameters->Flash_Parameters.CMD_Suspension_Support == NVM::FlashMemory::Command_Suspension_Mode::ERASE)
			erase_suspension = true;
		if (parameters->Flash_Parameters.CMD_Suspension_Support == NVM::FlashMemory::Command_Suspension_Mode::PROGRAM_ERASE)
		{
			program_suspension = true;
			erase_suspension = true;
		}
		switch (parameters->Transaction_Scheduling_Policy)
		{
		case SSD_Components::Flash_Scheduling_Type::OUT_OF_ORDER:
			tsu = new SSD_Components::TSU_OutofOrder(ftl->ID() + ".TSU", ftl, static_cast<SSD_Components::NVM_PHY_ONFI_NVDDR2*>(device->PHY),
				parameters->Flash_Channel_Count, parameters->Chip_No_Per_Channel,
				parameters->Flash_Parameters.Die_No_Per_Chip, parameters->Flash_Parameters.Plane_No_Per_Die,
				parameters->Preferred_suspend_write_time_for_read, parameters->Preferred_suspend_erase_time_for_read, parameters->Preferred_suspend_erase_time_for_write,
				erase_suspension, program_suspension);
			break;
		default:
			throw std::invalid_argument("No implementation is available for the specified transaction scheduling algorithm");
		}
		Simulator->AddObject(tsu);
		ftl->TSU = tsu;

		//Step 6: create Flash_Block_Manager
		SSD_Components::Flash_Block_Manager_Base* fbm;
		fbm = new SSD_Components::Flash_Block_Manager(NULL, parameters->Flash_Parameters.Block_PE_Cycles_Limit,
			(unsigned int)io_flows->size(), parameters->Flash_Channel_Count, parameters->Chip_No_Per_Channel,
			parameters->Flash_Parameters.Die_No_Per_Chip, parameters->Flash_Parameters.Plane_No_Per_Die,
			parameters->Flash_Parameters.Block_No_Per_Plane, parameters->Flash_Parameters.Page_No_Per_Block);
		ftl->BlockManager = fbm;


		//Step 7: create Address_Mapping_Unit
		SSD_Components::Address_Mapping_Unit_Base* amu;
		std::vector<std::vector<flash_channel_ID_type>> flow_channel_id_assignments;
		std::vector<std::vector<flash_chip_ID_type>> flow_chip_id_assignments;
		std::vector<std::vector<flash_die_ID_type>> flow_die_id_assignments;
		std::vector<std::vector<flash_plane_ID_type>> flow_plane_id_assignments;
		for (int i = 0; i < io_flows->size(); i++)
		{
			std::vector<flash_channel_ID_type> channel_ids;
			flow_channel_id_assignments.push_back(channel_ids);
			for (int j = 0; j < (*io_flows)[i]->Channel_No; j++)
				flow_channel_id_assignments[i].push_back((*io_flows)[i]->Channel_IDs[j]);
			std::vector<flash_chip_ID_type> chip_ids;
			flow_chip_id_assignments.push_back(chip_ids);
			for (int j = 0; j < (*io_flows)[i]->Chip_No; j++)
				flow_chip_id_assignments[i].push_back((*io_flows)[i]->Chip_IDs[j]);
			std::vector<flash_die_ID_type> die_ids;
			flow_die_id_assignments.push_back(die_ids);
			for (int j = 0; j < (*io_flows)[i]->Die_No; j++)
				flow_die_id_assignments[i].push_back((*io_flows)[i]->Die_IDs[j]);
			std::vector<flash_plane_ID_type> plane_ids;
			flow_plane_id_assignments.push_back(plane_ids);
			for (int j = 0; j < (*io_flows)[i]->Plane_No; j++)
				flow_plane_id_assignments[i].push_back((*io_flows)[i]->Plane_IDs[j]);
		}
		switch (parameters->Address_Mapping)
		{
		case SSD_Components::Flash_Address_Mapping_Type::PAGE_LEVEL:
			amu = new SSD_Components::Address_Mapping_Unit_Page_Level(ftl->ID() + ".AddressMappingUnit", ftl, (SSD_Components::NVM_PHY_ONFI*) device->PHY,
				fbm, parameters->Ideal_Mapping_Table, parameters->CMT_Capacity, parameters->Plane_Allocation_Scheme, (unsigned int)io_flows->size(),
				parameters->Flash_Channel_Count, parameters->Chip_No_Per_Channel, parameters->Flash_Parameters.Die_No_Per_Chip,
				parameters->Flash_Parameters.Plane_No_Per_Die,
				flow_channel_id_assignments, flow_chip_id_assignments, flow_die_id_assignments, flow_plane_id_assignments,
				parameters->Flash_Parameters.Block_No_Per_Plane, parameters->Flash_Parameters.Page_No_Per_Block,
				parameters->Flash_Parameters.Page_Capacity / SECTOR_SIZE_IN_BYTE, parameters->Flash_Parameters.Page_Capacity, parameters->Overprovisioning_Ratio,
				parameters->CMT_Sharing_Mode);
			break;
		case SSD_Components::Flash_Address_Mapping_Type::HYBRID:
			amu = new SSD_Components::Address_Mapping_Unit_Hybrid(ftl->ID() + ".AddressMappingUnit", ftl, (SSD_Components::NVM_PHY_ONFI*) device->PHY,
				fbm, parameters->Ideal_Mapping_Table, (unsigned int)io_flows->size(),
				parameters->Flash_Channel_Count, parameters->Chip_No_Per_Channel, parameters->Flash_Parameters.Die_No_Per_Chip,
				parameters->Flash_Parameters.Plane_No_Per_Die, parameters->Flash_Parameters.Block_No_Per_Plane, parameters->Flash_Parameters.Page_No_Per_Block,
				parameters->Flash_Parameters.Page_Capacity / SECTOR_SIZE_IN_BYTE, parameters->Flash_Parameters.Page_Capacity, parameters->Overprovisioning_Ratio);
			break;
		default:
			throw std::invalid_argument("No implementation is available fo the secified address mapping strategy");
		}
		Simulator->AddObject(amu);
		ftl->Address_Mapping_Unit = amu;
		LSA_type total_logical_sectors_in_SSD = 0;
		for (stream_id_type stream_id = 0; stream_id < io_flows->size(); stream_id++)
			total_logical_sectors_in_SSD += amu->Get_logical_sectors_count_allocated_to_stream(stream_id);

		//Step 8: create GC_and_WL_unit
		SSD_Components::GC_and_WL_Unit_Base* gcwl;
		gcwl = new SSD_Components::GC_and_WL_Unit_Page_Level(ftl->ID() + ".GCandWLUnit", amu, fbm, tsu, (SSD_Components::NVM_PHY_ONFI*)device->PHY,
			parameters->GC_Block_Selection_Policy, parameters->GC_Exec_Threshold, parameters->Preemptible_GC_Enabled, parameters->GC_Hard_Threshold,
			parameters->Flash_Channel_Count, parameters->Chip_No_Per_Channel,
			parameters->Flash_Parameters.Die_No_Per_Chip, parameters->Flash_Parameters.Plane_No_Per_Die,
			parameters->Flash_Parameters.Block_No_Per_Plane, parameters->Flash_Parameters.Page_No_Per_Block,
			parameters->Flash_Parameters.Page_Capacity / SECTOR_SIZE_IN_BYTE, parameters->Use_Copyback_for_GC, 
			parameters->Seed++);
		Simulator->AddObject(gcwl);
		fbm->Set_GC_and_WL_Unit(gcwl);
		ftl->GC_and_WL_Unit = gcwl;

		//Step 9: create Data_Cache_Manager
		SSD_Components::Data_Cache_Manager_Base* dcm;
		SSD_Components::Caching_Mode* caching_modes = new SSD_Components::Caching_Mode[io_flows->size()];
		for (unsigned int i = 0; i < io_flows->size(); i++)
			caching_modes[i] = (*io_flows)[i]->Device_Level_Data_Caching_Mode;
		dcm = new SSD_Components::Data_Cache_Manager_Flash(device->ID() + ".DataCache", NULL, ftl, (SSD_Components::NVM_PHY_ONFI*) device->PHY,
			parameters->Data_Cache_Capacity, parameters->Data_Cache_DRAM_Row_Size, parameters->Data_Cache_DRAM_Data_Rate,
			parameters->Data_Cache_DRAM_Data_Busrt_Size, parameters->Data_Cache_DRAM_tRCD, parameters->Data_Cache_DRAM_tCL, parameters->Data_Cache_DRAM_tRP,
			caching_modes, parameters->Data_Cache_Sharing_Mode, (unsigned int)io_flows->size(),
			parameters->Flash_Parameters.Page_Capacity / SECTOR_SIZE_IN_BYTE, parameters->Flash_Channel_Count * parameters->Chip_No_Per_Channel * parameters->Flash_Parameters.Die_No_Per_Chip * parameters->Flash_Parameters.Plane_No_Per_Die * parameters->Flash_Parameters.Page_Capacity / SECTOR_SIZE_IN_BYTE);
		Simulator->AddObject(dcm);
		ftl->Data_cache_manager = dcm;
		device->Cache_manager = dcm;

		//Step 10: create Host_Interface
		switch (parameters->HostInterface_Type)
		{
		case HostInterfaceType::NVME:
			device->Host_interface = new SSD_Components::Host_Interface_NVMe(device->ID() + ".HostInterface",
				total_logical_sectors_in_SSD, parameters->IO_Queue_Depth, parameters->IO_Queue_Depth,
				(unsigned int)io_flows->size(), parameters->Queue_Fetch_Size, parameters->Flash_Parameters.Page_Capacity / SECTOR_SIZE_IN_BYTE, dcm);
			break;
		case HostInterfaceType::SATA:
			break;
		default:
			break;
		}
		Simulator->AddObject(device->Host_interface);
		dcm->Set_host_interface(device->Host_interface);
		break;
	}
	default:
		throw std::invalid_argument("Undefined NVM type specified ");
	}
}

void SSD_Device::Attach_to_host(Host_Components::PCIe_Switch* pcie_switch)
{
	this->Host_interface->Attach_to_device(pcie_switch);
}

void SSD_Device::Perform_preconditioning()
{
	this->Cache_manager->Make_warmup();
	((SSD_Components::FTL*)this->Firmware)->Perform_precondition();
}

void SSD_Device::Start_simulation() {}

void SSD_Device::Validate_simulation_config() {}

void SSD_Device::Execute_simulator_event(MQSimEngine::Sim_Event* event) {}

void SSD_Device::Report_results_in_XML(std::string name_prefix, Utils::XmlWriter& xmlwriter)
{
	std::string tmp;
	tmp = ID();
	xmlwriter.Write_open_tag(tmp);

	if (Memory_Type == NVM::NVM_Type::FLASH)
	{
		((SSD_Components::FTL*)this->Firmware)->Report_results_in_XML(ID(), xmlwriter);
		((SSD_Components::FTL*)this->Firmware)->TSU->Report_results_in_XML(ID(), xmlwriter);
	}
	xmlwriter.Write_close_tag();
}