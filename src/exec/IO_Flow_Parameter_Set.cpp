#include "IO_Flow_Parameter_Set.h"

/*
SSD_Components::Caching_Mode IO_Flow_Parameter_Set::Device_Level_Data_Caching_Mode = SSD_Components::Caching_Mode::WRITE_CACHE;
Flow_Type IO_Flow_Parameter_Set::Type = Flow_Type::SYNTHETIC;
IO_Flow_Priority_Class IO_Flow_Parameter_Set::Priority_Class = IO_Flow_Priority_Class::HIGH;//The priority class is only considered when the SSD device uses NVMe host interface
flash_channel_ID_type* IO_Flow_Parameter_Set::Channel_IDs;//Resource partitioning: which channel ids are allocated to this flow
flash_chip_ID_type* IO_Flow_Parameter_Set::Chip_IDs;//Resource partitioning: which chip ids are allocated to this flow
flash_die_ID_type* IO_Flow_Parameter_Set::Die_IDs;//Resource partitioning: which die ids are allocted to this flow
flash_plane_ID_type* IO_Flow_Parameter_Set::Plane_IDs;//Resource partitioning: which plane ids are allocated to this flow


double IO_Flow_Parameter_Set_Synthetic::Read_Ratio = 1.0;
Host_Components::Address_Distribution_Type IO_Flow_Parameter_Set_Synthetic::Address_Distribution = Host_Components::Address_Distribution_Type::STREAMING;
double IO_Flow_Parameter_Set_Synthetic::Ratio_of_Hot_Region = 0.0;//This parameters used if the address distribution type is hot/cold (i.e., (100-H)% of the whole I/O requests are going to a H% hot region of the storage space)
Host_Components::Request_Size_Distribution_Type IO_Flow_Parameter_Set_Synthetic::Request_Size_Distribution = Host_Components::Request_Size_Distribution_Type::Fixed;
unsigned int IO_Flow_Parameter_Set_Synthetic::Average_Request_Size = 16;//Average request size in sectors
unsigned int IO_Flow_Parameter_Set_Synthetic::Variance_Request_Size = 0;//Variance of request size in sectors
int IO_Flow_Parameter_Set_Synthetic::Seed = 19283;
unsigned int IO_Flow_Parameter_Set_Synthetic::Average_No_of_Reqs_in_Queue = 2;//Average number of I/O requests from this flow in the 
sim_time_type IO_Flow_Parameter_Set_Synthetic::Stop_Time = 1000000000;//Defines when to stop generating I/O requests
unsigned int IO_Flow_Parameter_Set_Synthetic::Total_Request_To_Generate = 0;//If Stop_Time is equal to zero, then requst generator considers Total_Request_To_Generate to decide when to stop generating I/O requests


std::string IO_Flow_Parameter_Set_Trace_Based::File_Path;
int IO_Flow_Parameter_Set_Trace_Based::Percentage_To_Be_Executed = 100;
*/