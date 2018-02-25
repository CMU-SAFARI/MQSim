#ifndef QUEUE_PROBE_H
#define QUEUE_PROBE_H

#include <unordered_map>
#include <vector>
#include <string>
#include "../sim/Sim_Defs.h"
#include "NVM_Transaction.h"
#include "../utils/XMLWriter.h"

namespace SSD_Components
{
	class StateStatistics
	{
	public:
		StateStatistics();
		unsigned long nEnterances;
		sim_time_type totalTime;
	};

	class Queue_Probe
	{
	public:
		Queue_Probe();
		void EnqueueRequest(NVM_Transaction* transaction);
		void DequeueRequest(NVM_Transaction* transaction);
		void ResetEpochStatistics();
		void Snapshot(std::string id, Utils::XmlWriter& writer);
		unsigned long NRequests();
		unsigned long NDepartures();
		int QueueLength();
		std::vector<StateStatistics> States();
		unsigned int MaxQueueLength();
		double AvgQueueLength();
		double STDevQueueLength();
		double AvgQueueLengthEpoch();
		sim_time_type MaxWaitingTime();
		sim_time_type AvgWaitingTime();
		sim_time_type AvgWaitingTimeEpoch();
		sim_time_type TotalWaitingTime();
	private:
		unsigned int count = 0;
		unsigned long nRequests = 0;
		unsigned long nDepartures = 0;
		sim_time_type totalWaitingTime = 0;
		sim_time_type lastCountChange = 0;
		sim_time_type lastCountChangeReference = 0;
		unsigned long nRequestsEpoch = 0;
		unsigned long nDeparturesEpoch = 0;
		sim_time_type totalWaitingTimeEpoch = 0;
		sim_time_type epochStartTime;
		std::vector<StateStatistics> states;
		std::vector<StateStatistics> statesEpoch;
		std::unordered_map<NVM_Transaction*, sim_time_type> currentObjectsInQueue;
		unsigned int maxQueueLength = 0;
		sim_time_type maxWaitingTime = 0;
		void setCount(int val);
	};

}

#endif // !QUEUE_PROBE
