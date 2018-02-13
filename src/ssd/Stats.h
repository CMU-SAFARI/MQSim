#ifndef STATS_H
#define STATS_H

#include "SSDTypes.h"

namespace SSD_Components
{
	class Stats
	{
	public:
		static unsigned long IssuedReadCMD, IssuedCopybackReadCMD, IssuedInterleaveReadCMD, IssuedMultiplaneReadCMD, IssuedMultiplaneCopybackReadCMD;
		static unsigned long IssuedProgramCMD, IssuedInterleaveProgramCMD, IssuedMultiplaneProgramCMD, IssuedInterleaveMultiplaneProgramCMD, IssuedCopybackProgramCMD, IssuedMultiplaneCopybackProgramCMD;
		static unsigned long IssuedEraseCMD, IssuedInterleaveEraseCMD, IssuedMultiplaneEraseCMD, IssuedInterleaveMultiplaneEraseCMD;
		static unsigned long IssuedSuspendProgramCMD, IssuedSuspendEraseCMD;

		static unsigned long TotalMappingReadRequests, TotalMappingWriteRequests;
		static unsigned long MappingReadRequests[MAX_SUPPORT_STREAMS], MappingWriteRequests[MAX_SUPPORT_STREAMS];
	};
}

#endif // !STATS_H
