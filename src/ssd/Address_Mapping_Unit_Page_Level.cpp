#include <cmath>
#include <assert.h>

#include "Address_Mapping_Unit_Page_Level.h"
#include "Stats.h"

namespace SSD_Components
{
	Cached_Mapping_Table::Cached_Mapping_Table(unsigned int capacity) : capacity(capacity)
	{}

	inline bool Cached_Mapping_Table::Exists(const stream_id_type streamID, const LPA_type lpa)
	{
		LPA_type key = LPN_TO_UNIQUE_KEY(streamID, lpa);
		auto it = addressMap.find(key);
		if (it == addressMap.end())
		{
			DEBUG("Address mapping table query - Stream ID:" << streamID << ", LPA:" << lpa << ", MISS")
			return false;
		}
		if (it->second->Status != CMTEntryStatus::VALID)
		{
			return false;
			DEBUG("Address mapping table query - Stream ID:" << streamID << ", LPA:" << lpa << ", MISS")
		}
		DEBUG("Address mapping table query - Stream ID:" << streamID << ", LPA:" << lpa << ", HIT")
		return true;
	}
	PPA_type Cached_Mapping_Table::Rerieve_ppa(const stream_id_type streamID, const LPA_type lpn)
	{
		LPA_type key = LPN_TO_UNIQUE_KEY(streamID, lpn);
		auto it = addressMap.find(key);
		assert(it != addressMap.end());
		assert(it->second->Status == CMTEntryStatus::VALID);
		lruList.splice(lruList.begin(), lruList, it->second->listPtr);
		return it->second->PPA;
	}
	void Cached_Mapping_Table::Update_mapping_info(const stream_id_type streamID, const LPA_type lpa, const PPA_type ppa, const page_status_type pageWriteState)
	{
		LPA_type key = LPN_TO_UNIQUE_KEY(streamID, lpa);
		auto it = addressMap.find(key);
		assert(it != addressMap.end());
		assert(it->second->Status == CMTEntryStatus::VALID);

		it->second->PPA = ppa;
		it->second->WrittenStateBitmap = pageWriteState;
		it->second->Dirty = true;
		DEBUG("Address mapping table update entry - Stream ID:" << streamID << ", LPA:" << lpa << ", PPA:" << ppa)
	}
	void Cached_Mapping_Table::Insert_new_mapping_info(const stream_id_type streamID, const LPA_type lpa, const PPA_type ppa, const unsigned long long pageWriteState)
	{
		LPA_type key = LPN_TO_UNIQUE_KEY(streamID, lpa);
		auto it = addressMap.find(key);
		if (it == addressMap.end())
			throw "No slot is reserved!";

		it->second->Status = CMTEntryStatus::VALID;

		it->second->PPA = ppa;
		it->second->WrittenStateBitmap = pageWriteState;
		it->second->Dirty = false;
		DEBUG("Address mapping table insert entry - Stream ID:" << streamID << ", LPA:" << lpa << ", PPA:" << ppa)
	}
	page_status_type Cached_Mapping_Table::Get_bitmap_vector_of_written_sectors(const stream_id_type streamID, const LPA_type lpn)
	{
		LPA_type key = LPN_TO_UNIQUE_KEY(streamID, lpn);
		auto it = addressMap.find(key);
		assert(it != addressMap.end());
		assert(it->second->Status == CMTEntryStatus::VALID);
		return it->second->WrittenStateBitmap;
	}
	bool Cached_Mapping_Table::Is_slot_reserved_for_lpn_and_waiting(const stream_id_type streamID, const LPA_type lpn)
	{
		LPA_type key = LPN_TO_UNIQUE_KEY(streamID, lpn);
		auto it = addressMap.find(key);
		if (it != addressMap.end())
			if (it->second->Status == CMTEntryStatus::WAITING)
				return true;
		return false;
	}
	inline bool Cached_Mapping_Table::Check_free_slot_availability()
	{
		return addressMap.size() < capacity;
	}
	void Cached_Mapping_Table::Reserve_slot_for_lpn(const stream_id_type streamID, const LPA_type lpn)
	{
		LPA_type key = LPN_TO_UNIQUE_KEY(streamID, lpn);
		if (addressMap.find(key) != addressMap.end())
			throw "Duplicate lpa insertion into CMT!";
		if (addressMap.size() >= capacity)
			throw "CMT overfull!";

		CMTSlotType* cmtEnt = new CMTSlotType();
		cmtEnt->Dirty = false;
		lruList.push_front(std::pair<LPA_type, CMTSlotType*>(key, cmtEnt));
		cmtEnt->Status = CMTEntryStatus::WAITING;
		cmtEnt->listPtr = lruList.begin();
		addressMap[key] = cmtEnt;
	}
	CMTSlotType Cached_Mapping_Table::Evict_one_slot()
	{
		assert(addressMap.size() > 0);
		addressMap.erase(lruList.back().first);
		CMTSlotType evictedItem = *lruList.back().second;
		delete lruList.back().second;
		lruList.pop_back();
		return evictedItem;
	}
	bool Cached_Mapping_Table::Is_dirty(const stream_id_type streamID, const LPA_type lpn)
	{
		LPA_type key = LPN_TO_UNIQUE_KEY(streamID, lpn);
		auto it = addressMap.find(key);
		if (it == addressMap.end())
			throw "The requested slot does not exist!";

		return it->second->Dirty;
	}
	void Cached_Mapping_Table::Make_clean(const stream_id_type streamID, const LPA_type lpn)
	{
		LPA_type key = LPN_TO_UNIQUE_KEY(streamID, lpn);
		auto it = addressMap.find(key);
		if (it == addressMap.end())
			throw "The requested slot does not exist!";

		it->second->Dirty = false;
	}


	AddressMappingDomain::AddressMappingDomain(unsigned int cmt_capacity, unsigned int cmt_entry_size, unsigned int no_of_translation_entries_per_page,
		Cached_Mapping_Table* CMT,
		Flash_Plane_Allocation_Scheme_Type PlaneAllocationScheme,
		flash_channel_ID_type* ChannelIDs, unsigned int ChannelNo, flash_chip_ID_type* ChipIDs, unsigned int ChipNo,
		flash_die_ID_type* DieIDs, unsigned int DieNo, flash_plane_ID_type* PlaneIDs, unsigned int PlaneNo,
		double Share_per_plane,
		unsigned int Block_no_per_plane, unsigned int Page_no_per_block, unsigned int Sectors_no_per_page,
		double Overprovisioning_ratio) :
		CMT_entry_size(cmt_entry_size), Translation_entries_per_page(no_of_translation_entries_per_page),
		PlaneAllocationScheme(PlaneAllocationScheme),
		ChannelIDs(ChannelIDs), ChannelNo(ChannelNo), ChipIDs(ChipIDs), ChipNo(ChipNo), DieIDs(DieIDs), DieNo(DieNo), PlaneIDs(PlaneIDs), PlaneNo(PlaneNo),
		Block_no_per_plane(Block_no_per_plane), Page_no_per_block(Page_no_per_block), Sectors_no_per_page(Sectors_no_per_page), 
		Overprovisioning_ratio(Overprovisioning_ratio),
		STAT_CMT_hits(0), STAT_readTR_CMT_hits(0), STAT_writeTR_CMT_hits(0),
		STAT_CMT_miss(0), STAT_readTR_CMT_miss(0), STAT_writeTR_CMT_miss(0),
		STAT_total_CMT_queries(0), STAT_total_readTR_CMT_queries(0), STAT_total_writeTR_CMT_queries(0)
	{
		Pages_no_per_plane = (unsigned int)(((double)Page_no_per_block * Block_no_per_plane) * Share_per_plane);
		Pages_no_per_die = Pages_no_per_plane * PlaneNo;
		Pages_no_per_chip = Pages_no_per_die * DieNo;
		Pages_no_per_channel = Pages_no_per_chip * ChipNo;
		Total_physical_pages_no = Pages_no_per_channel * ChannelNo;
		Total_logical_pages_no = (unsigned int)((double)Total_physical_pages_no * (1 - Overprovisioning_ratio));
		max_logical_sector_address = (LSA_type)(Sectors_no_per_page * Total_logical_pages_no - 1);

		GlobalMappingTable = new GMTEntryType[Total_logical_pages_no];
		for (unsigned int i = 0; i < Total_logical_pages_no; i++)
		{
			GlobalMappingTable[i].PPA = NO_PPA;
			GlobalMappingTable[i].WrittenStateBitmap = UNWRITTEN_LOGICAL_PAGE;
			GlobalMappingTable[i].TimeStamp = INVALID_TIME_STAMP;
		}

		if (CMT == NULL)//If CMT is NULL, then each address mapping domain should create its own CMT
			this->CMT = new Cached_Mapping_Table(cmt_capacity);//Each flow (address mapping domain) has its own CMT, so CMT is create here in the constructor
		else this->CMT = CMT;//The entire CMT space is shared among concurrently running flows (i.e., address mapping domains of all flow)

		Total_translation_pages_no = Total_logical_pages_no / Translation_entries_per_page;
		GlobalTranslationDirectory = new GTDEntryType[Total_translation_pages_no + 1];
		for (unsigned int i = 0; i <= Total_translation_pages_no; i++)
		{
			GlobalTranslationDirectory[i].MPPN = (MPPN_type) NO_MPPN;
			GlobalTranslationDirectory[i].TimeStamp = INVALID_TIME_STAMP;
		}
	}

