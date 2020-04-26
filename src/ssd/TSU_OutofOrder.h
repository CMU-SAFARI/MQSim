#ifndef TSU_OUTOFORDER_H
#define TSU_OUTOFORDER_H

#include <list>
#include "TSU_Base.h"
#include "NVM_Transaction_Flash.h"
#include "NVM_PHY_ONFI_NVDDR2.h"
#include "FTL.h"

namespace SSD_Components
{
class FTL;

/*
	* This class implements a transaction scheduling unit which supports:
	* 1. Out-of-order execution of flash transactions, similar to the Sprinkler proposal
	*    described in "Jung et al., Sprinkler: Maximizing resource utilization in many-chip
	*    solid state disks, HPCA, 2014".
	* 2. Program and erase suspension, similar to the proposal described in "G. Wu and X. He,
	*    Reducing SSD read latency via NAND flash program and erase suspension, FAST 2012".
	*/
class TSU_OutOfOrder : public TSU_Base
{
public:
	TSU_OutOfOrder(const sim_object_id_type &id, FTL *ftl,
				   NVM_PHY_ONFI_NVDDR2 *NVMController,
				   unsigned int Channel_no,
				   unsigned int chip_no_per_channel,
				   unsigned int DieNoPerChip,
				   unsigned int PlaneNoPerDie,
				   sim_time_type WriteReasonableSuspensionTimeForRead,
				   sim_time_type EraseReasonableSuspensionTimeForRead,
				   sim_time_type EraseReasonableSuspensionTimeForWrite,
				   bool EraseSuspensionEnabled,
				   bool ProgramSuspensionEnabled);
	~TSU_OutOfOrder();

	void Schedule();

	void Start_simulation();
	void Validate_simulation_config();
	void Execute_simulator_event(MQSimEngine::Sim_Event *);
	void Report_results_in_XML(std::string name_prefix, Utils::XmlWriter &xmlwriter);

private:
	Flash_Transaction_Queue **UserReadTRQueue;
	Flash_Transaction_Queue **UserWriteTRQueue;
	Flash_Transaction_Queue **GCReadTRQueue;
	Flash_Transaction_Queue **GCWriteTRQueue;
	Flash_Transaction_Queue **GCEraseTRQueue;
	Flash_Transaction_Queue **MappingReadTRQueue;
	Flash_Transaction_Queue **MappingWriteTRQueue;

	bool service_read_transaction(NVM::FlashMemory::Flash_Chip *chip);
	bool service_write_transaction(NVM::FlashMemory::Flash_Chip *chip);
	bool service_erase_transaction(NVM::FlashMemory::Flash_Chip *chip);
};
} // namespace SSD_Components

#endif // TSU_OUTOFORDER_H
