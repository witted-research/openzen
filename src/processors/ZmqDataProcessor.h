#ifndef ZEN_ZMQ_DATA_PROCESSOR_H_
#define ZEN_ZMQ_DATA_PROCESSOR_H_

#include "Sensor.h"
#include "DataProcessor.h"
#include "utility/LockingQueue.h"
#include "utility/ManagedThread.h"
#include "streaming/StreamingProtocol.h"

#include <zmq.hpp>
#include <spdlog/spdlog.h>

namespace zen
{
    /**
    */
    class ZmqDataProcessor : public DataProcessor {
    public:
        ZmqDataProcessor(std::shared_ptr<Sensor> sensor);

        bool connect(const std::string & endpoint);

        LockingQueue<ZenEvent>& getEventQueue();

        void release() override;

    private:
        /** Our own event queue where the Sensor class will send new sensor events*/
        LockingQueue<ZenEvent> m_queue;

        std::string m_endpoint;
        
        struct SenderThreadParams {
            LockingQueue<ZenEvent>& m_queue;
            std::unique_ptr<zmq::socket_t> & m_publisher;
        };

        zmq::context_t m_context;
        std::shared_ptr<Sensor> m_sensor;
        std::unique_ptr<zmq::socket_t> m_publisher;

        ManagedThread<SenderThreadParams> m_senderThread;
    };
}

#endif