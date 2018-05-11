#ifndef DISTRIBUTION_TYPES
#define DISTRIBUTION_TYPES

namespace Utils
{
	enum class Address_Distribution_Type { STREAMING, UNIFORM_RANDOM, HOTCOLD_RANDOM };
	enum class Request_Size_Distribution_Type { FIXED, NORMAL };
	enum class Workload_Type { SYNTHETIC, TRACE_BASED };
	enum class Request_Generator_Type { BANDWIDTH, QUEUE_DEPTH };//Time_INTERVAL: general requests based on the arrival rate definitions, DEMAND_BASED: just generate a request, every time that there is a demand
}
#endif