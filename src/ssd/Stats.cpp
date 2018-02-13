#include "Stats.h"


namespace SSD_Components
{
	unsigned long Stats::IssuedReadCMD = 0;
	unsigned long Stats::IssuedCopybackReadCMD = 0;
	unsigned long Stats::IssuedInterleaveReadCMD = 0;
	unsigned long Stats::IssuedMultiplaneReadCMD = 0;;
	unsigned long Stats::IssuedMultiplaneCopybackReadCMD = 0;;
	unsigned long Stats::IssuedProgramCMD = 0;;
	unsigned long Stats::IssuedInterleaveProgramCMD = 0;;
	unsigned long Stats::IssuedMultiplaneProgramCMD = 0;;
	unsigned long Stats::IssuedMultiplaneCopybackProgramCMD = 0;;
	unsigned long Stats::IssuedInterleaveMultiplaneProgramCMD = 0;
	unsigned long Stats::IssuedSuspendProgramCMD = 0;
	unsigned long Stats::IssuedCopybackProgramCMD = 0;
	unsigned long Stats::IssuedEraseCMD = 0;;
	unsigned long Stats::IssuedInterleaveEraseCMD = 0;;
	unsigned long Stats::IssuedMultiplaneEraseCMD = 0;;
	unsigned long Stats::IssuedInterleaveMultiplaneEraseCMD = 0;;
	unsigned long Stats::IssuedSuspendEraseCMD = 0;
	unsigned long Stats::TotalMappingReadRequests = 0;
	unsigned long Stats::TotalMappingWriteRequests = 0;
	unsigned long Stats::MappingReadRequests[MAX_SUPPORT_STREAMS] = { 0 };
	unsigned long Stats::MappingWriteRequests[MAX_SUPPORT_STREAMS] = { 0 };
}