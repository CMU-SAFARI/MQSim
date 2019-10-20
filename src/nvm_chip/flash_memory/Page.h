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
		};

		class Page {
		public:
			Page()
			{
				//Metadata.Status = FREE_PAGE;
				Metadata.LPA = NO_LPA;
				//Metadata.SourceStreamID = NO_STREAM;
			};
			
			PageMetadata Metadata;

			void Write_metadata(const PageMetadata& metadata)
			{
				this->Metadata.LPA = metadata.LPA;
			}
			
			void Read_metadata(PageMetadata& metadata)
			{
				metadata.LPA = this->Metadata.LPA;
			}
		};
	}
}

#endif // !PAGE_H