	Address_Mapping_Unit_Page_Level* Address_Mapping_Unit_Page_Level::_myInstance = NULL;

	Address_Mapping_Unit_Page_Level::Address_Mapping_Unit_Page_Level(const sim_object_id_type& id, FTL* ftl, NVM_PHY_ONFI* flash_controller, Flash_Block_Manager_Base* BlockManager,
		unsigned int cmt_capacity_in_byte, Flash_Plane_Allocation_Scheme_Type PlaneAllocationScheme,
		unsigned int ConcurrentStreamNo,
		unsigned int ChannelCount, unsigned int ChipNoPerChannel, unsigned int DieNoPerChip, unsigned int PlaneNoPerDie,
		unsigned int Block_no_per_plane, unsigned int Page_no_per_block, unsigned int SectorsPerPage, unsigned int PageSizeInByte,
		double Overprovisioning_ratio, CMT_Sharing_Mode SharingMode, bool fold_large_addresses)
		: Address_Mapping_Unit_Base(id, ftl, flash_controller, BlockManager,
			ConcurrentStreamNo, ChannelCount, ChipNoPerChannel, DieNoPerChip, PlaneNoPerDie,
			Block_no_per_plane, Page_no_per_block, SectorsPerPage, PageSizeInByte, Overprovisioning_ratio, fold_large_addresses),
		SharingMode(SharingMode)
	{
		_myInstance = this;
		domains = new AddressMappingDomain*[input_stream_no];
		/* Equal partitioning of all resources among concurrent running streams,
		*  For N streams:
		*  1. Each stream has 1/N share of CMT
		*  2. Each stream can access all channels, chips, dies, and planes
		*  3. Each stream has 1/N share of the blocks within a plane
		*/
		for (unsigned int domainID = 0; domainID < input_stream_no; domainID++)
		{
			flash_channel_ID_type* ChannelIDs = new flash_channel_ID_type[channel_count];
			for (unsigned int i = 0; i < channel_count; i++)
				ChannelIDs[i] = i;

			flash_channel_ID_type* ChipIDs = new flash_channel_ID_type[chip_no_per_channel];
			for (unsigned int i = 0; i < chip_no_per_channel; i++)
				ChipIDs[i] = i;
			
			flash_channel_ID_type* dieIDs = new flash_channel_ID_type[die_no_per_chip];
			for (unsigned int i = 0; i < die_no_per_chip; i++)
				dieIDs[i] = i;

			flash_channel_ID_type* planeIDs = new flash_channel_ID_type[plane_no_per_die];
			for (unsigned int i = 0; i < plane_no_per_die; i++)
				planeIDs[i] = i;

			/* Since we want to have the same mapping table entry size for all streams, the entry size
			*  is calculated at this level and then pass it to the constructors of mapping domains
			* entry size = sizeOf(lpa) + sizeOf(ppn) + sizeOf(bit vector that shows written sectors of a page)
			*/
			CMT_entry_size = (unsigned int) std::ceil(((2 * std::log2(total_physical_pages_no)) + sector_no_per_page) / 8);
			//In GTD we do not need to store lpa
			GTD_entry_size = (unsigned int) std::ceil((std::log2(total_physical_pages_no) + sector_no_per_page) / 8);
			no_of_translation_entries_per_page = (SectorsPerPage * SECTOR_SIZE_IN_BYTE) / GTD_entry_size;

			Cached_Mapping_Table* sharedCMT = NULL;
			unsigned int per_stream_cmt_capacity = 0;
			cmt_capacity = cmt_capacity_in_byte / CMT_entry_size;
			switch (SharingMode)
			{
			case CMT_Sharing_Mode::SHARED:
				per_stream_cmt_capacity = cmt_capacity;
				sharedCMT = new Cached_Mapping_Table(cmt_capacity);
				break;
			case CMT_Sharing_Mode::EQUAL_SIZE_PARTITIONING:
				per_stream_cmt_capacity = cmt_capacity / input_stream_no;
				break;
			}
			domains[domainID] = new AddressMappingDomain(per_stream_cmt_capacity, CMT_entry_size, no_of_translation_entries_per_page,
				sharedCMT,
				PlaneAllocationScheme,
				ChannelIDs, channel_count, ChipIDs, chip_no_per_channel, dieIDs, die_no_per_chip, planeIDs, plane_no_per_die,
				1/(double)input_stream_no, block_no_per_plane, pages_no_per_block, sector_no_per_page, overprovisioning_ratio);
		}
	}

	void Address_Mapping_Unit_Page_Level::Setup_triggers()
	{
		Sim_Object::Setup_triggers();
		flash_controller->ConnectToTransactionServicedSignal(handle_transaction_serviced_signal_from_PHY);
	}

	void Address_Mapping_Unit_Page_Level::Start_simulation() 
	{
		prepare_mapping_table();
	}

	void Address_Mapping_Unit_Page_Level::Validate_simulation_config() {}

	void Address_Mapping_Unit_Page_Level::Execute_simulator_event(MQSimEngine::Sim_Event* event) {}

	void Address_Mapping_Unit_Page_Level::Translate_lpa_to_ppa_and_dispatch(const std::list<NVM_Transaction*>& transactionList)
	{
		for (std::list<NVM_Transaction*>::const_iterator it = transactionList.begin();
			it != transactionList.end(); it++)
			check_and_translate((NVM_Transaction_Flash*)(*it));

		ftl->TSU->Prepare_for_transaction_submit();
		for (std::list<NVM_Transaction*>::const_iterator it = transactionList.begin();
			it != transactionList.end(); it++)
			if(((NVM_Transaction_Flash*)(*it))->Physical_address_determined)
				ftl->TSU->Submit_transaction(static_cast<NVM_Transaction_Flash*>(*it));
		ftl->TSU->Schedule();
	}

	bool Address_Mapping_Unit_Page_Level::Check_address_range(stream_id_type streamID, LPA_type lsn, unsigned int size)
	{
		if (fold_large_addresses)
			return true;

		if ((lsn + size * sector_no_per_page) <= domains[streamID]->max_logical_sector_address)
			return true;
		else
			return false;
	}

