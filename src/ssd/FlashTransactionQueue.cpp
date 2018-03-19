#include "FlashTransactionQueue.h"

namespace SSD_Components
{
	FlashTransactionQueue::FlashTransactionQueue() {}

	FlashTransactionQueue::FlashTransactionQueue(std::string id) : id(id)
	{
	}

	void FlashTransactionQueue::Set_id(std::string id)
	{
		this->id = id;
	}

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

	void FlashTransactionQueue::Report_results_in_XML(std::string name_prefix, Utils::XmlWriter& xmlwriter)
	{
		std::string tmp = name_prefix;
		xmlwriter.Write_start_element_tag(tmp);

		std::string attr = "Name";
		std::string val = id;
		xmlwriter.Write_attribute_string_inline(attr, val);

		attr = "No_Of_Transactions_Enqueued";
		val = std::to_string(RequestQueueProbe.NRequests());
		xmlwriter.Write_attribute_string_inline(attr, val);

		attr = "No_Of_Transactions_Dequeued";
		val = std::to_string(RequestQueueProbe.NDepartures());
		xmlwriter.Write_attribute_string_inline(attr, val);

		attr = "Avg_Queue_Length";
		val = std::to_string(RequestQueueProbe.AvgQueueLength());
		xmlwriter.Write_attribute_string_inline(attr, val);

		attr = "Max_Queue_Length";
		val = std::to_string(RequestQueueProbe.MaxQueueLength());
		xmlwriter.Write_attribute_string_inline(attr, val);

		attr = "STDev_Queue_Length";
		val = std::to_string(RequestQueueProbe.STDevQueueLength());
		xmlwriter.Write_attribute_string_inline(attr, val);

		attr = "Avg_Transaction_Waiting_Time";
		val = std::to_string(RequestQueueProbe.AvgWaitingTime());
		xmlwriter.Write_attribute_string_inline(attr, val);

		attr = "Max_Transaction_Waiting_Time";
		val = std::to_string(RequestQueueProbe.MaxWaitingTime());
		xmlwriter.Write_attribute_string_inline(attr, val);

		xmlwriter.Write_end_element_tag();
	}
}