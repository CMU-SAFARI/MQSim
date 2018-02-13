#ifndef EXECUTION_PARAMETER_SET_H
#define EXECUTION_PARAMETER_SET_H

#include <vector>
#include "Device_Parameter_Set.h"
#include "IO_Flow_Parameter_Set.h"
#include "Host_Parameter_Set.h"

class Execution_Parameter_Set
{
public:
	static Host_Parameter_Set Host_Configuration;
	static Device_Parameter_Set SSD_Device_Configuration;
};

#endif