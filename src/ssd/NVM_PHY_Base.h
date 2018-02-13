#ifndef NVM_PHY_BASE
#define NVM_PHY_BASE

#include "../sim/Sim_Object.h"

namespace SSD_Components
{
	class NVM_PHY_Base : public MQSimEngine::Sim_Object
	{
	public:
		NVM_PHY_Base(const sim_object_id_type& id);
		~NVM_PHY_Base();
	};
}

#endif // !NVM_PHY_BASE
