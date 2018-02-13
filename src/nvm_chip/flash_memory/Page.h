#ifndef PAGE_H
#define PAGE_H

#include "FlashTypes.h"

namespace NVM
{
	namespace FlashMemory
	{
		//Since MQSim does not actually use physical page's metadata , metada is empty to save simulation space
		class Page {};
		/*
		struct PageMetadata
		{
			page_status_type Status;
			LPA_type LPA;
			stream_id_type SourceStreamID;
		};

		class Page {
		public:
			Page();
			PageMetadata Metadata;
		};
		Page::Page()
		{
			Metadata.Status = FREE_PAGE;
			Metadata.LPA = INVALID_LPN;
			Metadata.SourceStreamID = NO_STREAM;
		}*/
	}
}

#endif // !PAGE_H