	PPA_type Address_Mapping_Unit_Page_Level::Online_create_entry_for_reads(LPA_type lpa, const stream_id_type stream_id, NVM::FlashMemory::Physical_Page_Address& read_address, uint64_t read_sectors_bitmap)
	{
		AddressMappingDomain* domain = domains[stream_id];
		switch (domain->PlaneAllocationScheme)
		{
			//Static: Channel first
		case Flash_Plane_Allocation_Scheme_Type::CWDP:
			read_address.ChannelID = domain->ChannelIDs[(unsigned int)(lpa % domain->ChannelNo)];
			read_address.ChipID = domain->ChipIDs[(unsigned int)((lpa / domain->ChannelNo) % domain->ChipNo)];
			read_address.DieID = domain->DieIDs[(unsigned int)((lpa / (domain->ChannelNo * domain->ChipNo)) % domain->DieNo)];
			read_address.PlaneID = domain->PlaneIDs[(unsigned int)((lpa / (domain->ChannelNo * domain->ChipNo * domain->DieNo)) % domain->PlaneNo)];
			break;
		case Flash_Plane_Allocation_Scheme_Type::CWPD:
			read_address.ChannelID = domain->ChannelIDs[(unsigned int)(lpa % domain->ChannelNo)];
			read_address.ChipID = domain->ChipIDs[(unsigned int)((lpa / domain->ChannelNo) % domain->ChipNo)];
			read_address.DieID = domain->DieIDs[(unsigned int)((lpa / (domain->ChannelNo * domain->ChipNo * domain->PlaneNo)) % domain->DieNo)];
			read_address.PlaneID = domain->PlaneIDs[(unsigned int)((lpa / (domain->ChannelNo * domain->ChipNo)) % domain->PlaneNo)];
			break;
		case Flash_Plane_Allocation_Scheme_Type::CDWP:
			read_address.ChannelID = domain->ChannelIDs[(unsigned int)(lpa % domain->ChannelNo)];
			read_address.ChipID = domain->ChipIDs[(unsigned int)((lpa / (domain->ChannelNo * domain->DieNo)) % domain->ChipNo)];
			read_address.DieID = domain->DieIDs[(unsigned int)((lpa / domain->ChannelNo) % domain->DieNo)];
			read_address.PlaneID = domain->PlaneIDs[(unsigned int)((lpa / (domain->ChannelNo * domain->DieNo * domain->ChipNo)) % domain->PlaneNo)];
			break;
		case Flash_Plane_Allocation_Scheme_Type::CDPW:
			read_address.ChannelID = domain->ChannelIDs[(unsigned int)(lpa % domain->ChannelNo)];
			read_address.ChipID = domain->ChipIDs[(unsigned int)((lpa / (domain->ChannelNo * domain->DieNo * domain->PlaneNo)) % domain->ChipNo)];
			read_address.DieID = domain->DieIDs[(unsigned int)((lpa / domain->ChannelNo) % domain->DieNo)];
			read_address.PlaneID = domain->PlaneIDs[(unsigned int)((lpa / (domain->ChannelNo * domain->DieNo)) % domain->PlaneNo)];
			break;
		case Flash_Plane_Allocation_Scheme_Type::CPWD:
			read_address.ChannelID = domain->ChannelIDs[(unsigned int)(lpa % domain->ChannelNo)];
			read_address.ChipID = domain->ChipIDs[(unsigned int)((lpa / (domain->ChannelNo * domain->PlaneNo)) % domain->ChipNo)];
			read_address.DieID = domain->DieIDs[(unsigned int)((lpa / (domain->ChannelNo * domain->PlaneNo * domain->ChipNo)) % domain->DieNo)];
			read_address.PlaneID = domain->PlaneIDs[(unsigned int)((lpa / domain->ChannelNo) % domain->PlaneNo)];
			break;
		case Flash_Plane_Allocation_Scheme_Type::CPDW:
			read_address.ChannelID = domain->ChannelIDs[(unsigned int)(lpa % domain->ChannelNo)];
			read_address.ChipID = domain->ChipIDs[(unsigned int)((lpa / (domain->ChannelNo * domain->PlaneNo * domain->DieNo)) % domain->ChipNo)];
			read_address.DieID = domain->DieIDs[(unsigned int)((lpa / (domain->ChannelNo * domain->PlaneNo)) % domain->DieNo)];
			read_address.PlaneID = domain->PlaneIDs[(unsigned int)((lpa / domain->ChannelNo) % domain->PlaneNo)];
			break;
			//Static: Way first
		case Flash_Plane_Allocation_Scheme_Type::WCDP:
			read_address.ChannelID = domain->ChannelIDs[(unsigned int)((lpa / domain->ChipNo) % domain->ChannelNo)];
			read_address.ChipID = domain->ChipIDs[(unsigned int)(lpa % domain->ChipNo)];
			read_address.DieID = domain->DieIDs[(unsigned int)((lpa / (domain->ChipNo * domain->ChannelNo)) % domain->DieNo)];
			read_address.PlaneID = domain->PlaneIDs[(unsigned int)((lpa / (domain->ChipNo * domain->ChannelNo * domain->DieNo)) % domain->PlaneNo)];
			break;
		case Flash_Plane_Allocation_Scheme_Type::WCPD:
			read_address.ChannelID = domain->ChannelIDs[(unsigned int)((lpa / domain->ChipNo) % domain->ChannelNo)];
			read_address.ChipID = domain->ChipIDs[(unsigned int)(lpa % domain->ChipNo)];
			read_address.DieID = domain->DieIDs[(unsigned int)((lpa / (domain->ChipNo * domain->ChannelNo * domain->PlaneNo)) % domain->DieNo)];
			read_address.PlaneID = domain->PlaneIDs[(unsigned int)((lpa / (domain->ChipNo * domain->ChannelNo)) % domain->PlaneNo)];
			break;
		case Flash_Plane_Allocation_Scheme_Type::WDCP:
			read_address.ChannelID = domain->ChannelIDs[(unsigned int)((lpa / (domain->ChipNo * domain->DieNo)) % domain->ChannelNo)];
			read_address.ChipID = domain->ChipIDs[(unsigned int)(lpa % domain->ChipNo)];
			read_address.DieID = domain->DieIDs[(unsigned int)((lpa / domain->ChipNo) % domain->DieNo)];
			read_address.PlaneID = domain->PlaneIDs[(unsigned int)((lpa / (domain->ChipNo * domain->DieNo * domain->ChannelNo)) % domain->PlaneNo)];
			break;
		case Flash_Plane_Allocation_Scheme_Type::WDPC:
			read_address.ChannelID = domain->ChannelIDs[(unsigned int)((lpa / (domain->ChipNo * domain->DieNo * domain->PlaneNo)) % domain->ChannelNo)];
			read_address.ChipID = domain->ChipIDs[(unsigned int)(lpa % domain->ChipNo)];
			read_address.DieID = domain->DieIDs[(unsigned int)((lpa / domain->ChipNo) % domain->DieNo)];
			read_address.PlaneID = domain->PlaneIDs[(unsigned int)((lpa / (domain->ChipNo * domain->DieNo)) % domain->PlaneNo)];
			break;
		case Flash_Plane_Allocation_Scheme_Type::WPCD:
			read_address.ChannelID = domain->ChannelIDs[(unsigned int)((lpa / (domain->ChipNo * domain->PlaneNo)) % domain->ChannelNo)];
			read_address.ChipID = domain->ChipIDs[(unsigned int)(lpa % domain->ChipNo)];
			read_address.DieID = domain->DieIDs[(unsigned int)((lpa / (domain->ChipNo * domain->PlaneNo * domain->ChannelNo)) % domain->DieNo)];
			read_address.PlaneID = domain->PlaneIDs[(unsigned int)((lpa / domain->ChipNo) % domain->PlaneNo)];
			break;
		case Flash_Plane_Allocation_Scheme_Type::WPDC:
			read_address.ChannelID = domain->ChannelIDs[(unsigned int)((lpa / (domain->ChipNo * domain->PlaneNo * domain->DieNo)) % domain->ChannelNo)];
			read_address.ChipID = domain->ChipIDs[(unsigned int)(lpa % domain->ChipNo)];
			read_address.DieID = domain->DieIDs[(unsigned int)((lpa / (domain->ChipNo * domain->PlaneNo)) % domain->DieNo)];
			read_address.PlaneID = domain->PlaneIDs[(unsigned int)((lpa / domain->ChipNo) % domain->PlaneNo)];
			break;
			//Static: Die first
		case Flash_Plane_Allocation_Scheme_Type::DCWP:
			read_address.ChannelID = domain->ChannelIDs[(unsigned int)((lpa / domain->DieNo) % domain->ChannelNo)];
			read_address.ChipID = domain->ChipIDs[(unsigned int)((lpa / (domain->DieNo * domain->ChannelNo)) % domain->ChipNo)];
			read_address.DieID = domain->DieIDs[(unsigned int)(lpa % domain->DieNo)];
			read_address.PlaneID = domain->PlaneIDs[(unsigned int)((lpa / (domain->DieNo * domain->ChannelNo * domain->ChipNo)) % domain->PlaneNo)];
			break;
		case Flash_Plane_Allocation_Scheme_Type::DCPW:
			read_address.ChannelID = domain->ChannelIDs[(unsigned int)((lpa / domain->DieNo) % domain->ChannelNo)];
			read_address.ChipID = domain->ChipIDs[(unsigned int)((lpa / (domain->DieNo * domain->ChannelNo * domain->PlaneNo)) % domain->ChipNo)];
			read_address.DieID = domain->DieIDs[(unsigned int)(lpa % domain->DieNo)];
			read_address.PlaneID = domain->PlaneIDs[(unsigned int)((lpa / (domain->DieNo * domain->ChannelNo)) % domain->PlaneNo)];
			break;
		case Flash_Plane_Allocation_Scheme_Type::DWCP:
			read_address.ChannelID = domain->ChannelIDs[(unsigned int)((lpa / (domain->DieNo * domain->ChipNo)) % domain->ChannelNo)];
			read_address.ChipID = domain->ChipIDs[(unsigned int)((lpa / domain->DieNo) % domain->ChipNo)];
			read_address.DieID = domain->DieIDs[(unsigned int)(lpa % domain->DieNo)];
			read_address.PlaneID = domain->DieIDs[(unsigned int)((lpa / (domain->DieNo * domain->ChipNo * domain->ChannelNo)) % domain->PlaneNo)];
			break;
		case Flash_Plane_Allocation_Scheme_Type::DWPC:
			read_address.ChannelID = domain->ChannelIDs[(unsigned int)((lpa / (domain->DieNo * domain->ChipNo * domain->PlaneNo)) % domain->ChannelNo)];
			read_address.ChipID = domain->ChipIDs[(unsigned int)((lpa / domain->DieNo) % domain->ChipNo)];
			read_address.DieID = domain->DieIDs[(unsigned int)(lpa % domain->DieNo)];
			read_address.PlaneID = domain->PlaneIDs[(unsigned int)((lpa / (domain->DieNo * domain->ChipNo)) % domain->PlaneNo)];
			break;
		case Flash_Plane_Allocation_Scheme_Type::DPCW:
			read_address.ChannelID = domain->ChannelIDs[(unsigned int)((lpa / (domain->DieNo * domain->PlaneNo)) % domain->ChannelNo)];
			read_address.ChipID = domain->ChipIDs[(unsigned int)((lpa / (domain->DieNo * domain->PlaneNo * domain->ChannelNo)) % domain->ChipNo)];
			read_address.DieID = domain->DieIDs[(unsigned int)(lpa % domain->DieNo)];
			read_address.PlaneID = domain->PlaneIDs[(unsigned int)((lpa / domain->DieNo) % domain->PlaneNo)];
			break;
		case Flash_Plane_Allocation_Scheme_Type::DPWC:
			read_address.ChannelID = domain->ChannelIDs[(unsigned int)((lpa / (domain->DieNo * domain->PlaneNo * domain->ChipNo)) % domain->ChannelNo)];
			read_address.ChipID = domain->ChipIDs[(unsigned int)((lpa / (domain->DieNo * domain->PlaneNo)) % domain->ChipNo)];
			read_address.DieID = domain->DieIDs[(unsigned int)(lpa % domain->DieNo)];
			read_address.PlaneID = domain->PlaneIDs[(unsigned int)((lpa / domain->DieNo) % domain->PlaneNo)];
			break;
			//Static: Plane first
		case Flash_Plane_Allocation_Scheme_Type::PCWD:
			read_address.ChannelID = domain->ChannelIDs[(unsigned int)((lpa / domain->PlaneNo) % domain->ChannelNo)];
			read_address.ChipID = domain->ChipIDs[(unsigned int)((lpa / (domain->PlaneNo * domain->ChannelNo)) % domain->ChipNo)];
			read_address.DieID = domain->DieIDs[(unsigned int)((lpa / (domain->PlaneNo * domain->ChannelNo * domain->ChipNo)) % domain->DieNo)];
			read_address.PlaneID = domain->PlaneIDs[(unsigned int)(lpa % domain->PlaneNo)];
			break;
		case Flash_Plane_Allocation_Scheme_Type::PCDW:
			read_address.ChannelID = domain->ChannelIDs[(unsigned int)((lpa / domain->PlaneNo) % domain->ChannelNo)];
			read_address.ChipID = domain->ChipIDs[(unsigned int)((lpa / (domain->PlaneNo * domain->ChannelNo * domain->DieNo)) % domain->ChipNo)];
			read_address.DieID = domain->DieIDs[(unsigned int)((lpa / (domain->PlaneNo * domain->ChannelNo)) % domain->DieNo)];
			read_address.PlaneID = domain->PlaneIDs[(unsigned int)(lpa % domain->PlaneNo)];
			break;
		case Flash_Plane_Allocation_Scheme_Type::PWCD:
			read_address.ChannelID = domain->ChannelIDs[(unsigned int)((lpa / (domain->PlaneNo * domain->ChipNo)) % domain->ChannelNo)];
			read_address.ChipID = domain->ChipIDs[(unsigned int)((lpa / domain->PlaneNo) % domain->ChipNo)];
			read_address.DieID = domain->DieIDs[(unsigned int)((lpa / (domain->PlaneNo * domain->ChipNo * domain->ChannelNo)) % domain->DieNo)];
			read_address.PlaneID = domain->PlaneIDs[(unsigned int)(lpa % domain->PlaneNo)];
			break;
		case Flash_Plane_Allocation_Scheme_Type::PWDC:
			read_address.ChannelID = domain->ChannelIDs[(unsigned int)((lpa / (domain->PlaneNo * domain->ChipNo * domain->DieNo)) % domain->ChannelNo)];
			read_address.ChipID = domain->ChipIDs[(unsigned int)((lpa / domain->PlaneNo) % domain->ChipNo)];
			read_address.DieID = domain->DieIDs[(unsigned int)((lpa / (domain->PlaneNo * domain->ChipNo)) % domain->DieNo)];
			read_address.PlaneID = domain->PlaneIDs[(unsigned int)(lpa % domain->PlaneNo)];
			break;
		case Flash_Plane_Allocation_Scheme_Type::PDCW:
			read_address.ChannelID = domain->ChannelIDs[(unsigned int)((lpa / (domain->PlaneNo * domain->DieNo)) % domain->ChannelNo)];
			read_address.ChipID = domain->ChipIDs[(unsigned int)((lpa / (domain->PlaneNo * domain->DieNo * domain->ChannelNo)) % domain->ChipNo)];
			read_address.DieID = domain->DieIDs[(unsigned int)((lpa / domain->PlaneNo) % domain->DieNo)];
			read_address.PlaneID = domain->PlaneIDs[(unsigned int)(lpa % domain->PlaneNo)];
			break;
		case Flash_Plane_Allocation_Scheme_Type::PDWC:
			read_address.ChannelID = domain->ChannelIDs[(unsigned int)((lpa / (domain->PlaneNo * domain->DieNo * domain->ChipNo)) % domain->ChannelNo)];
			read_address.ChipID = domain->ChipIDs[(unsigned int)((lpa / (domain->PlaneNo * domain->DieNo)) % domain->ChipNo)];
			read_address.DieID = domain->DieIDs[(unsigned int)((lpa / domain->PlaneNo) % domain->DieNo)];
			read_address.PlaneID = domain->PlaneIDs[(unsigned int)(lpa % domain->PlaneNo)];
			break;
		default:
			PRINT_ERROR("Unhandled Page Allocation Scheme Type")
		}

		BlockManager->Allocate_block_and_page_in_plane_for_user_write(stream_id, read_address);
		PPA_type ppa = convert_ppa_to_address(read_address);
		domain->CMT->Update_mapping_info(stream_id, lpa, ppa, read_sectors_bitmap);
		
		return ppa;
	}
	
