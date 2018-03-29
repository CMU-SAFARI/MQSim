#ifndef STATS_H
#define STATS_H

#include "SSD_Defs.h"

namespace SSD_Components
{
	class Stats
	{
	public:
		void Init_stats(unsigned int channel_no, unsigned int chip_no_per_channel, unsigned int die_no_per_chip, unsigned int plane_no_per_die, unsigned int block_no_per_plane, unsigned int page_no_per_block, unsigned int max_allowed_block_erase_count);
		static unsigned long IssuedReadCMD, IssuedCopybackReadCMD, IssuedInterleaveReadCMD, IssuedMultiplaneReadCMD, IssuedMultiplaneCopybackReadCMD;
		static unsigned long IssuedProgramCMD, IssuedInterleaveProgramCMD, IssuedMultiplaneProgramCMD, IssuedInterleaveMultiplaneProgramCMD, IssuedCopybackProgramCMD, IssuedMultiplaneCopybackProgramCMD;
		static unsigned long IssuedEraseCMD, IssuedInterleaveEraseCMD, IssuedMultiplaneEraseCMD, IssuedInterleaveMultiplaneEraseCMD;
		static unsigned long IssuedSuspendProgramCMD, IssuedSuspendEraseCMD;

		static unsigned long TotalMappingReadRequests, TotalMappingWriteRequests;
		static unsigned long MappingReadRequests[MAX_SUPPORT_STREAMS], MappingWriteRequests[MAX_SUPPORT_STREAMS];
		
		static unsigned int***** Block_erase_histogram;
	};
}

#endif // !STATS_H
