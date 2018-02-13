/*
#include <iostream>
#include "Controller.h"
#include "HostInterfaceNVMe.h"

using std::cout;

namespace SSD_Components {
	Controller::Controller(std::string id, SimulationMode simulationMode, InitialSimulationStatus initialSSDStatus, uint32_t percentageOfValidPages, uint32_t validPagesStdDev,
		FCC* fcc, unsigned int channel_count, unsigned int die_no_per_chip, unsigned int plane_no_per_die, unsigned int block_no_per_plane, unsigned int pages_no_per_block,
		bool loggingEnabled, int logLineCount, std::string logFilePath, int seed) : Sim_Object(id),
		simulationMode(simulationMode), InitialStatus(initialSSDStatus),
		initialPercentageOfValidPages(percentageOfValidPages), initialPercentagesOfValidPagesStdDev(validPagesStdDev),
		_FCC(fcc), channel_count(channel_count), die_no_per_chip(die_no_per_chip), plane_no_per_die(plane_no_per_die), pages_no_per_block(pages_no_per_block),
		loggingEnabled(loggingEnabled)
	{
		if (loggingEnabled)
		{
			this->logLineCount = logLineCount;
			logFile = new std::ofstream(logFilePath);
		}

		pageStatusRandomGenerator = new RandomGenerator(seed);
	}

	Controller::~Controller()
	{
		delete logFile;
		delete pageStatusRandomGenerator;
	}


	void Controller::Start_simulation()
	{
		if (simulationMode == SimulationMode::STANDALONE) {
			cout << "\n";
			cout << ".................................................\n";
			cout << "Preparing address space";
			prepareInitialState();
			_HostInterface->PreprocessInputTraffic();
			ftl->ManageUnallocatedValidPages();
			cout << "Prepration finished\n";
			cout << ".................................................\n";

			if (loggingEnabled)
			{
				loggingPeriod = simulationStopTime / (ulong)logLineCount;
				cout << "Time(us)";
				cout << "\tGCRate(gc/s)";

				if (_HostInterface->Type() == HostInterfaceType::NVME)
				{
					std::vector<InputStream*> inputStreams = ((HostInterfaceNVMe *)_HostInterface)->InputStreams();
					for(std::vector<InputStream*>::iterator IS = inputStreams.begin();
						IS != inputStreams.end(); IS++)
					{
						cout << "\t" + (*IS)->StreamName + "_AverageResponseTime(us)";
						cout << "\t" + (*IS)->StreamName + "_AverageResponseTime_ShortTerm(us)";
						cout << "\t" + (*IS)->StreamName + "_AverageResponseTimeR_ShortTerm(us)";
						cout << "\t" + (*IS)->StreamName + "_AverageResponseTimeW_ShortTerm(us)";
						cout << "\t" + (*IS)->StreamName + "_AverageReadCMDLifeTime_ShortTerm(us)";
						cout << "\t" + (*IS)->StreamName + "_AverageProgramCMDLifeTime(us)_ShortTerm";
						//cout << "\t" + IS.FlowName + "_AverageResponseTime(us)";
						//cout << "\t" + IS.FlowName + "_AverageResponseTimeR(us)";
						//cout << "\t" + IS.FlowName + "_AverageResponseTimeW(us)";
						//cout << "\t" + IS.FlowName + "_AverageReadCMDLifeTime(us)";
						//cout << "\t" + IS.FlowName + "_AverageProgramCMDLifeTime(us)";
					}
				}
				cout << "\tGCRate_ShortTerm(gc/s)";
				cout << "\tChannelReadQueueLength";
				cout << "\tChannelWriteQueueLength";
				cout << "\tChipReadQueueLength";
				cout << "\tChipWriteQueueLength";
				cout << "\tChannelReadWaitingTime";
				cout << "\tChannelWriteWaitingTime";
				cout << "\tChipReadWaitingTime";
				cout << "\tChipWriteWaitingTime";
				if (ftl->GetIOSchedulerType() == IOSchedulerType::SPRINKLER)
				{
					for (int i = 0; i < channel_count; i++)
						cout << "\tAvgChannelReadQueueLength_" + std::to_string(i);
					for (int i = 0; i < channel_count; i++)
						cout << "\tAvgChannelWriteQueueLength_" + std::to_string(i);
					for (int i = 0; i < channel_count; i++)
						cout << "\tAvgChannelReadWaitingTime_" + std::to_string(i);
					for (int i = 0; i < channel_count; i++)
						cout << "\tAvgChannelWriteWaitingTime_" + std::to_string(i);
				}
				for (int i = 0; i < channel_count; i++)
					for (int j = 0; j < chip_no_per_channel; j++)
						cout << "\tAvgChipReadQueueLength_" + std::to_string(i) + "_" + std::to_string(j);
				for (int i = 0; i < channel_count; i++)
					for (int j = 0; j < chip_no_per_channel; j++)
						cout << "\tAvgChipWriteQueueLength_" + std::to_string(i) + "_" + std::to_string(j);
				for (int i = 0; i < channel_count; i++)
					for (int j = 0; j < chip_no_per_channel; j++)
						cout << "\tAvgChipReadWaitingTime_" + std::to_string(i) + "_" + std::to_string(j);
				for (int i = 0; i < channel_count; i++)
					for (int j = 0; j < chip_no_per_channel; j++)
						cout << "\tAvgChipWriteWaitingTime_" + std::to_string(i) + "_" + std::to_string(j);
				cout << "\n";
				Simulator->Register_sim_event(loggingPeriod, this, NULL, 0);
			}
		}
	}

	void Controller::prepareInitialState()
	{
		switch (InitialStatus)
		{
		case InitialSimulationStatus::STEADY_STATE:
			double validPagesAverage = ((double)initialPercentageOfValidPages / 100) * pages_no_per_block;
			double validPagesStdDev = ((double)initialPercentagesOfValidPagesStdDev / 100) * pages_no_per_block;
			uint totalAvailableBlocks = block_no_per_plane - (ftl->GetEmergencyThresholdPerPlane() / pages_no_per_block) - 1;
			for (flash_channel_ID_type channelID = 0; channelID < channel_count; channelID++)
			{
				for (flash_chip_ID_type chipID = 0; chipID < chip_no_per_channel; chipID++)
					for (flash_die_ID_type dieID = 0; dieID < die_no_per_chip; dieID++)
						for (flash_plane_ID_type planeID = 0; planeID < plane_no_per_die; planeID++)
						{
							for (flash_block_ID_type blockID = 0; blockID < totalAvailableBlocks; blockID++)
							{
								double randomValue = pageStatusRandomGenerator->Normal(validPagesAverage, validPagesStdDev);
								if (randomValue < 0)
									randomValue = validPagesAverage + (validPagesAverage - randomValue);
								uint32_t numberOfValidPages = (uint32_t)(randomValue);// * FTL.PagesNoPerBlock);
								if (numberOfValidPages > pages_no_per_block)
									numberOfValidPages = pages_no_per_block;
								for (flash_page_ID_type pageID = 0; pageID < numberOfValidPages; pageID++)
									ftl->MakeDummyValidPage(channelID, chipID, dieID, planeID, blockID, pageID);
								for (flash_page_ID_type pageID = numberOfValidPages; pageID < pages_no_per_block; pageID++)
									ftl->MakeDummyInvalidPage(channelID, chipID, dieID, planeID, blockID, pageID);

								if ((chips[channelID][chipID])->Dies[dieID]->Planes[planeID]->Blocks[blockID]->FreePageNo != 0)
									throw "Problem in preparing SSD intitial status!";
								if ((chips[channelID][chipID])->Dies[dieID]->Planes[planeID]->Blocks[blockID]->LastWrittenPageNo != 255)
									throw "Problem in preparing SSD intitial status!";
							}
						}
			}
			break;
		case InitialSimulationStatus::EMPTY:
			break;
		default:
			throw "Unhandled initial status!";
		}
	}

	void Controller::writeLog()
	{
		logFile << Simulator->Time() / 1000;
		logFile << "\t" + FTL.GarbageCollector.AverageEmergencyGCRate);
		if (_HostInterface->Type() == HostInterfaceType::NVME)
		{
			std::vector<InputStream*> inputStreams = ((HostInterfaceNVMe *)_HostInterface)->InputStreams();
			for (std::vector<InputStream*>::iterator IS = inputStreams.begin();
				IS != inputStreams.end(); IS++)
			{
				logFile << "\t" + (*IS)->AvgResponseTime();
				logFile << "\t" + (*IS)->AvgResponseTime_CurrentLogRound();
				logFile << "\t" + (*IS)->AvgResponseTimeR_CurrentLogRound();
				logFile << "\t" + (*IS)->AvgResponseTimeW_CurrentLogRound();
				logFile << "\t" + (*IS)->AverageReadCMDLifeTime_CurrentLogRound();
				logFile << "\t" + (*IS)->AverageProgramCMDLifeTime_CurrentLogRound();
				//logFile << "\t" + (*IS)->AvgResponseTime);
				//logFile << "\t" + (*IS)->AvgResponseTimeR);
				//logFile << "\t" + (*IS)->AvgResponseTimeW);
				//logFile << "\t" + (*IS)->AverageReadCMDLifeTime);
				//logFile << "\t" + (*IS)->AverageProgramCMDLifeTime);
			}
		}
		logFile << "\t" + FTL.GarbageCollector.AverageEmergencyGCRate_CurrentLogRound);
		if (FTL.SchedulingPolicy == IOSchedulingPolicy.Sprinkler)
		{
			double sumChannelReadQueueLength = 0, sumChannelWriteQueueLength = 0, sumChannelReadWaitingTimeLength = 0, sumChannelWriteWaitingTimeLength = 0;
			double sumChipReadQueueLength = 0, sumChipWriteQueueLength = 0, sumChipReadWaitingTimeLength = 0, sumChipWriteWaitingTimeLength = 0;
			for (int i = 0; i < FTL.ChannelCount; i++)
			{
				sumChannelReadQueueLength += (FTL.ChannelInfos[i] as BusChannelSprinkler).WaitingFlashReadReqsQueue.RequestQueueProbe.AvgQueueLengthEpoch;
				sumChannelWriteQueueLength += (FTL.ChannelInfos[i] as BusChannelSprinkler).WaitingFlashWriteReqsQueue.RequestQueueProbe.AvgQueueLengthEpoch;
				sumChannelReadWaitingTimeLength += (FTL.ChannelInfos[i] as BusChannelSprinkler).WaitingFlashReadReqsQueue.RequestQueueProbe.AvgWaitingTimeEpoch;
				sumChannelWriteWaitingTimeLength += (FTL.ChannelInfos[i] as BusChannelSprinkler).WaitingFlashWriteReqsQueue.RequestQueueProbe.AvgWaitingTimeEpoch;
				for (int j = 0; j < FTL.ChipNoPerChannel; j++)
				{
					sumChipReadQueueLength = FTL.ChannelInfos[i].FlashChips[j].WaitingUserReadQueue.RequestQueueProbe.AvgQueueLengthEpoch;
					sumChipWriteQueueLength = FTL.ChannelInfos[i].FlashChips[j].WaitingUserWriteQueue.RequestQueueProbe.AvgQueueLengthEpoch;
					sumChipReadWaitingTimeLength = FTL.ChannelInfos[i].FlashChips[j].WaitingUserReadQueue.RequestQueueProbe.AvgWaitingTimeEpoch;
					sumChipWriteWaitingTimeLength = FTL.ChannelInfos[i].FlashChips[j].WaitingUserWriteQueue.RequestQueueProbe.AvgWaitingTimeEpoch;
				}
			}
			logFile << "\t" + (sumChannelReadQueueLength / FTL.ChannelCount).ToString());
			logFile << "\t" + (sumChannelWriteQueueLength / FTL.ChannelCount).ToString());
			logFile << "\t" + (sumChipReadQueueLength / FTL.ChannelCount).ToString());
			logFile << "\t" + (sumChipWriteQueueLength / FTL.ChannelCount).ToString());
			logFile << "\t" + (sumChannelReadWaitingTimeLength / FTL.ChannelCount).ToString());
			logFile << "\t" + (sumChannelWriteWaitingTimeLength / FTL.ChannelCount).ToString());
			logFile << "\t" + (sumChipReadWaitingTimeLength / FTL.ChannelCount).ToString());
			logFile << "\t" + (sumChipWriteWaitingTimeLength / FTL.ChannelCount).ToString());
			for (int i = 0; i < FTL.ChannelCount; i++)
				logFile << "\t" + (FTL.ChannelInfos[i] as BusChannelSprinkler).WaitingFlashReadReqsQueue.RequestQueueProbe.AvgQueueLengthEpoch);
			for (int i = 0; i < FTL.ChannelCount; i++)
				logFile << "\t" + (FTL.ChannelInfos[i] as BusChannelSprinkler).WaitingFlashWriteReqsQueue.RequestQueueProbe.AvgQueueLengthEpoch);
			for (int i = 0; i < FTL.ChannelCount; i++)
				logFile << "\t" + (FTL.ChannelInfos[i] as BusChannelSprinkler).WaitingFlashReadReqsQueue.RequestQueueProbe.AvgWaitingTimeEpoch);
			for (int i = 0; i < FTL.ChannelCount; i++)
				logFile << "\t" + (FTL.ChannelInfos[i] as BusChannelSprinkler).WaitingFlashWriteReqsQueue.RequestQueueProbe.AvgWaitingTimeEpoch);
			for (int i = 0; i < FTL.ChannelCount; i++)
			{
				(FTL.ChannelInfos[i] as BusChannelSprinkler).WaitingFlashReadReqsQueue.RequestQueueProbe.ResetEpochStatistics();
				(FTL.ChannelInfos[i] as BusChannelSprinkler).WaitingFlashWriteReqsQueue.RequestQueueProbe.ResetEpochStatistics();
			}
		}

		for (int i = 0; i < FTL.ChannelCount; i++)
			for (int j = 0; j < FTL.ChipNoPerChannel; j++)
				logFile << "\t" + FTL.ChannelInfos[i].FlashChips[j].WaitingUserReadQueue.RequestQueueProbe.AvgQueueLengthEpoch);
		for (int i = 0; i < FTL.ChannelCount; i++)
			for (int j = 0; j < FTL.ChipNoPerChannel; j++)
				logFile << "\t" + FTL.ChannelInfos[i].FlashChips[j].WaitingUserWriteQueue.RequestQueueProbe.AvgQueueLengthEpoch);
		for (int i = 0; i < FTL.ChannelCount; i++)
			for (int j = 0; j < FTL.ChipNoPerChannel; j++)
			{
				logFile << "\t" + FTL.ChannelInfos[i].FlashChips[j].WaitingUserReadQueue.RequestQueueProbe.AvgWaitingTimeEpoch);
				FTL.ChannelInfos[i].FlashChips[j].WaitingUserReadQueue.RequestQueueProbe.ResetEpochStatistics();
			}
		for (int i = 0; i < FTL.ChannelCount; i++)
			for (int j = 0; j < FTL.ChipNoPerChannel; j++)
			{
				logFile << "\t" + FTL.ChannelInfos[i].FlashChips[j].WaitingUserWriteQueue.RequestQueueProbe.AvgWaitingTimeEpoch);
				FTL.ChannelInfos[i].FlashChips[j].WaitingUserWriteQueue.RequestQueueProbe.ResetEpochStatistics();
			}
		logFile << "\n");
		if (HostInterface is HostInterfaceNVMe)
			foreach(InputStreamBase IS in(HostInterface as HostInterfaceNVMe).InputStreams)
			IS.ResetShortTermStatistics();
		FTL.GarbageCollector.ResetShortTermStatistics();
	}

}*/