	void Address_Mapping_Unit_Page_Level::prepare_mapping_table()
	{
		//Since address translation functions work on flash transactions
		NVM_Transaction_Flash_WR* dummy_tr = new NVM_Transaction_Flash_WR(Transaction_Source_Type::MAPPING, 0, 0,
			INVALID_LPN, 0, NULL, 0, NULL, 0, 0);

		for (unsigned int stream_id = 0; stream_id < input_stream_no; stream_id++)
		{
			dummy_tr->Stream_id = stream_id;
			for (unsigned int translation_page_id = 0; translation_page_id < domains[stream_id]->Total_translation_pages_no; translation_page_id++)
			{
				dummy_tr->LPA = (LPA_type) translation_page_id;
				allocate_plane_for_translation_write(dummy_tr);
				allocate_page_in_plane_for_translation_write(dummy_tr, (MVPN_type)dummy_tr->LPA);
			}
		}
	}

	inline bool Address_Mapping_Unit_Page_Level::Get_bitmap_vector_of_written_sectors_of_lpn(const stream_id_type streamID, const LPA_type lpn, page_status_type& pageState)
	{
		if (domains[streamID]->CMT->Exists(streamID, lpn))
		{
			pageState = domains[streamID]->CMT->Get_bitmap_vector_of_written_sectors(streamID, lpn);
			return true;
		}
		else return false;
	}


	inline MVPN_type Address_Mapping_Unit_Page_Level::get_MVPN(const LPA_type lpn, stream_id_type stream_id)
	{
		//return (MVPN_type)((lpn % (domains[stream_id]->Total_logical_pages_no)) / no_of_translation_entries_per_page);
		return (MVPN_type)(lpn / no_of_translation_entries_per_page);
	}

	inline LPA_type Address_Mapping_Unit_Page_Level::get_start_LPN_MVP(const MVPN_type mvpn)
	{
		return (MVPN_type)(mvpn * no_of_translation_entries_per_page);
	}

	inline LPA_type Address_Mapping_Unit_Page_Level::get_end_LPN_in_MVP(const MVPN_type mvpn)
	{
		return (MVPN_type)(mvpn * no_of_translation_entries_per_page + no_of_translation_entries_per_page - 1);
	}

	bool Address_Mapping_Unit_Page_Level::check_and_translate(NVM_Transaction_Flash* transaction)
	{
		stream_id_type streamID = transaction->Stream_id;
		domains[streamID]->STAT_total_CMT_queries++;

		if (domains[streamID]->CMT->Exists(streamID, transaction->LPA))
		{
			domains[streamID]->STAT_CMT_hits++;
			if (transaction->Type == TransactionType::READ)
			{
				domains[streamID]->STAT_total_readTR_CMT_queries++;
				domains[streamID]->STAT_readTR_CMT_hits++;
			}
			else//This is a write transaction
			{
				domains[streamID]->STAT_total_writeTR_CMT_queries++;
				domains[streamID]->STAT_writeTR_CMT_hits++;
			}
			translate_lpa_to_ppa(streamID, transaction);
			transaction->Physical_address_determined = true;
			return true;
		}
		else
		{
			if (request_mapping_entry_for_lpn(streamID, transaction->LPA))//Maybe we can catch mapping data from an on-the-fly write back request
			{
				domains[streamID]->STAT_CMT_miss++;
				if (transaction->Type == TransactionType::READ)
				{
					domains[streamID]->STAT_total_readTR_CMT_queries++;
					domains[streamID]->STAT_readTR_CMT_miss++;
				}
				else//This is a write transaction
				{
					domains[streamID]->STAT_total_writeTR_CMT_queries++;
					domains[streamID]->STAT_writeTR_CMT_miss++;
				}
				translate_lpa_to_ppa(streamID, transaction);
				transaction->Physical_address_determined = true;
				return true;
			}
			else
			{
				if (transaction->Type == TransactionType::READ)
				{
					domains[streamID]->STAT_total_readTR_CMT_queries++;
					domains[streamID]->STAT_readTR_CMT_miss++;
					domains[streamID]->Waiting_unmapped_read_transactions.insert(std::pair<LPA_type, NVM_Transaction_Flash*>(transaction->LPA, transaction));
				}
				else//This is a write transaction
				{
					domains[streamID]->STAT_total_writeTR_CMT_queries++;
					domains[streamID]->STAT_writeTR_CMT_miss++;
					domains[streamID]->Waiting_unmapped_program_transactions.insert(std::pair<LPA_type, NVM_Transaction_Flash*>(transaction->LPA, transaction));
				}
			}
			return false;
		}
	}
	
	/*This function should be invoked only if the address translation entry exists in CMT.
	* Otherwise, the call to the CMT->Rerieve_ppa, within this function, will throw an exception.
	*/
	inline void Address_Mapping_Unit_Page_Level::translate_lpa_to_ppa(stream_id_type streamID, NVM_Transaction_Flash* transaction)
	{
		PPA_type ppa = domains[streamID]->CMT->Rerieve_ppa(streamID, transaction->LPA);
		if (transaction->Type == TransactionType::READ)
		{
			if (ppa == NO_PPA)
				ppa = Online_create_entry_for_reads(transaction->LPA, streamID, transaction->Address, ((NVM_Transaction_Flash_RD*)transaction)->read_sectors_bitmap);
			transaction->PPA = ppa;
			convert_ppa_to_address(transaction->PPA, transaction->Address);
		}
		else//This is a write transaction
		{
			allocate_plane_for_user_write((NVM_Transaction_Flash_WR*)transaction);
			allocate_page_in_plane_for_user_write((NVM_Transaction_Flash_WR*)transaction);
		}
	}
	
