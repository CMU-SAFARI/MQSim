#ifndef EVENT_TREE_H
#define EVENT_TREE_H

#include "Sim_Defs.h"
#include "Sim_Event.h"

namespace MQSimEngine
{
	class EventTreeNode
	{
	public:
		// key provided by the calling class
		sim_time_type Key;
		// the data or value associated with the key
		Sim_Event* FirstSimEvent;
		Sim_Event* LastSimEvent;
		// color - used to balance the tree
		/*RED = 0 , BLACK = 1;*/
		int Color;
		// left node 
		EventTreeNode* Left;
		// right node 
		EventTreeNode* Right;
		// parent node 
		EventTreeNode* Parent;

		EventTreeNode()
		{
			Color = 0;
			Left = NULL;
			Right = NULL;
			Parent = NULL;
		}
	};

	class EventTree
	{
	public:
		EventTree();
		~EventTree();

		// the number of nodes contained in the tree
		int Count;
		//  sentinelNode is convenient way of indicating a leaf node.
		static EventTreeNode* SentinelNode;
		void Add(sim_time_type key, Sim_Event* data);
		void RotateLeft(EventTreeNode* x);
		void RotateRight(EventTreeNode* x);
		Sim_Event* GetData(sim_time_type key);
		void Insert_sim_event(Sim_Event* data);
		sim_time_type Get_min_key();
		Sim_Event* Get_min_value();
		EventTreeNode* Get_min_node();
		void Remove(sim_time_type key);
		void Remove(EventTreeNode* node);
		void Remove_min();
		void Clear();
	private:
		// the tree
		EventTreeNode* rbTree;
		// the node that was last found; used to optimize searches
		EventTreeNode* lastNodeFound;
		void RestoreAfterInsert(EventTreeNode* x);
		void Delete(EventTreeNode* z);
		void Restore_after_delete(EventTreeNode* x);
	};
}

#endif // !EVENT_TREE_H
