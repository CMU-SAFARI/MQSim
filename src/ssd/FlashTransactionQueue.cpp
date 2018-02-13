#include "FlashTransactionQueue.h"

namespace SSD_Components
{
	void FlashTransactionQueue::push_back(NVM_Transaction_Flash* const& transaction)
	{
		RequestQueueProbe.EnqueueRequest(transaction);
		return list<NVM_Transaction_Flash*>::push_back(transaction);
	}
	void FlashTransactionQueue::push_front(NVM_Transaction_Flash* const& transaction)
	{
		RequestQueueProbe.EnqueueRequest(transaction);
		return list<NVM_Transaction_Flash*>::push_front(transaction);
	}
	std::list<NVM_Transaction_Flash*>::iterator FlashTransactionQueue::insert(list<NVM_Transaction_Flash*>::iterator position, NVM_Transaction_Flash* const& transaction)
	{
		RequestQueueProbe.EnqueueRequest(transaction);
		return list<NVM_Transaction_Flash*>::insert(position, transaction);
	}
	void FlashTransactionQueue::remove(NVM_Transaction_Flash* const& transaction)
	{
		RequestQueueProbe.DequeueRequest(transaction);
		return list<NVM_Transaction_Flash*>::remove(transaction);
	}
	void FlashTransactionQueue::remove(std::list<NVM_Transaction_Flash*>::iterator const& itr_pos)
	{
		RequestQueueProbe.DequeueRequest(*itr_pos);
		list<NVM_Transaction_Flash*>::erase(itr_pos);
	}
	void FlashTransactionQueue::pop_front()
	{
		RequestQueueProbe.DequeueRequest(this->front());
		list<NVM_Transaction_Flash*>::pop_front();
	}
}