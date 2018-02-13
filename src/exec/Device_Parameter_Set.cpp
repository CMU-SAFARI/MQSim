#include "Device_Parameter_Set.h"



HostInterfaceType Device_Parameter_Set::HostInterface_Type = HostInterfaceType::NVME;
uint16_t Device_Parameter_Set::IO_Queue_Depth = 1024;//For NVMe, it determines the size of the submission/completion queues; for SATA, it determines the size of NCQ
uint16_t Device_Parameter_Set::Queue_Fetch_Size = 512;//Used in NVMe host interface
SSD_Components::Cache_Sharing_Mode Device_Parameter_Set::Data_Cache_Sharing_Mode = SSD_Components::Cache_Sharing_Mode::SHARED;//Data cache sharing among concurrently running I/O flows, if NVMe host interface is used
unsigned int Device_Parameter_Set::Data_Cache_Capacity = 1024 * 1024 * 512;//Data cache capacity in bytes
unsigned int Device_Parameter_Set::Data_Cache_DRAM_Row_Size = 8192;//The row size of DRAM in the data cache, the unit is bytes
unsigned int Device_Parameter_Set::Data_Cache_DRAM_Data_Rate = 800;//Data access rate to access DRAM in the data cache, the unit is MT/s
unsigned int Device_Parameter_Set::Data_Cache_DRAM_Data_Busrt_Size = 4;//The number of bytes that are transferred in one burst (it depends on the number of DRAM chips)
sim_time_type Device_Parameter_Set::Data_Cache_DRAM_tRCD = 13;//tRCD parameter to access DRAM in the data cache, the unit is nano-seconds
sim_time_type Device_Parameter_Set::Data_Cache_DRAM_tCL = 13;//tCL parameter to access DRAM in the data cache, the unit is nano-seconds
sim_time_type Device_Parameter_Set::Data_Cache_DRAM_tRP = 13;//tRP parameter to access DRAM in the data cache, the unit is nano-seconds
SSD_Components::Flash_Address_Mapping_Type Device_Parameter_Set::Address_Mapping = SSD_Components::Flash_Address_Mapping_Type::PAGE_LEVEL;
unsigned int Device_Parameter_Set::CMT_Capacity = 2 * 1024 * 1024;//Size of SRAM/DRAM space that is used to cache address mapping table in bytes
SSD_Components::CMT_Sharing_Mode Device_Parameter_Set::CMT_Sharing_Mode = SSD_Components::CMT_Sharing_Mode::SHARED;//How the entire CMT space is shared among concurrently running flows
SSD_Components::Flash_Plane_Allocation_Scheme_Type Device_Parameter_Set::Plane_Allocation_Scheme = SSD_Components::Flash_Plane_Allocation_Scheme_Type::CWDP;
SSD_Components::Flash_Scheduling_Type Device_Parameter_Set::Transaction_Scheduling_Policy = SSD_Components::Flash_Scheduling_Type::OUT_OF_ORDER;
double Device_Parameter_Set::Overprovisioning_Ratio = 0.02;//The ratio of spare space with respect to the whole available storage space of SSD
double Device_Parameter_Set::GC_Exect_Threshold = 0.05;//The threshold for the ratio of free pages that used to trigger GC
SSD_Components::GC_Block_Selection_Policy_Type Device_Parameter_Set::GC_Block_Selection_Policy = SSD_Components::GC_Block_Selection_Policy_Type::RGA;
bool Device_Parameter_Set::Preemptible_GC_Enabled = true;
double Device_Parameter_Set::GC_Hard_Threshold = 0.005;//The hard gc execution threshold, used to stop preemptible gc execution
sim_time_type Device_Parameter_Set::Prefered_suspend_erase_time_for_read = 700000;//in nano-seconds
sim_time_type Device_Parameter_Set::Prefered_suspend_erase_time_for_write = 700000;//in nano-seconds
sim_time_type Device_Parameter_Set::Prefered_suspend_write_time_for_read = 100000;//in nano-seconds
unsigned int Device_Parameter_Set::Flash_Channel_Count = 8;
unsigned int Device_Parameter_Set::Flash_Channel_Width = 1;//Channel width in byte
unsigned int Device_Parameter_Set::Channel_Transfer_Rate = 300;//MT/s
unsigned int Device_Parameter_Set::Chip_No_Per_Channel = 4;
SSD_Components::ONFI_Protocol Device_Parameter_Set::Flash_Comm_Protocol = SSD_Components::ONFI_Protocol::NVDDR2;
Flash_Parameter_Set Device_Parameter_Set::Flash_Parameters;
