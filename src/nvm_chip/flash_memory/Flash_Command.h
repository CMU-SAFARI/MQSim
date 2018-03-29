#ifndef FLASH_COMMAND_H
#define FLASH_COMMAND_H

#include <vector>
#include "FlashTypes.h"
#include "Physical_Page_Address.h"
#include "Page.h"


#define CMD_READ 0x0030
#define CMD_READ_PAGE 0x0030
#define CMD_READ_PAGE_CACHE_SEQ 0x31
#define CMD_READ_PAGE_CACHE_RANDOM 0x0031
#define CMD_READ_PAGE_MULTIPLANE 0x0032
#define CMD_READ_PAGE_COPYBACK 0x0035
#define CMD_READ_PAGE_COPYBACK_MULTIPLANE 0x00321 //since the codes of multiplane copyback (i.e., 0x0032) and multiplane read (i.e., 0x0032) are identical, we change the code of multiplane copyback 0x0032 to 0x00321 so that we can differentiate copyback multiplane from normal multiplane
#define CMD_PROGRAM 0x8000
#define CMD_PROGRAM_PAGE 0x8010
#define CMD_PROGRAM_PAGE_MULTIPLANE 0x8011
#define CMD_PROGRAM_PAGE_COPYBACK 0x8510
#define CMD_PROGRAM_PAGE_COPYBACK_MULTIPLANE 0x85111 //since the codes of multiplane copyback (i.e., 0x8511) and multiplane program (i.e., 0x8511) are identical, we change the code of multiplane copyback 0x8511 to 0x85111 so that we can differentiate copyback multiplane from normal multiplane
//#define CMD_SUSPEND_PROGRAM 0x86
//#define CMD_RESUME_PROGRAM 0xD1
#define CMD_ERASE 0x6000
#define CMD_ERASE_BLOCK 0x60D0
#define CMD_ERASE_BLOCK_MULTIPLANE 0x60D1
//#define CMD_SUSPEND_ERASE 0x61
//#define CMD_RESUME_ERASE 0xD2

namespace NVM
{
	namespace FlashMemory
	{
		class Flash_Command
		{
		public:
			command_code_type CommandCode;
			std::vector<Physical_Page_Address> Address;
			std::vector<PageMetadata> Meta_data;
		};
	}
}
#endif // !FLASH_COMMAND
