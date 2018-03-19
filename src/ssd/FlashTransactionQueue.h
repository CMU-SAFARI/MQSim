#ifndef FLASH_TRANSACTION_QUEUE_H
#define FLASH_TRANSACTION_QUEUE_H

#include <list>
#include <string>
#include "NVM_Transaction_Flash.h"
#include "Queue_Probe.h"
#include "../sim/Sim_Reporter.h"

namespace SSD_Components
{
	class FlashTransactionQueue : public std::list<NVM_Transaction_Flash*>, public MQSimEngine::Sim_Reporter
	{
	public:
		FlashTransactionQueue();
		FlashTransactionQueue(std::string id);
		void Set_id(std::string id);
		void push_back(NVM_Transaction_Flash* const&);
		void push_front(NVM_Transaction_Flash* const&);
		std::list<NVM_Transaction_Flash*>::iterator insert(list<NVM_Transaction_Flash*>::iterator position, NVM_Transaction_Flash* const& transaction);
		void remove(NVM_Transaction_Flash* const& transaction);
		void remove(std::list<NVM_Transaction_Flash*>::iterator const& itr_pos);
		void pop_front();
		void Report_results_in_XML(std::string name_prefix, Utils::XmlWriter& xmlwriter);
	private:
		std::string id;
		Queue_Probe RequestQueueProbe;
	};
}

#endif // !FLASH_TRANSACTION_QUEUE_H
