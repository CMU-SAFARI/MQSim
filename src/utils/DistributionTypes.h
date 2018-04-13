#ifndef DISTRIBUTION_TYPES
#define DISTRIBUTION_TYPES

namespace Utils
{
	enum class Address_Distribution_Type { STREAMING, UNIFORM_RANDOM, HOTCOLD_RANDOM };
	enum class Request_Size_Distribution_Type { FIXED, NORMAL };
	enum class Workload_Type { SYNTHETIC, TRACE_BASED };
}
#endif