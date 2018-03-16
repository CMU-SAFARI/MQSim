#ifndef SIM_REPORTER_H
#define SIM_REPORTER_H

#include "../utils/XMLWriter.h"

namespace MQSimEngine
{
	class Sim_Reporter
	{
	public:
		virtual void Report_results_in_XML(Utils::XmlWriter& xmlwriter) = 0;
	};
}

#endif // !SIM_REPORTER_H
