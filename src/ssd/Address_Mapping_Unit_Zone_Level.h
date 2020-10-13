#ifndef ADDRESS_MAPPING_UNIT_ZONE_LEVEL
#define ADDRESS_MAPPING_UNIT_ZONE_LEVEL

#include <unordered_map>
#include <map>
#include <queue>
#include <set>
#include <list>
#include "Address_Mapping_Unit_Base.h"
#include "Address_Mapping_Unit_Page_Level.h"
#include "Flash_Block_Manager_Base.h"
#include "SSD_Defs.h"
#include "NVM_Transaction_Flash_RD.h"
#include "NVM_Transaction_Flash_WR.h"

namespace SSD_Components
{
	class Address_Mapping_Unit_Zone_Level : public Address_Mapping_Unit_Page_Level
	{
		friend class GC_and_WL_Unit_Page_Level;
		//friend class GC_and_WL_Unit_Zone_Level;
	public:
		Address_Mapping_Unit_Zone_Level(const sim_object_id_type& id, FTL* ftl, 
			NVM_PHY_ONFI* flash_controller, Flash_Block_Manager_Base* block_manager, Flash_Zone_Manager_Base* zone_manager,
			bool ideal_mapping_table, unsigned int cmt_capacity_in_byte, 
			Flash_Plane_Allocation_Scheme_Type PlaneAllocationScheme,
			Zone_Allocation_Scheme_Type ZoneAllocationScheme,
			SubZone_Allocation_Scheme_Type SubZoneAllocationScheme,
			unsigned int ConcurrentStreamNo,
			unsigned int ChannelCount, unsigned int chip_no_per_channel, 
			unsigned int DieNoPerChip, unsigned int PlaneNoPerDie,
			unsigned int ChannelNoPerZone, unsigned int ChipNoPerZone,
			unsigned int DieNoPerZone, unsigned int PlaneNoPerZone,
			std::vector<std::vector<flash_channel_ID_type>> stream_channel_ids, 
			std::vector<std::vector<flash_chip_ID_type>> stream_chip_ids,
			std::vector<std::vector<flash_die_ID_type>> stream_die_ids, 
			std::vector<std::vector<flash_plane_ID_type>> stream_plane_ids,
			unsigned int Block_no_per_plane, unsigned int Page_no_per_block, 
			unsigned int SectorsPerPage, unsigned int PageSizeInBytes,
			double Overprovisioning_ratio, CMT_Sharing_Mode sharing_mode = CMT_Sharing_Mode::SHARED, 
			bool fold_large_addresses = true,
			bool Support_Zone = true);
		~Address_Mapping_Unit_Zone_Level();

	private:
		static Address_Mapping_Unit_Zone_Level* _my_instance;
		Flash_Zone_Manager_Base *fzm;
		NVM::FlashMemory::Zone **zones;

		void allocate_plane_for_user_write(NVM_Transaction_Flash_WR* transaction);
		void allocate_page_in_plane_for_user_write(NVM_Transaction_Flash_WR* transaction, bool is_for_gc);
		void allocate_plane_for_preconditioning(stream_id_type stream_id, LPA_type lpn, NVM::FlashMemory::Physical_Page_Address& targetAddress);

	};

}

#endif // !ADDRESS_MAPPING_UNIT_ZONE_LEVEL
