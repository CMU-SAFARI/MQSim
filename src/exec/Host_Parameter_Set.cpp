#include "Host_Parameter_Set.h"


double Host_Parameter_Set::PCIe_Lane_Bandwidth = 0.4;//uint is GB/s
unsigned int Host_Parameter_Set::PCIe_Lane_Count = 4;
std::vector<IO_Flow_Parameter_Set*> Host_Parameter_Set::IO_Flow_Definitions;