#ifndef PLANE_H
#define PLANE_H

#include "FlashTypes.h"
#include "Block.h"
#include "Flash_Command.h"

namespace NVM
{
	namespace FlashMemory
	{
		class Plane
		{
		public:
			Plane(unsigned int BlocksNoPerPlane, unsigned int PagesNoPerBlock);
			~Plane();
			Block** Blocks;
			unsigned int HealthyBlockNo;
			unsigned long ReadCount;                     //how many read count in the process of workload
			unsigned long ProgamCount;
			unsigned long EraseCount;
			stream_id_type* AllocatedStreams;
		};
	}
}
#endif // !PLANE_H
