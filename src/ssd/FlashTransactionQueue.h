#ifndef FLASH_TRANSACTION_QUEUE_H
#define FLASH_TRANSACTION_QUEUE_H

#include <list>
#include "NVM_Transaction_Flash.h"
#include "Queue_Probe.h"

namespace SSD_Components
{
	class FlashTransactionQueue : public std::list<NVM_Transaction_Flash*>
	{
	public:
		void push_back(NVM_Transaction_Flash* const&);
		void push_front(NVM_Transaction_Flash* const&);
		std::list<NVM_Transaction_Flash*>::iterator insert(list<NVM_Transaction_Flash*>::iterator position, NVM_Transaction_Flash* const& transaction);
		void remove(NVM_Transaction_Flash* const& transaction);
		void remove(std::list<NVM_Transaction_Flash*>::iterator const& itr_pos);
		void pop_front();
	private:
		Queue_Probe RequestQueueProbe;
	};
}

#endif // !FLASH_TRANSACTION_QUEUE_H
