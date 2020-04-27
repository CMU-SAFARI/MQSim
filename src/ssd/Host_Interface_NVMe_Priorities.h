#ifndef HOST_INTERFACE_NVME_PRIORITIES_H_
#define HOST_INTERFACE_NVME_PRIORITIES_H_

#include <stdexcept>

class IO_Flow_Priority_Class
{
public:
    enum Priority
    {
        URGENT = 0,
        HIGH = 1,
        MEDIUM = 2,
        LOW = 3,
        UNDEFINED = 10000
    };

    static const int NUMBER_OF_PRIORITY_LEVELS = 4;

    static int get_scheduling_weight(Priority priority)
    {
        switch (priority)
        {
        case URGENT:
            return INT32_MAX;
        case HIGH:
            return 4;
        case MEDIUM:
            return 2;
        case LOW:
            return 1;
        default:
            return 0;
        }
    }

    static std::string to_string(Priority priority)
    {
        switch (priority)
        {
        case URGENT:
            return "URGENT";
        case HIGH:
            return "HIGH";
        case MEDIUM:
            return "MEDIUM";
        case LOW:
            return "LOW";
        default:
            return "";
        }
    }

    static std::string to_string(unsigned int priority)
    {
        return to_string(static_cast<IO_Flow_Priority_Class::Priority>(priority));
    }

    static Priority to_priority(int priorityInt)
    {
        switch (priorityInt)
        {
        case 0:
            return URGENT;
        case 1:
            return HIGH;
        case 2:
            return MEDIUM;
        case 3:
            return LOW;
        default:
            throw std::invalid_argument("Priority level integer value");
        }
    }
};

#endif // !HOST_INTERFACE_NVME_PRIORITIES_H_