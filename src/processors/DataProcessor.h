#ifndef ZEN_DATA_PROCESSOR_H_
#define ZEN_DATA_PROCESSOR_H_

#include "utility/LockingQueue.h"
#include "ZenTypes.h"

namespace zen
{
    /**
    DataProcessor is a base class for implementations which want to receive sensor data
    to process it inside OpenZen. Typical examples are data logging to file, network streaming
    or calibration algorithms.
    */
    class DataProcessor {
    public:

        virtual ~DataProcessor() = default;

        virtual LockingQueue<ZenEvent>& getEventQueue() = 0;

        virtual void release() = 0;

    };

}


#endif