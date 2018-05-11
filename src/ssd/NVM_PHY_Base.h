#ifndef NVM_PHY_BASE
#define NVM_PHY_BASE

#include "../sim/Sim_Object.h"
#include "../nvm_chip/NVM_Chip.h"

namespace SSD_Components
{
	class NVM_PHY_Base : public MQSimEngine::Sim_Object
	{
	public:
		NVM_PHY_Base(const sim_object_id_type& id);
		~NVM_PHY_Base();
		virtual void Change_memory_status_preconditioning(const NVM::NVM_Memory_Address* address, const void* status_info) = 0;
	};
}

#endif // !NVM_PHY_BASE
