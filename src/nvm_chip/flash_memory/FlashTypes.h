#ifndef FLASH_TYPES_H
#define FLASH_TYPES_H

#include<cstdint>
#include "../../sim/Sim_Defs.h"
#include "../NVM_Types.h"

namespace NVM
{
	namespace FlashMemory
	{
		enum class Command_Suspension_Mode { NONE, PROGRAM, PROGRAM_ERASE, ERASE };
	}
}

typedef uint64_t page_status_type;
#define FULL_PROGRAMMED_PAGE 0xffffffffffffffffULL
typedef uint32_t flash_channel_ID_type;
typedef uint32_t flash_chip_ID_type;
typedef uint32_t flash_die_ID_type;
typedef uint32_t flash_plane_ID_type;
typedef uint32_t flash_block_ID_type;
typedef uint32_t flash_page_ID_type;
typedef uint64_t LPA_type;
typedef uint64_t PPA_type;
typedef uint64_t command_code_type;

enum class Flash_Technology_Type { SLC = 1, MLC = 2, TLC = 3 };

#define FREE_PAGE 0x0000000000000000ULL
#define NO_LPA 0xffffffffffffffffULL
#define NO_STREAM 0xff
#define UNWRITTEN_LOGICAL_PAGE 0x0000000000000000ULL
#define NO_PPA 0xffffffffffffffffULL
#define NO_MPPN 0xffffffffULL

#endif // !FLASH_TYPES_H
