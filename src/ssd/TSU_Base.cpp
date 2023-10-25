#include <string>
#include "TSU_Base.h"

namespace SSD_Components
{
	TSU_Base* TSU_Base::_my_instance = NULL;

	TSU_Base::TSU_Base(const sim_object_id_type& id, FTL* ftl, NVM_PHY_ONFI_NVDDR2* NVMController, Flash_Scheduling_Type Type,
		unsigned int ChannelCount, unsigned int chip_no_per_channel, unsigned int DieNoPerChip, unsigned int PlaneNoPerDie,
		bool EraseSuspensionEnabled, bool ProgramSuspensionEnabled,
		sim_time_type WriteReasonableSuspensionTimeForRead,
		sim_time_type EraseReasonableSuspensionTimeForRead,
		sim_time_type EraseReasonableSuspensionTimeForWrite)
		: Sim_Object(id), ftl(ftl), _NVMController(NVMController), type(Type),
		channel_count(ChannelCount), chip_no_per_channel(chip_no_per_channel), die_no_per_chip(DieNoPerChip), plane_no_per_die(PlaneNoPerDie),
		eraseSuspensionEnabled(EraseSuspensionEnabled), programSuspensionEnabled(ProgramSuspensionEnabled),
		writeReasonableSuspensionTimeForRead(WriteReasonableSuspensionTimeForRead), eraseReasonableSuspensionTimeForRead(EraseReasonableSuspensionTimeForRead),
		eraseReasonableSuspensionTimeForWrite(EraseReasonableSuspensionTimeForWrite), opened_scheduling_reqs(0)
	{
		_my_instance = this;
		Round_robin_turn_of_channel = new flash_chip_ID_type[channel_count];
		for (unsigned int channelID = 0; channelID < channel_count; channelID++)
			Round_robin_turn_of_channel[channelID] = 0;
	}

	TSU_Base::~TSU_Base()
	{
		delete[] Round_robin_turn_of_channel;
	}

	void TSU_Base::Setup_triggers()
	{
		Sim_Object::Setup_triggers();
		_NVMController->ConnectToTransactionServicedSignal(handle_transaction_serviced_signal_from_PHY);
		_NVMController->ConnectToChannelIdleSignal(handle_channel_idle_signal);
		_NVMController->ConnectToChipIdleSignal(handle_chip_idle_signal);
	}

	void TSU_Base::handle_transaction_serviced_signal_from_PHY(NVM_Transaction_Flash* transaction)
	{
		//TSU does nothing. The generator of the transaction will handle it.
	}

	void TSU_Base::handle_channel_idle_signal(flash_channel_ID_type channelID)
	{
		for (unsigned int i = 0; i < _my_instance->chip_no_per_channel; i++) {
			NVM::FlashMemory::Flash_Chip* chip = _my_instance->_NVMController->Get_chip(channelID, _my_instance->Round_robin_turn_of_channel[channelID]);
			//The TSU does not check if the chip is idle or not since it is possible to suspend a busy chip and issue a new command
			if (!_my_instance->service_read_transaction(chip))
				if (!_my_instance->service_write_transaction(chip))
					_my_instance->service_erase_transaction(chip);
			_my_instance->Round_robin_turn_of_channel[channelID] = (flash_chip_ID_type)(_my_instance->Round_robin_turn_of_channel[channelID] + 1) % _my_instance->chip_no_per_channel;

			//A transaction has been started, so TSU should stop searching for another chip
			if (_my_instance->_NVMController->Get_channel_status(chip->ChannelID) == BusChannelStatus::BUSY)
				break;
		}
	}
	
	void TSU_Base::handle_chip_idle_signal(NVM::FlashMemory::Flash_Chip* chip)
	{
		if (_my_instance->_NVMController->Get_channel_status(chip->ChannelID) == BusChannelStatus::IDLE)
		{
			if (!_my_instance->service_read_transaction(chip))
				if (!_my_instance->service_write_transaction(chip))
					_my_instance->service_erase_transaction(chip);
		}
	}

	void TSU_Base::Report_results_in_XML(std::string name_prefix, Utils::XmlWriter& xmlwriter)
	{
	}
}