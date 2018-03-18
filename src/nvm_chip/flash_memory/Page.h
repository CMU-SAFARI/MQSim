#ifndef PAGE_H
#define PAGE_H

#include "FlashTypes.h"

namespace NVM
{
	namespace FlashMemory
	{
		
		struct PageMetadata
		{
			//page_status_type Status;
			LPA_type LPA;
			//stream_id_type SourceStreamID;
		};

		class Page {
		public:
			Page()
			{
				//Metadata.Status = FREE_PAGE;
				Metadata.LPA = INVALID_LPN;
				//Metadata.SourceStreamID = NO_STREAM;
			};
			PageMetadata Metadata;
		};
	}
}

#endif // !PAGE_H
