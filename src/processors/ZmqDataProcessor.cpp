//===========================================================================//
//
// Copyright (C) 2020 LP-Research Inc.
//
// This file is part of OpenZen, under the MIT License.
// See https://bitbucket.org/lpresearch/openzen/src/master/LICENSE for details
// SPDX-License-Identifier: MIT
//
//===========================================================================//

#include "processors/ZmqDataProcessor.h"

namespace zen
{

ZmqDataProcessor::ZmqDataProcessor() :
    m_senderThread([](SenderThreadParams& p ) {
    auto eventResult = p.m_queue.waitToPop();

    bool terminate = !eventResult.has_value();
    if (eventResult.has_value()) {
        // check for disconnect
        terminate = terminate || eventResult->eventType == ZenEventType_SensorDisconnected;
    }

    if (terminate) {
        // in case the waitToPop returns with an emtpy optional its signaling
        // that the connected queue will be shutdown.
        spdlog::info("ZmqDataProcessor will terminate because sensor event queue terminated.");
                
        p.m_publisher->close();
        return false;
    }

    zmq::message_t message;
    bool streamable = zen::Streaming::toZmqMessage(*eventResult, message);

    if (streamable) {
        p.m_publisher->send(message, zmq::send_flags::dontwait);
    } else {
        spdlog::error("Got sensor message which is not streamable");
    }

    // continue wait for next event
    return true;
    })
{
}

bool ZmqDataProcessor::connect(const std::string & endpoint) {
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

LockingQueue<ZenEvent>& ZmqDataProcessor::getEventQueue() {
    return m_queue;
}

void ZmqDataProcessor::release() {
    ZenEvent evt;

    evt.eventType = ZenEventType_SensorDisconnected;
    m_queue.push(evt);
    // wait for the thread to terminate
    m_senderThread.stop();
}

}
