//===========================================================================//
//
// Copyright (C) 2020 LP-Research Inc.
//
// This file is part of OpenZen, under the MIT License.
// See https://bitbucket.org/lpresearch/openzen/src/master/LICENSE for details
// SPDX-License-Identifier: MIT
//
//===========================================================================//

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
    class ZmqDataProcessor final : public DataProcessor {
    public:
        ZmqDataProcessor();

        bool connect(const std::string & endpoint);

        LockingQueue<ZenEvent>& getEventQueue() override;

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
        std::unique_ptr<zmq::socket_t> m_publisher;

        ManagedThread<SenderThreadParams> m_senderThread;
    };
}

#endif