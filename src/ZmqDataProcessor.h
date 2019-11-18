#ifndef ZEN_ZMQ_DATA_PROCESSOR_H_
#define ZEN_ZMQ_DATA_PROCESSOR_H_

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
        ZmqDataProcessor() : m_senderThread([](SenderThreadParams& p ) {

            auto eventResult = p.m_queue.waitToPop();

            if (!eventResult.has_value()) {
                // in case the waitToPop returns with an emtpy optional its signaling
                // that the connected queue will be shutdown.
                return false;
            }

            zmq::message_t message;
            zen::Streaming::toZmqMessage(*eventResult, message);

            p. m_publisher->send(message, zmq::send_flags::dontwait);

            // continue wait for next event
            return true;
            })
        {
        }

        bool connect(const std::string & endpoint) {
            m_endpoint = endpoint;
            m_publisher = std::make_unique<zmq::socket_t>(m_context, ZMQ_PUB);
            try {
                m_publisher->bind(m_endpoint);
            }
            catch (zmq::error_t & err) {
                spdlog::error("Cannot publish events on endpoint {0} because: {1}",
                    m_endpoint, err.what());
                return false;
            }

            // start polling thread
            m_senderThread.start(SenderThreadParams{ getEventQueue(), m_publisher });
            return true;
        }

        LockingQueue<ZenEvent>& getEventQueue() {
            return m_queue;
        }

    private:
        /** Our own event queue where the Sensor class will send new sensor events*/
        LockingQueue<ZenEvent> m_queue;

        std::string m_endpoint;
        
        struct SenderThreadParams {
            LockingQueue<ZenEvent>& m_queue;
            std::unique_ptr<zmq::socket_t> & m_publisher;
        };

        zmq::context_t m_context;
        std::unique_ptr<zmq::socket_t> m_publisher;
        //m_context = std::make_unique<>();


        ManagedThread<SenderThreadParams> m_senderThread;

    };
}

#endif