	void Address_Mapping_Unit_Page_Level::allocate_plane_for_user_write(NVM_Transaction_Flash_WR* transaction)
	{
		LPA_type lpn = transaction->LPA;
		NVM::FlashMemory::Physical_Page_Address& targetAddress = transaction->Address;
		AddressMappingDomain* domain = domains[transaction->Stream_id];

		switch (domain->PlaneAllocationScheme)
		{
		case Flash_Plane_Allocation_Scheme_Type::CWDP:
			targetAddress.ChannelID = domain->ChannelIDs[(unsigned int)(lpn % domain->ChannelNo)];
			targetAddress.ChipID = domain->ChipIDs[(unsigned int)((lpn / domain->ChannelNo) % domain->ChipNo)];
			targetAddress.DieID = domain->DieIDs[(unsigned int)((lpn / (domain->ChipNo * domain->ChannelNo)) % domain->DieNo)];
			targetAddress.PlaneID = domain->PlaneIDs[(unsigned int)((lpn / (domain->DieNo * domain->ChipNo * domain->ChannelNo)) % domain->PlaneNo)];
			break;
		case Flash_Plane_Allocation_Scheme_Type::CWPD:
			targetAddress.ChannelID = domain->ChannelIDs[(unsigned int)(lpn % domain->ChannelNo)];
			targetAddress.ChipID = domain->ChipIDs[(unsigned int)((lpn / domain->ChannelNo) % domain->ChipNo)];
			targetAddress.DieID = domain->DieIDs[(unsigned int)((lpn / (domain->ChannelNo * domain->ChipNo * domain->PlaneNo)) % domain->DieNo)];
			targetAddress.PlaneID = domain->PlaneIDs[(unsigned int)((lpn / (domain->ChannelNo * domain->ChipNo)) % domain->PlaneNo)];
			break;
		case Flash_Plane_Allocation_Scheme_Type::CDWP:
			targetAddress.ChannelID = domain->ChannelIDs[(unsigned int)(lpn % domain->ChannelNo)];
			targetAddress.ChipID = domain->ChipIDs[(unsigned int)((lpn / (domain->DieNo * domain->ChannelNo)) % domain->ChipNo)];
			targetAddress.DieID = domain->DieIDs[(unsigned int)((lpn / domain->ChannelNo) % domain->DieNo)];
			targetAddress.PlaneID = domain->PlaneIDs[(unsigned int)((lpn / (domain->DieNo * domain->ChipNo * domain->ChannelNo)) % domain->PlaneNo)];
			break;
		case Flash_Plane_Allocation_Scheme_Type::CDPW:
			targetAddress.ChannelID = domain->ChannelIDs[(unsigned int)(lpn % domain->ChannelNo)];
			targetAddress.ChipID = domain->ChipIDs[(unsigned int)((lpn / (domain->PlaneNo * domain->DieNo * domain->ChannelNo)) % domain->ChipNo)];
			targetAddress.DieID = domain->DieIDs[(unsigned int)((lpn / domain->ChannelNo) % domain->DieNo)];
			targetAddress.PlaneID = domain->PlaneIDs[(unsigned int)((lpn / (domain->DieNo * domain->ChannelNo)) % domain->PlaneNo)];
			break;
		case Flash_Plane_Allocation_Scheme_Type::CPWD:
			targetAddress.ChannelID = domain->ChannelIDs[(unsigned int)(lpn % domain->ChannelNo)];
			targetAddress.ChipID = domain->ChipIDs[(unsigned int)((lpn / (domain->PlaneNo * domain->ChannelNo)) % domain->ChipNo)];
			targetAddress.DieID = domain->DieIDs[(unsigned int)((lpn / (domain->PlaneNo * domain->ChipNo * domain->ChannelNo)) % domain->DieNo)];
			targetAddress.PlaneID = domain->PlaneIDs[(unsigned int)((lpn / domain->ChannelNo) % domain->PlaneNo)];
			break;
		case Flash_Plane_Allocation_Scheme_Type::CPDW:
			targetAddress.ChannelID = domain->ChannelIDs[(unsigned int)(lpn % domain->ChannelNo)];
			targetAddress.ChipID = domain->ChipIDs[(unsigned int)((lpn / (domain->PlaneNo * domain->DieNo * domain->ChannelNo)) % domain->ChipNo)];
			targetAddress.DieID = domain->DieIDs[(unsigned int)((lpn / (domain->PlaneNo * domain->ChannelNo)) % domain->DieNo)];
			targetAddress.PlaneID = domain->PlaneIDs[(unsigned int)((lpn / domain->ChannelNo) % domain->PlaneNo)];
			break;
			//Static: Way first
		case Flash_Plane_Allocation_Scheme_Type::WCDP:
			targetAddress.ChannelID = domain->ChannelIDs[(unsigned int)((lpn / domain->ChipNo) % domain->ChannelNo)];
			targetAddress.ChipID = domain->ChipIDs[(unsigned int)(lpn % domain->ChipNo)];
			targetAddress.DieID = domain->DieIDs[(unsigned int)((lpn / (domain->ChipNo * domain->ChannelNo)) % domain->DieNo)];
			targetAddress.PlaneID = domain->PlaneIDs[(unsigned int)((lpn / (domain->ChipNo * domain->ChannelNo * domain->DieNo)) % domain->PlaneNo)];
			break;
		case Flash_Plane_Allocation_Scheme_Type::WCPD:
			targetAddress.ChannelID = domain->ChannelIDs[(unsigned int)((lpn / domain->ChipNo) % domain->ChannelNo)];
			targetAddress.ChipID = domain->ChipIDs[(unsigned int)(lpn % domain->ChipNo)];
			targetAddress.DieID = domain->DieIDs[(unsigned int)((lpn / (domain->ChipNo * domain->ChannelNo * domain->PlaneNo)) % domain->DieNo)];
			targetAddress.PlaneID = domain->PlaneIDs[(unsigned int)((lpn / (domain->ChipNo * domain->ChannelNo)) % domain->PlaneNo)];
			break;
		case Flash_Plane_Allocation_Scheme_Type::WDCP:
			targetAddress.ChannelID = domain->ChannelIDs[(unsigned int)((lpn / (domain->ChipNo * domain->DieNo)) % domain->ChannelNo)];
			targetAddress.ChipID = domain->ChipIDs[(unsigned int)(lpn % domain->ChipNo)];
			targetAddress.DieID = domain->DieIDs[(unsigned int)((lpn / domain->ChipNo) % domain->DieNo)];
			targetAddress.PlaneID = domain->PlaneIDs[(unsigned int)((lpn / (domain->ChipNo * domain->DieNo * domain->ChannelNo)) % domain->PlaneNo)];
			break;
		case Flash_Plane_Allocation_Scheme_Type::WDPC:
			targetAddress.ChannelID = domain->ChannelIDs[(unsigned int)((lpn / (domain->ChipNo * domain->DieNo * domain->PlaneNo)) % domain->ChannelNo)];
			targetAddress.ChipID = domain->ChipIDs[(unsigned int)(lpn % domain->ChipNo)];
			targetAddress.DieID = domain->DieIDs[(unsigned int)((lpn / domain->ChipNo) % domain->DieNo)];
			targetAddress.PlaneID = domain->PlaneIDs[(unsigned int)((lpn / (domain->ChipNo * domain->DieNo)) % domain->PlaneNo)];
			break;
		case Flash_Plane_Allocation_Scheme_Type::WPCD:
			targetAddress.ChannelID = domain->ChannelIDs[(unsigned int)((lpn / (domain->ChipNo * domain->PlaneNo)) % domain->ChannelNo)];
			targetAddress.ChipID = domain->ChipIDs[(unsigned int)(lpn % domain->ChipNo)];
			targetAddress.DieID = domain->DieIDs[(unsigned int)((lpn / (domain->ChipNo * domain->PlaneNo * domain->ChannelNo)) % domain->DieNo)];
			targetAddress.PlaneID = domain->PlaneIDs[(unsigned int)((lpn / domain->ChipNo) % domain->PlaneNo)];
			break;
		case Flash_Plane_Allocation_Scheme_Type::WPDC:
			targetAddress.ChannelID = domain->ChannelIDs[(unsigned int)((lpn / (domain->ChipNo * domain->PlaneNo * domain->DieNo)) % domain->ChannelNo)];
			targetAddress.ChipID = domain->ChipIDs[(unsigned int)(lpn % domain->ChipNo)];
			targetAddress.DieID = domain->DieIDs[(unsigned int)((lpn / (domain->ChipNo * domain->PlaneNo)) % domain->DieNo)];
			targetAddress.PlaneID = domain->PlaneIDs[(unsigned int)((lpn / domain->ChipNo) % domain->PlaneNo)];
			break;
			//Static: Die first
		case Flash_Plane_Allocation_Scheme_Type::DCWP:
			targetAddress.ChannelID = domain->ChannelIDs[(unsigned int)((lpn / domain->DieNo) % domain->ChannelNo)];
			targetAddress.ChipID = domain->ChipIDs[(unsigned int)((lpn / (domain->DieNo * domain->ChannelNo)) % domain->ChipNo)];
			targetAddress.DieID = domain->DieIDs[(unsigned int)(lpn % domain->DieNo)];
			targetAddress.PlaneID = domain->PlaneIDs[(unsigned int)((lpn / (domain->DieNo * domain->ChannelNo * domain->ChipNo)) % domain->PlaneNo)];
			break;
		case Flash_Plane_Allocation_Scheme_Type::DCPW:
			targetAddress.ChannelID = domain->ChannelIDs[(unsigned int)((lpn / domain->DieNo) % domain->ChannelNo)];
			targetAddress.ChipID = domain->ChipIDs[(unsigned int)((lpn / (domain->DieNo * domain->ChannelNo * domain->PlaneNo)) % domain->ChipNo)];
			targetAddress.DieID = domain->DieIDs[(unsigned int)(lpn % domain->DieNo)];
			targetAddress.PlaneID = domain->PlaneIDs[(unsigned int)((lpn / (domain->DieNo * domain->ChannelNo)) % domain->PlaneNo)];
			break;
		case Flash_Plane_Allocation_Scheme_Type::DWCP:
			targetAddress.ChannelID = domain->ChannelIDs[(unsigned int)((lpn / (domain->DieNo * domain->ChipNo)) % domain->ChannelNo)];
			targetAddress.ChipID = domain->ChipIDs[(unsigned int)((lpn / domain->DieNo) % domain->ChipNo)];
			targetAddress.DieID = domain->DieIDs[(unsigned int)(lpn % domain->DieNo)];
			targetAddress.PlaneID = domain->PlaneIDs[(unsigned int)((lpn / (domain->DieNo * domain->ChipNo * domain->ChannelNo)) % domain->PlaneNo)];
			break;
		case Flash_Plane_Allocation_Scheme_Type::DWPC:
			targetAddress.ChannelID = domain->ChannelIDs[(unsigned int)((lpn / (domain->DieNo * domain->ChipNo * domain->PlaneNo)) % domain->ChannelNo)];
			targetAddress.ChipID = domain->ChipIDs[(unsigned int)((lpn / domain->DieNo) % domain->ChipNo)];
			targetAddress.DieID = domain->DieIDs[(unsigned int)(lpn % domain->DieNo)];
			targetAddress.PlaneID = domain->PlaneIDs[(unsigned int)((lpn / (domain->DieNo * domain->ChipNo)) % domain->PlaneNo)];
			break;
		case Flash_Plane_Allocation_Scheme_Type::DPCW:
			targetAddress.ChannelID = domain->ChannelIDs[(unsigned int)((lpn / (domain->DieNo * domain->PlaneNo)) % domain->ChannelNo)];
			targetAddress.ChipID = domain->ChipIDs[(unsigned int)((lpn / (domain->DieNo * domain->PlaneNo * domain->ChannelNo)) % domain->ChipNo)];
			targetAddress.DieID = domain->DieIDs[(unsigned int)(lpn % domain->DieNo)];
			targetAddress.PlaneID = domain->PlaneIDs[(unsigned int)((lpn / domain->DieNo) % domain->PlaneNo)];
			break;
		case Flash_Plane_Allocation_Scheme_Type::DPWC:
			targetAddress.ChannelID = domain->ChannelIDs[(unsigned int)((lpn / (domain->DieNo * domain->PlaneNo * domain->ChipNo)) % domain->ChannelNo)];
			targetAddress.ChipID = domain->ChipIDs[(unsigned int)((lpn / (domain->DieNo * domain->PlaneNo)) % domain->ChipNo)];
			targetAddress.DieID = domain->DieIDs[(unsigned int)(lpn % domain->DieNo)];
			targetAddress.PlaneID = domain->PlaneIDs[(unsigned int)((lpn / domain->DieNo) % domain->PlaneNo)];
			break;
			//Static: Plane first
		case Flash_Plane_Allocation_Scheme_Type::PCWD:
			targetAddress.ChannelID = domain->ChannelIDs[(unsigned int)((lpn / domain->PlaneNo) % domain->ChannelNo)];
			targetAddress.ChipID = domain->ChipIDs[(unsigned int)((lpn / (domain->PlaneNo * domain->ChannelNo)) % domain->ChipNo)];
			targetAddress.DieID = domain->DieIDs[(unsigned int)((lpn / (domain->PlaneNo * domain->ChannelNo * domain->ChipNo)) % domain->DieNo)];
			targetAddress.PlaneID = domain->PlaneIDs[(unsigned int)(lpn % domain->PlaneNo)];
			break;
		case Flash_Plane_Allocation_Scheme_Type::PCDW:
			targetAddress.ChannelID = domain->ChannelIDs[(unsigned int)((lpn / domain->PlaneNo) % domain->ChannelNo)];
			targetAddress.ChipID = domain->ChipIDs[(unsigned int)((lpn / (domain->PlaneNo * domain->ChannelNo * domain->DieNo)) % domain->ChipNo)];
			targetAddress.DieID = domain->DieIDs[(unsigned int)((lpn / (domain->PlaneNo * domain->ChannelNo)) % domain->DieNo)];
			targetAddress.PlaneID = domain->PlaneIDs[(unsigned int)(lpn % domain->PlaneNo)];
			break;
		case Flash_Plane_Allocation_Scheme_Type::PWCD:
			targetAddress.ChannelID = domain->ChannelIDs[(unsigned int)((lpn / (domain->PlaneNo * domain->ChipNo)) % domain->ChannelNo)];
			targetAddress.ChipID = domain->ChipIDs[(unsigned int)((lpn / domain->PlaneNo) % domain->ChipNo)];
			targetAddress.DieID = domain->DieIDs[(unsigned int)((lpn / (domain->PlaneNo * domain->ChipNo * domain->ChannelNo)) % domain->DieNo)];
			targetAddress.PlaneID = domain->PlaneIDs[(unsigned int)(lpn % domain->PlaneNo)];
			break;
		case Flash_Plane_Allocation_Scheme_Type::PWDC:
			targetAddress.ChannelID = domain->ChannelIDs[(unsigned int)((lpn / (domain->PlaneNo * domain->ChipNo * domain->DieNo)) % domain->ChannelNo)];
			targetAddress.ChipID = domain->ChipIDs[(unsigned int)((lpn / domain->PlaneNo) % domain->ChipNo)];
			targetAddress.DieID = domain->DieIDs[(unsigned int)((lpn / (domain->PlaneNo * domain->ChipNo)) % domain->DieNo)];
			targetAddress.PlaneID = domain->PlaneIDs[(unsigned int)(lpn % domain->PlaneNo)];
			break;
		case Flash_Plane_Allocation_Scheme_Type::PDCW:
			targetAddress.ChannelID = domain->ChannelIDs[(unsigned int)((lpn / (domain->PlaneNo * domain->DieNo)) % domain->ChannelNo)];
			targetAddress.ChipID = domain->ChipIDs[(unsigned int)((lpn / (domain->PlaneNo * domain->DieNo * domain->ChannelNo)) % domain->ChipNo)];
			targetAddress.DieID = domain->DieIDs[(unsigned int)((lpn / domain->PlaneNo) % domain->DieNo)];
			targetAddress.PlaneID = domain->PlaneIDs[(unsigned int)(lpn % domain->PlaneNo)];
			break;
		case Flash_Plane_Allocation_Scheme_Type::PDWC:
			targetAddress.ChannelID = domain->ChannelIDs[(unsigned int)((lpn / (domain->PlaneNo * domain->DieNo * domain->ChipNo)) % domain->ChannelNo)];
			targetAddress.ChipID = domain->ChipIDs[(unsigned int)((lpn / (domain->PlaneNo * domain->DieNo)) % domain->ChipNo)];
			targetAddress.DieID = domain->DieIDs[(unsigned int)((lpn / domain->PlaneNo) % domain->DieNo)];
			targetAddress.PlaneID = domain->PlaneIDs[(unsigned int)(lpn % domain->PlaneNo)];
			break;
		default:
			PRINT_ERROR("Unhandled allocation scheme type!")
		}
	}
	
