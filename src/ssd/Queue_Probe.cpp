#include <cmath>
#include "Queue_Probe.h"
#include "../sim/Engine.h"

namespace SSD_Components
{
	StateStatistics::StateStatistics()
	{
		this->nEnterances = 0;
		this->totalTime = 0;
	}

	Queue_Probe::Queue_Probe()
	{
		states.push_back(StateStatistics());
		statesEpoch.push_back(StateStatistics());
	}

	void Queue_Probe::EnqueueRequest(NVM_Transaction* transaction)
	{
		if (transaction == NULL)
			PRINT_ERROR("Inserting a null object to queue!")

		currentObjectsInQueue[transaction] = Simulator->Time();
		nRequests++;
		nRequestsEpoch++;
		setCount(count + 1);
	}

	void Queue_Probe::DequeueRequest(NVM_Transaction* transaction)
	{
		nDepartures++;
		nDeparturesEpoch++;

		if (transaction == NULL)
			throw std::logic_error("Object can not be null if accurateTimingEnabled=ture");
		sim_time_type et = currentObjectsInQueue[transaction];
		currentObjectsInQueue.erase(transaction);
		sim_time_type tc = Simulator->Time() - et;
		totalWaitingTime += tc;
		if (tc > maxWaitingTime)
			maxWaitingTime = tc;
		totalWaitingTimeEpoch += tc;
		setCount(count - 1);
	}

	void Queue_Probe::setCount(int val)
	{
		sim_time_type n = Simulator->Time();
		states[count].totalTime += n - lastCountChange;
		statesEpoch[count].totalTime += n - lastCountChange;

		lastCountChange = n;
		count = val;
		if (count > maxQueueLength) {
			maxQueueLength = count;
		}
		while (count + 1 > states.size()) {
			states.push_back(StateStatistics());
		}
		states[count].nEnterances++;
		while (count + 1 > statesEpoch.size()) {
			statesEpoch.push_back(StateStatistics());
		}
		statesEpoch[count].nEnterances++;
	}

	void Queue_Probe::ResetEpochStatistics()
	{
		nRequestsEpoch = 0;
		nDeparturesEpoch = 0;
		totalWaitingTimeEpoch = 0;
		epochStartTime = Simulator->Time();

		for(unsigned int i = 0; i < statesEpoch.size(); i++) {
			statesEpoch[i].totalTime = 0;
			statesEpoch[i].nEnterances = 0;
		}
	}

	void Queue_Probe::Snapshot(std::string id, Utils::XmlWriter& writer)
	{
		writer.Write_start_element_tag(id + "_QueueProbe");
		writer.Write_attribute_string("MaxQueueLength", std::to_string(MaxQueueLength()));
		writer.Write_attribute_string("AvgQueueLength", std::to_string(AvgQueueLength()));
		writer.Write_attribute_string("MaxWaitingTime", std::to_string(MaxWaitingTime()));
		writer.Write_attribute_string("AvgWaitingTime", std::to_string(AvgWaitingTime()));
		writer.Write_attribute_string("NRequests", std::to_string(NRequests()));
		writer.Write_attribute_string("NDepartures", std::to_string(NDepartures()));
		for (unsigned int i = 0; i < states.size(); i++) {
			writer.Write_start_element_tag(id + "_Distribution");
			sim_time_type now = Simulator->Time();
			sim_time_type t = states[i].totalTime;
			if (count == i) {
				t += now - lastCountChange;
			}
			double r = (double)t / (double)(now - lastCountChangeReference);
			writer.Write_attribute_string("QueueLength", std::to_string(i));
			writer.Write_attribute_string("nEnterances", std::to_string(states[i].nEnterances));
			writer.Write_attribute_string("totalTime", std::to_string(t));
			writer.Write_attribute_string("totalTimeRatio", std::to_string(r));
			writer.Write_end_element_tag();
		}
		writer.Write_end_element_tag();//id + "_QueueProbe"
	}

	unsigned long Queue_Probe::NRequests()
	{
		return nRequests;
	}

	unsigned long Queue_Probe::NDepartures()
	{
		return nDepartures;
	}

	int Queue_Probe::QueueLength()
	{
		return count;
	}

	std::vector<StateStatistics> Queue_Probe::States()
	{
		return states;
	}

	unsigned int Queue_Probe::MaxQueueLength()
	{
		return maxQueueLength;
	}

	double Queue_Probe::AvgQueueLength()
	{
		sim_time_type sum = 0;
		for (unsigned int len = 0; len < states.size(); len++)
			sum += states[len].totalTime * len;
		return (double)sum / (double)Simulator->Time();
	}

	double Queue_Probe::STDevQueueLength()
	{
		sim_time_type sum = 0;
		for (unsigned int len = 0; len < states.size(); len++) {
			sum += states[len].totalTime * len;
		}
		double mean = (double)sum / (double)Simulator->Time();
		double stdev = 0.0;
		for (unsigned int len = 0; len < states.size(); len++) {
			stdev += std::pow((double)states[len].totalTime * len / (double)Simulator->Time() - mean, 2);
		}

		return std::sqrt(stdev);
	}

	double Queue_Probe::AvgQueueLengthEpoch()
	{
		sim_time_type sum = 0;
		sim_time_type sumTime = 0;
		for (unsigned int len = 0; len < states.size(); len++) {
			sum += statesEpoch[len].totalTime * len;
			sumTime += statesEpoch[len].totalTime;
		}

		return (double)sum / (double)(Simulator->Time() - epochStartTime);
	}

	sim_time_type Queue_Probe::MaxWaitingTime()
	{
		return maxWaitingTime / 1000;//convert nano-seconds to micro-seconds
	}

	sim_time_type Queue_Probe::AvgWaitingTime()
	{
		return (sim_time_type)((double)totalWaitingTime / (double)(nDepartures * 1000));//convert nano-seconds to micro-seconds
	}

	sim_time_type Queue_Probe::AvgWaitingTimeEpoch()
	{
		return (sim_time_type)((double)totalWaitingTimeEpoch / (double)(nDeparturesEpoch * 1000));
	}

	sim_time_type Queue_Probe::TotalWaitingTime()
	{
		return totalWaitingTime;
	}
}