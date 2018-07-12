#ifndef DEVICE_PARAMETER_SET_H
#define DEVICE_PARAMETER_SET_H

#include "../ssd/SSD_Defs.h"
#include "../ssd/Host_Interface_Defs.h"
#include "../ssd/Host_Interface_Base.h"
#include "../ssd/Data_Cache_Manager_Base.h"
#include "../ssd/Address_Mapping_Unit_Base.h"
#include "../ssd/TSU_Base.h"
#include "../ssd/ONFI_Channel_Base.h"
#include "../ssd/GC_and_WL_Unit_Page_Level.h"
#include "../nvm_chip/NVM_Types.h"
#include "Parameter_Set_Base.h"
#include "Flash_Parameter_Set.h"

class Device_Parameter_Set : public Parameter_Set_Base
{
public:
	static int Seed;//Seed for random number generation (used in device's random number generators)
	static bool Enabled_Preconditioning;
	static NVM::NVM_Type Memory_Type;
	static HostInterface_Types HostInterface_Type;
	static uint16_t IO_Queue_Depth;//For NVMe, it determines the size of the submission/completion queues; for SATA, it determines the size of NCQ_Control_Structure
	static uint16_t Queue_Fetch_Size;//Used in NVMe host interface
	static SSD_Components::Caching_Mechanism Caching_Mechanism;
	static SSD_Components::Cache_Sharing_Mode Data_Cache_Sharing_Mode;//Data cache sharing among concurrently running I/O flows, if NVMe host interface is used
	static unsigned int Data_Cache_Capacity;//Data cache capacity in bytes
	static unsigned int Data_Cache_DRAM_Row_Size;//The row size of DRAM in the data cache, the unit is bytes
	static unsigned int Data_Cache_DRAM_Data_Rate;//Data access rate to access DRAM in the data cache, the unit is MT/s
	static unsigned int Data_Cache_DRAM_Data_Busrt_Size;//The number of bytes that are transferred in one burst (it depends on the number of DRAM chips)
	static sim_time_type Data_Cache_DRAM_tRCD;//tRCD parameter to access DRAM in the data cache, the unit is nano-seconds
	static sim_time_type Data_Cache_DRAM_tCL;//tCL parameter to access DRAM in the data cache, the unit is nano-seconds
	static sim_time_type Data_Cache_DRAM_tRP;//tRP parameter to access DRAM in the data cache, the unit is nano-seconds
	static SSD_Components::Flash_Address_Mapping_Type Address_Mapping;
	static bool Ideal_Mapping_Table;//If mapping is ideal, then all the mapping entries are found in the DRAM and there is no need to read mapping entries from flash
	static unsigned int CMT_Capacity;//Size of SRAM/DRAM space that is used to cache address mapping table, the unit is bytes
	static SSD_Components::CMT_Sharing_Mode CMT_Sharing_Mode;//How the entire CMT space is shared among concurrently running flows
	static SSD_Components::Flash_Plane_Allocation_Scheme_Type Plane_Allocation_Scheme;
	static SSD_Components::Flash_Scheduling_Type Transaction_Scheduling_Policy;
	static double Overprovisioning_Ratio;//The ratio of spare space with respect to the whole available storage space of SSD
	static double GC_Exec_Threshold;//The threshold for the ratio of free pages that used to trigger GC
	static SSD_Components::GC_Block_Selection_Policy_Type GC_Block_Selection_Policy;
	static bool Use_Copyback_for_GC;
	static bool Preemptible_GC_Enabled;
	static double GC_Hard_Threshold;//The hard gc execution threshold, used to stop preemptible gc execution
	static bool Dynamic_Wearleveling_Enabled;
	static bool Static_Wearleveling_Enabled;
	static unsigned int Static_Wearleveling_Threshold;
	static sim_time_type Preferred_suspend_erase_time_for_read;//in nano-seconds, if the remaining time of the ongoing erase is smaller than Prefered_suspend_erase_time_for_read, then the ongoing erase operation will be suspended
	static sim_time_type Preferred_suspend_erase_time_for_write;//in nano-seconds, if the remaining time of the ongoing erase is smaller than Prefered_suspend_erase_time_for_write, then the ongoing erase operation will be suspended
	static sim_time_type Preferred_suspend_write_time_for_read;//in nano-seconds, if the remaining time of the ongoing write is smaller than Prefered_suspend_write_time_for_read, then the ongoing erase operation will be suspended
	static unsigned int Flash_Channel_Count;
	static unsigned int Flash_Channel_Width;//Channel width in byte
	static unsigned int Channel_Transfer_Rate;//MT/s
	static unsigned int Chip_No_Per_Channel;
	static SSD_Components::ONFI_Protocol Flash_Comm_Protocol;
	static Flash_Parameter_Set Flash_Parameters;
	void XML_serialize(Utils::XmlWriter& xmlwriter);
	void XML_deserialize(rapidxml::xml_node<> *node);
};

#endif // !DEVICE_PARAMETER_SET_H