	void Address_Mapping_Unit_Page_Level::allocate_page_in_plane_for_user_write(NVM_Transaction_Flash_WR* transaction)
	{
		AddressMappingDomain* domain = domains[transaction->Stream_id];
		BlockManager->Allocate_block_and_page_in_plane_for_user_write(transaction->Stream_id, transaction->Address);
		transaction->PPA = convert_ppa_to_address(transaction->Address);

		PPA_type old_ppn = domain->CMT->Rerieve_ppa(transaction->Stream_id, transaction->LPA);
		if (old_ppn == NO_PPA)  /*this is the first access to the logical page*/
			domain->CMT->Update_mapping_info(transaction->Stream_id, transaction->LPA, 
				transaction->PPA, ((NVM_Transaction_Flash_WR*)transaction)->write_sectors_bitmap);
		else
		{
			page_status_type prev_page_status = domain->CMT->Get_bitmap_vector_of_written_sectors(transaction->Stream_id, transaction->LPA);
			if ((transaction->write_sectors_bitmap & prev_page_status) == prev_page_status)
			{
				NVM::FlashMemory::Physical_Page_Address addr;
				convert_ppa_to_address(old_ppn, addr);
				BlockManager->Invalidate_page_in_block(transaction->Stream_id, addr);
			}
			else
			{
				NVM_Transaction_Flash_RD *update_read_tr = new NVM_Transaction_Flash_RD(transaction->Source, transaction->Stream_id, 
					sector_count(prev_page_status) * SECTOR_SIZE_IN_BYTE, transaction->LPA, old_ppn, transaction->UserIORequest,
					transaction->Content, transaction, prev_page_status, domain->GlobalMappingTable[transaction->LPA].TimeStamp);
				convert_ppa_to_address(old_ppn, update_read_tr->Address);
				BlockManager->Invalidate_page_in_block(transaction->Stream_id, update_read_tr->Address);
				transaction->RelatedRead = update_read_tr;
			}
			domain->CMT->Update_mapping_info(transaction->Stream_id, transaction->LPA, transaction->PPA,
				((NVM_Transaction_Flash_WR*)transaction)->write_sectors_bitmap | domain->CMT->Get_bitmap_vector_of_written_sectors(transaction->Stream_id, transaction->LPA));
		}
	}

