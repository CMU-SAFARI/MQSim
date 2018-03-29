#include "Plane.h"

namespace NVM
{
	namespace FlashMemory
	{
		Plane::Plane(unsigned int BlocksNoPerPlane, unsigned int PagesNoPerBlock) :
			ReadCount(0), ProgamCount(0), Erase_count(0)
		{
			HealthyBlockNo = BlocksNoPerPlane;
			Blocks = new Block*[BlocksNoPerPlane];
			for (unsigned int i = 0; i < BlocksNoPerPlane; i++)
				Blocks[i] = new Block(PagesNoPerBlock, i);
			AllocatedStreams = NULL;
		}

		Plane::~Plane()
		{
			for (unsigned int i = 0; i < HealthyBlockNo; i++)
				delete Blocks[i];
			delete[] Blocks;
		}
	}
}