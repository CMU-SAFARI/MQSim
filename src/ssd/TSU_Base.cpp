#include "TSU_Base.h"

namespace SSD_Components
{
	TSU_Base* TSU_Base::_myInstance = NULL;

	TSU_Base::TSU_Base(const sim_object_id_type& id, FTL* ftl, NVM_PHY_ONFI_NVDDR2* NVMController, Flash_Scheduling_Type Type,
		unsigned int ChannelCount, unsigned int ChipNoPerChannel, unsigned int DieNoPerChip, unsigned int PlaneNoPerDie,
		bool EraseSuspensionEnabled, bool ProgramSuspensionEnabled,
		sim_time_type WriteReasonableSuspensionTimeForRead,
		sim_time_type EraseReasonableSuspensionTimeForRead,
		sim_time_type EraseReasonableSuspensionTimeForWrite)
		: Sim_Object(id), ftl(ftl), _NVMController(NVMController), type(Type),
		channel_count(ChannelCount), chip_no_per_channel(ChipNoPerChannel), die_no_per_chip(DieNoPerChip), plane_no_per_die(PlaneNoPerDie),
		eraseSuspensionEnabled(EraseSuspensionEnabled), programSuspensionEnabled(ProgramSuspensionEnabled),
		writeReasonableSuspensionTimeForRead(WriteReasonableSuspensionTimeForRead), eraseReasonableSuspensionTimeForRead(EraseReasonableSuspensionTimeForRead),
		eraseReasonableSuspensionTimeForWrite(EraseReasonableSuspensionTimeForWrite)
	{
		_myInstance = this;
		LastChip = new flash_chip_ID_type[channel_count];
		UserReadTRQueue = new FlashTransactionQueue*[channel_count];
		UserWriteTRQueue = new FlashTransactionQueue*[channel_count];
		GCReadTRQueue = new FlashTransactionQueue*[channel_count];
		GCWriteTRQueue = new FlashTransactionQueue*[channel_count];
		GCEraseTRQueue = new FlashTransactionQueue*[channel_count];
		MappingReadTRQueue = new FlashTransactionQueue*[channel_count];
		MappingWriteTRQueue = new FlashTransactionQueue*[channel_count];
		for (unsigned int channelID = 0; channelID < channel_count; channelID++)
		{
			LastChip[channelID] = 0;
			UserReadTRQueue[channelID] = new FlashTransactionQueue[chip_no_per_channel];
			UserWriteTRQueue[channelID] = new FlashTransactionQueue[chip_no_per_channel];;
			GCReadTRQueue[channelID] = new FlashTransactionQueue[chip_no_per_channel];;
			GCWriteTRQueue[channelID] = new FlashTransactionQueue[chip_no_per_channel];;
			GCEraseTRQueue[channelID] = new FlashTransactionQueue[chip_no_per_channel];;
			MappingReadTRQueue[channelID] = new FlashTransactionQueue[chip_no_per_channel];;
			MappingWriteTRQueue[channelID] = new FlashTransactionQueue[chip_no_per_channel];;
		}
	}

	void TSU_Base::Setup_triggers()
	{
		Sim_Object::Setup_triggers();
		_NVMController->ConnectToTransactionServicedSignal(handle_transaction_serviced_signal);
		_NVMController->ConnectToChannelIdleSignal(handleChannelIdleSignal);
		_NVMController->ConnectToChipIdleSignal(handleChipIdleSignal);
	}

	void TSU_Base::handle_transaction_serviced_signal(NVM_Transaction_Flash* transaction)
	{
		//TSU does nothing. The generator of the transaction will handle it.
	}

	void TSU_Base::handleChannelIdleSignal(flash_channel_ID_type channelID)
	{
		for (unsigned int i = 0; i < _myInstance->chip_no_per_channel; i++) {
			NVM::FlashMemory::Chip* chip = _myInstance->_NVMController->GetChip(channelID, _myInstance->LastChip[channelID]);
			//The TSU does not check if the chip is idle or not since it is possible to suspend a busy chip and issue a new command
			if (!_myInstance->serviceReadTransaction(chip))
				if (!_myInstance->serviceWriteTransaction(chip))
					_myInstance->serviceEraseTransaction(chip);
			_myInstance->LastChip[channelID] = (flash_chip_ID_type)(_myInstance->LastChip[channelID] + 1) % _myInstance->chip_no_per_channel;

			//A transaction has been started, so TSU should stop searching for another chip
			if (_myInstance->_NVMController->GetChannelStatus(chip->ChannelID) == BusChannelStatus::BUSY)
				break;
		}
	}
	
	void TSU_Base::handleChipIdleSignal(NVM::FlashMemory::Chip* chip)
	{
		if (_myInstance->_NVMController->GetChannelStatus(chip->ChannelID) == BusChannelStatus::IDLE)
		{
			if (!_myInstance->serviceReadTransaction(chip))
				if (!_myInstance->serviceWriteTransaction(chip))
					_myInstance->serviceEraseTransaction(chip);
		}
	}
}