	void Address_Mapping_Unit_Page_Level::allocate_plane_for_translation_write(NVM_Transaction_Flash* transaction)
	{
		allocate_plane_for_user_write((NVM_Transaction_Flash_WR*)transaction);
	}

	void Address_Mapping_Unit_Page_Level::allocate_page_in_plane_for_translation_write(NVM_Transaction_Flash* transaction, MVPN_type mvpn)
	{
		AddressMappingDomain* domain = domains[transaction->Stream_id];
		BlockManager->Allocate_block_and_page_in_plane_for_translation_write(transaction->Stream_id, transaction->Address);
		transaction->PPA = convert_ppa_to_address(transaction->Address);

		MPPN_type old_MPPN = domain->GlobalTranslationDirectory[mvpn].MPPN;
		if (old_MPPN != NO_MPPN)  /*this is the first access to the mvpn*/
		{
			NVM::FlashMemory::Physical_Page_Address prevAddr;
			convert_ppa_to_address(old_MPPN, prevAddr);
			BlockManager->Invalidate_page_in_block(transaction->Stream_id, prevAddr);
		}
		domain->GlobalTranslationDirectory[mvpn].MPPN = (MPPN_type) transaction->PPA;
		domain->GlobalTranslationDirectory[mvpn].TimeStamp = CurrentTimeStamp;
	}

	bool Address_Mapping_Unit_Page_Level::request_mapping_entry_for_lpn(const stream_id_type stream_id, const LPA_type lpn)
	{
		AddressMappingDomain* domain = domains[stream_id];
		MVPN_type mvpn = get_MVPN(lpn, stream_id);

		/*This is the first time that a user request accesses this address.
		Just create an entry in cache! No flash read is needed.*/
		if (domain->GlobalTranslationDirectory[mvpn].MPPN == NO_MPPN)
		{
			if (!domain->CMT->Check_free_slot_availability())
			{
				CMTSlotType evictedItem = domain->CMT->Evict_one_slot();
				if (evictedItem.Dirty)
				{
					/* In order to eliminate possible race conditions for the requests that
					* will access the evicted lpa in the near future (before the translation
					* write finishes), MQSim updates GMT (the on flash mapping table) right
					* after eviction happens.*/
					domain->GlobalMappingTable[lpn].PPA = evictedItem.PPA;
					domain->GlobalMappingTable[lpn].WrittenStateBitmap = evictedItem.WrittenStateBitmap;
					if (domain->GlobalMappingTable[lpn].TimeStamp > CurrentTimeStamp)
						throw "Unexpected situation occured in handling GMT!";
					domain->GlobalMappingTable[lpn].TimeStamp = CurrentTimeStamp;
					generate_flash_writeback_request_for_mapping_data(stream_id, lpn);
				}
			}
			domain->CMT->Reserve_slot_for_lpn(stream_id, lpn);
			domain->CMT->Insert_new_mapping_info(stream_id, lpn, NO_PPA, UNWRITTEN_LOGICAL_PAGE);
			return true;
		}

		/*A read transaction is already under process to retrieve the MVP content.
		* This situation may happen in two different cases:
		* 1. A read has been issued to retrieve unchanged parts of the mapping data and merge them
		*     with the changed parts (i.e., an update read of MVP). This read will be followed
		*     by a writeback of MVP content to a new flash page.
		* 2. A read has been issued to retrieve the mapping data for some previous user requests*/
		if (domain->ArrivingMappingEntries.find(mvpn) != domain->ArrivingMappingEntries.end())
		{
			if (domain->CMT->Is_slot_reserved_for_lpn_and_waiting(stream_id, lpn))
				return false;
			else //An entry should be created in the cache
			{
				if (!domain->CMT->Check_free_slot_availability())
				{
					CMTSlotType evictedItem = domain->CMT->Evict_one_slot();
					if (evictedItem.Dirty)
					{
						/* In order to eliminate possible race conditions for the requests that
						* will access the evicted lpa in the near future (before the translation
						* write finishes), MQSim updates GMT (the on flash mapping table) right
						* after eviction happens.*/
						domain->GlobalMappingTable[lpn].PPA = evictedItem.PPA;
						domain->GlobalMappingTable[lpn].WrittenStateBitmap = evictedItem.WrittenStateBitmap;
						if (domain->GlobalMappingTable[lpn].TimeStamp > CurrentTimeStamp)
							throw "Unexpected situation occured in handling GMT!";
						domain->GlobalMappingTable[lpn].TimeStamp = CurrentTimeStamp;
						generate_flash_writeback_request_for_mapping_data(stream_id, lpn);
					}
				}
				domain->CMT->Reserve_slot_for_lpn(stream_id, lpn);
				domain->ArrivingMappingEntries.insert(std::pair<MVPN_type, LPA_type>(mvpn, lpn));
				return false;
			}
		}

		/*MQSim assumes that the data of all departing (evicted from CMT) translation pages are in memory, until
		the flash program operation finishes and the entry it is cleared from DepartingMappingEntries.*/
		if (domain->DepartingMappingEntries.find(mvpn) != domain->DepartingMappingEntries.end())
		{
			if (!domain->CMT->Check_free_slot_availability())
			{
				CMTSlotType evictedItem = domain->CMT->Evict_one_slot();
				if (evictedItem.Dirty)
				{
					/* In order to eliminate possible race conditions for the requests that
					* will access the evicted lpa in the near future (before the translation
					* write finishes), MQSim updates GMT (the on flash mapping table) right
					* after eviction happens.*/
					domain->GlobalMappingTable[lpn].PPA = evictedItem.PPA;
					domain->GlobalMappingTable[lpn].WrittenStateBitmap = evictedItem.WrittenStateBitmap;
					if (domain->GlobalMappingTable[lpn].TimeStamp > CurrentTimeStamp)
						throw "Unexpected situation occured in handling GMT!";
					domain->GlobalMappingTable[lpn].TimeStamp = CurrentTimeStamp;
					generate_flash_writeback_request_for_mapping_data(stream_id, lpn);
				}
			}
			domain->CMT->Reserve_slot_for_lpn(stream_id, lpn);
			/*Hack: since we do not actually save the values of translation requests, we copy the mapping
			data from GlobalMappingTable (which actually must be stored on flash)*/
			domain->CMT->Insert_new_mapping_info(stream_id, lpn,
				domain->GlobalMappingTable[lpn].PPA, domain->GlobalMappingTable[lpn].WrittenStateBitmap);
			return true;
		}	

		//Non of the above options provide mapping data. So, MQSim, must read the translation data from flash memory
		if (!domain->CMT->Check_free_slot_availability())
		{
			CMTSlotType evictedItem = domain->CMT->Evict_one_slot();
			if (evictedItem.Dirty)
			{
				/* In order to eliminate possible race conditions for the requests that
				* will access the evicted lpa in the near future (before the translation
				* write finishes), MQSim updates GMT (the on flash mapping table) right
				* after eviction happens.*/
				domain->GlobalMappingTable[lpn].PPA = evictedItem.PPA;
				domain->GlobalMappingTable[lpn].WrittenStateBitmap = evictedItem.WrittenStateBitmap;
				if (domain->GlobalMappingTable[lpn].TimeStamp > CurrentTimeStamp)
					throw "Unexpected situation occured in handling GMT!";
				domain->GlobalMappingTable[lpn].TimeStamp = CurrentTimeStamp;
				generate_flash_writeback_request_for_mapping_data(stream_id, lpn);
			}
		}
		domain->CMT->Reserve_slot_for_lpn(stream_id, lpn);
		generate_flash_read_request_for_mapping_data(stream_id, lpn);//consult GTD and create read transaction
		return false;
	}

