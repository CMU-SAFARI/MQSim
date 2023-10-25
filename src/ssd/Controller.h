#ifndef CONTROLLER_H
#define CONTROLLER_H

#include <fstream>
#include "../sim/Sim_Defs.h"
#include "../sim/Sim_Object.h"
#include "../sim/Sim_Event.h"
#include "../utils/RandomGenerator.h"
#include "../nvm_chip/flash_memory/Chip.h"
#include "FTL.h"

namespace SSD_Components
{
	class Controller : MQSimEngine::Sim_Object
	{
	public:
		Controller(sim_object_id_type id, SimulationMode simulationMode, InitialSimulationStatus initialSSDStatus, uint32_t percentageOfValidPages, uint32_t validPagesStdDev,
			unsigned int channel_count, unsigned int die_no_per_chip, unsigned int plane_no_per_die, unsigned int block_no_per_plane, unsigned int pages_no_per_block,
			bool loggingEnabled, int logLineCount, std::string logFilePath, int seed);
	private:
		NVM::FlashMemory::Chip*** chips;
		InitialSimulationStatus InitialStatus;
		int logLineCount;
		sim_time_type simulationStopTime;
		sim_time_type loggingPeriod;
		std::ofstream* logFile;
		bool loggingEnabled;
		uint32_t initialPercentageOfValidPages, initialPercentagesOfValidPagesStdDev;
		RandomGenerator* pageStatusRandomGenerator;
		SimulationMode simulationMode;
		unsigned int channel_count, ChipNoPerChannel, die_no_per_chip, plane_no_per_die, block_no_per_plane, pages_no_per_block;
		void prepareInitialState();
		void writeLog();
	};
}

#endif // !CONTROLLER_H
