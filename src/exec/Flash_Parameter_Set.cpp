#include "Flash_Parameter_Set.h"

Flash_Technology_Type Flash_Parameter_Set::Flash_Technology = Flash_Technology_Type::MLC;
NVM::FlashMemory::Command_Suspension_Mode Flash_Parameter_Set::CMD_Suspension_Support = NVM::FlashMemory::Command_Suspension_Mode::ERASE;
sim_time_type Flash_Parameter_Set::Page_Read_Latency_LSB = 75000;
sim_time_type Flash_Parameter_Set::Page_Read_Latency_CSB = 75000;
sim_time_type Flash_Parameter_Set::Page_Read_Latency_MSB = 75000;
sim_time_type Flash_Parameter_Set::Page_Program_Latency_LSB = 750000;
sim_time_type Flash_Parameter_Set::Page_Program_Latency_CSB = 75000;
sim_time_type Flash_Parameter_Set::Page_Program_Latency_MSB = 750000;
sim_time_type Flash_Parameter_Set::Block_Erase_Latency;//Block erase latency in nano-seconds
unsigned int Flash_Parameter_Set::Block_PE_Cycles_Limit = 10000;
sim_time_type Flash_Parameter_Set::Suspend_Erase_Time = 700000;//in nano-seconds
sim_time_type Flash_Parameter_Set::Suspend_Write_Time = 100000;//in nano-seconds
unsigned int Flash_Parameter_Set::Die_No_Per_Chip = 2;
unsigned int Flash_Parameter_Set::Plane_No_Per_Die = 2;
unsigned int Flash_Parameter_Set::Block_No_Per_Plane = 2048;
unsigned int Flash_Parameter_Set::Page_No_Per_Block = 256;//Page no per block
unsigned int Flash_Parameter_Set::Page_Capacity = 8192;//Flash page capacity in bytes
unsigned int Flash_Parameter_Set::Page_Metadat_Capacity = 8192;//Flash page capacity in bytes