	void Address_Mapping_Unit_Page_Level::generate_flash_writeback_request_for_mapping_data(const stream_id_type stream_id, const LPA_type lpn)
	{
		ftl->TSU->Prepare_for_transaction_submit();

		//Writing back all dirty CMT entries that fall into the same translation virtual page (MVPN)
		unsigned int read_size = 0;
		page_status_type readSectorsBitmap = 0;
		MVPN_type mvpn = get_MVPN(lpn, stream_id);
		LPA_type startLPN = get_start_LPN_MVP(mvpn);
		LPA_type endLPN = get_end_LPN_in_MVP(mvpn);
		for (LPA_type lpn = startLPN; lpn <= endLPN; lpn++)
			if (domains[stream_id]->CMT->Exists(stream_id, lpn))
			{
				if (domains[stream_id]->CMT->Is_dirty(stream_id, lpn))
					domains[stream_id]->CMT->Make_clean(stream_id, lpn);
				else
				{
					page_status_type bitlocation = (((page_status_type)0x1) << (((lpn - startLPN) * GTD_entry_size) / SECTOR_SIZE_IN_BYTE));
					if ((readSectorsBitmap & bitlocation) == 0)
					{
						readSectorsBitmap |= bitlocation;
						read_size += SECTOR_SIZE_IN_BYTE;
					}
				}
			}
		
		//Read the unchaged mapping entries from flash to merge them with updated parts of MVPN
		NVM_Transaction_Flash_RD* readTR = NULL;
		MPPN_type mppn = domains[stream_id]->GlobalTranslationDirectory[mvpn].MPPN;
		if (mppn != NO_PPA)
		{
			readTR = new NVM_Transaction_Flash_RD(Transaction_Source_Type::MAPPING, stream_id, read_size,
				INVALID_LPN, mppn, NULL, mvpn, NULL, readSectorsBitmap, CurrentTimeStamp);
			convert_ppa_to_address(mppn, readTR->Address);
			domains[stream_id]->ArrivingMappingEntries.insert(std::pair<MVPN_type, LPA_type>(mvpn, lpn));
			ftl->TSU->Submit_transaction(readTR);
		}

		NVM_Transaction_Flash_WR* writeTR = new NVM_Transaction_Flash_WR(Transaction_Source_Type::MAPPING, stream_id, SECTOR_SIZE_IN_BYTE * sector_no_per_page,
			INVALID_LPN, mppn, NULL, mvpn, readTR, (((page_status_type)0x1) << sector_no_per_page) - 1, CurrentTimeStamp);
		allocate_plane_for_translation_write(writeTR);
		allocate_page_in_plane_for_translation_write(writeTR, mvpn);		
		domains[stream_id]->DepartingMappingEntries.insert(get_MVPN(lpn, stream_id));
		ftl->TSU->Submit_transaction(writeTR);


		Stats::TotalMappingReadRequests++;
		Stats::TotalMappingWriteRequests++;
		Stats::MappingReadRequests[stream_id]++;
		Stats::MappingWriteRequests[stream_id]++;

		ftl->TSU->Schedule();
	}

	void Address_Mapping_Unit_Page_Level::generate_flash_read_request_for_mapping_data(const stream_id_type stream_id, const LPA_type lpn)
	{
		ftl->TSU->Prepare_for_transaction_submit();

		MVPN_type mvpn = get_MVPN(lpn, stream_id);
		PPA_type ppn = domains[stream_id]->GlobalTranslationDirectory[mvpn].MPPN;
		
		if(ppn == NO_PPA)
			PRINT_ERROR("Reading an unaviable physical flash page in function generate_flash_read_request_for_mapping_data")

		NVM_Transaction_Flash_RD* readTR = new NVM_Transaction_Flash_RD(Transaction_Source_Type::MAPPING, stream_id,
			SECTOR_SIZE_IN_BYTE, INVALID_LPN, NULL, mvpn, ((page_status_type)0x1) << sector_no_per_page, CurrentTimeStamp);
		convert_ppa_to_address(ppn, readTR->Address); 
		readTR->PPA = ppn;
		domains[stream_id]->ArrivingMappingEntries.insert(std::pair<MVPN_type, LPA_type>(mvpn, lpn));
		ftl->TSU->Submit_transaction(readTR);


		Stats::TotalMappingReadRequests++;
		Stats::MappingReadRequests[stream_id]++;

		ftl->TSU->Schedule();
	}
	
	inline void Address_Mapping_Unit_Page_Level::handle_transaction_serviced_signal_from_PHY(NVM_Transaction_Flash* transaction)
	{
		//First check if the transaction source is Mapping Module
		if (transaction->Source != Transaction_Source_Type::MAPPING)
			return;
		if (transaction->Type == TransactionType::WRITE)
		{
			_myInstance->domains[transaction->Stream_id]->DepartingMappingEntries.erase((MVPN_type)((NVM_Transaction_Flash_WR*)transaction)->Content);
		}
		else
		{
			/*If this is a read for an MVP that is required for merging unchanged mapping enries
			* (that are stored on flash) with those updated entries that are evicted from CMT*/
			if (((NVM_Transaction_Flash_RD*)transaction)->RelatedWrite != NULL)
				((NVM_Transaction_Flash_RD*)transaction)->RelatedWrite->RelatedRead = NULL;

			_myInstance->ftl->TSU->Prepare_for_transaction_submit();
			MVPN_type mvpn = (MVPN_type)((NVM_Transaction_Flash_RD*)transaction)->Content;
			std::multimap<MVPN_type, LPA_type>::iterator it = _myInstance->domains[transaction->Stream_id]->ArrivingMappingEntries.find(mvpn);
			while (it != _myInstance->domains[transaction->Stream_id]->ArrivingMappingEntries.end())
			{
				if ((*it).first == mvpn)
				{
					LPA_type lpa = (*it).second;
					_myInstance->domains[transaction->Stream_id]->CMT->Insert_new_mapping_info(transaction->Stream_id, lpa,
						_myInstance->domains[transaction->Stream_id]->GlobalMappingTable[lpa].PPA,
						_myInstance->domains[transaction->Stream_id]->GlobalMappingTable[lpa].WrittenStateBitmap);
					std::map<LPA_type, NVM_Transaction_Flash*>::iterator it2 = _myInstance->domains[transaction->Stream_id]->Waiting_unmapped_read_transactions.find(lpa);
					while (it2 != _myInstance->domains[transaction->Stream_id]->Waiting_unmapped_read_transactions.end() &&
						(*it2).first == lpa)
					{
						_myInstance->translate_lpa_to_ppa(transaction->Stream_id, it2->second);
						_myInstance->ftl->TSU->Submit_transaction(it2->second);
						_myInstance->domains[transaction->Stream_id]->Waiting_unmapped_read_transactions.erase(it2++);
					}
					it2 = _myInstance->domains[transaction->Stream_id]->Waiting_unmapped_program_transactions.find(lpa);
					while (it2 != _myInstance->domains[transaction->Stream_id]->Waiting_unmapped_program_transactions.end() &&
						(*it2).first == lpa)
					{
						_myInstance->translate_lpa_to_ppa(transaction->Stream_id, it2->second);
						_myInstance->ftl->TSU->Submit_transaction(it2->second);
						_myInstance->domains[transaction->Stream_id]->Waiting_unmapped_program_transactions.erase(it2++);
					}
				}
				else break;
				_myInstance->domains[transaction->Stream_id]->ArrivingMappingEntries.erase(it++);
			}
			_myInstance->ftl->TSU->Schedule();
		}
	}



	inline NVM::FlashMemory::Physical_Page_Address Address_Mapping_Unit_Page_Level::convert_ppa_to_address(const PPA_type ppn)
	{
		NVM::FlashMemory::Physical_Page_Address target;
		target.ChannelID = (flash_channel_ID_type)(ppn / page_no_per_channel);
		target.ChipID = (flash_chip_ID_type)((ppn % page_no_per_channel) / page_no_per_chip);
		target.DieID = (flash_die_ID_type)(((ppn % page_no_per_channel) % page_no_per_chip) / page_no_per_die);
		target.PlaneID = (flash_plane_ID_type)((((ppn % page_no_per_channel) % page_no_per_chip) % page_no_per_die) / page_no_per_plane);
		target.BlockID = (flash_block_ID_type)(((((ppn % page_no_per_channel) % page_no_per_chip) % page_no_per_die) % page_no_per_plane) / pages_no_per_block);
		target.PageID = (flash_page_ID_type)((((((ppn % page_no_per_channel) % page_no_per_chip) % page_no_per_die) % page_no_per_plane) % pages_no_per_block) % pages_no_per_block);

		return target;
	}

	inline void Address_Mapping_Unit_Page_Level::convert_ppa_to_address(const PPA_type ppn, NVM::FlashMemory::Physical_Page_Address& address)
	{
		address.ChannelID = (flash_channel_ID_type)(ppn / page_no_per_channel);
		address.ChipID = (flash_chip_ID_type)((ppn % page_no_per_channel) / page_no_per_chip);
		address.DieID = (flash_die_ID_type)(((ppn % page_no_per_channel) % page_no_per_chip) / page_no_per_die);
		address.PlaneID = (flash_plane_ID_type)((((ppn % page_no_per_channel) % page_no_per_chip) % page_no_per_die) / page_no_per_plane);
		address.BlockID = (flash_block_ID_type)(((((ppn % page_no_per_channel) % page_no_per_chip) % page_no_per_die) % page_no_per_plane) / pages_no_per_block);
		address.PageID = (flash_page_ID_type)((((((ppn % page_no_per_channel) % page_no_per_chip) % page_no_per_die) % page_no_per_plane) % pages_no_per_block) % pages_no_per_block);
	}

	inline PPA_type Address_Mapping_Unit_Page_Level::convert_ppa_to_address(const NVM::FlashMemory::Physical_Page_Address& pageAddress)
	{
		return (PPA_type)this->page_no_per_chip * (PPA_type)(pageAddress.ChannelID * this->chip_no_per_channel + pageAddress.ChipID)
			+ this->page_no_per_die * pageAddress.DieID + this->page_no_per_plane * pageAddress.PlaneID
			+ this->pages_no_per_block * pageAddress.BlockID + pageAddress.PageID;
	}

}
