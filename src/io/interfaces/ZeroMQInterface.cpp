//===========================================================================//
//
// Copyright (C) 2020 LP-Research Inc.
//
// This file is part of OpenZen, under the MIT License.
// See https://bitbucket.org/lpresearch/openzen/src/master/LICENSE for details
// SPDX-License-Identifier: MIT
//
//===========================================================================//

#include "io/interfaces/ZeroMQInterface.h"
#include "io/systems/ZeroMQSystem.h"
#include "streaming/StreamingProtocol.h"

#include <spdlog/spdlog.h>

namespace zen
{
    ZeroMQInterface::ZeroMQInterface(IIoEventSubscriber& subscriber)
        : IIoEventInterface(subscriber)
    {        
    }

    ZeroMQInterface::~ZeroMQInterface()
    {
        spdlog::info("Terminating ZeroMQ interface.");
        if (m_subscriber->connected()) {
            m_subscriber->close();
            m_terminate = true;
        }
        // this will send the ETERM signal to the recv() in the worker thread
        m_context->close();
        m_context.reset();

        if (m_pollingThread.joinable()) {
            m_pollingThread.join();
        }
        spdlog::info("ZeroMQ interface terminated.");
    }

    bool ZeroMQInterface::connect(std::string const& endpoint) {
        spdlog::info("Creating ZMQ interface for endpoint {0}", endpoint);
        m_context = std::make_unique< zmq::context_t>();
        m_subscriber = std::make_unique<zmq::socket_t>(*m_context.get(), ZMQ_SUB);

        m_endpoint = endpoint;
        try {
            // next line may throw zmq::error_t if the endpoint string is not solid
            m_subscriber->connect(endpoint);
        }
        catch (zmq::error_t & err) {
            spdlog::error("Cannot connect to endpoint {0} due to error: {1}",
                endpoint, err.what());
            return false;
        }

        // subscribe to all messages
        m_subscriber->setsockopt(ZMQ_SUBSCRIBE, nullptr, 0);

        spdlog::info("Created ZMQ interface for endpoint {} done", endpoint);

        m_terminate = false;
        m_pollingThread = std::thread(&ZeroMQInterface::run, this);

        return true;
    }

    std::string_view ZeroMQInterface::type() const noexcept
    {
        return ZeroMQSystem::KEY;
    }

    bool ZeroMQInterface::equals(const ZenSensorDesc& desc) const noexcept
    {
        if (std::string_view(ZeroMQSystem::KEY) != desc.ioType)
            return false;

        return std::string(desc.name) == m_endpoint;
    }

    int ZeroMQInterface::run()
    {
      spdlog::info("Running ZMQ interface thread");
      while (!m_terminate)
        {
          zmq::message_t zmqMessage;

          try
          {
              // todo: package event in some data struct and use proper serializer
              const auto recv_result = this->m_subscriber->recv(zmqMessage, zmq::recv_flags::none);
              if (recv_result.has_value() && (*recv_result > 0)) {
                  auto unpackedMessage = zen::Streaming::fromZmqMessage(zmqMessage);
                  if (unpackedMessage.has_value()) {
                      if (!m_terminate) {
                          auto zenEvent = zen::Streaming::streamingMessageToZenEvent(*unpackedMessage);
                          if (zenEvent) {
                              publishReceivedData(*zenEvent);
                          } else {
                              spdlog::error("Cannot convert streaming message of type {0} to ZenEvent",
                                  unpackedMessage->type);
                          }
                      }
                  }
                  else {
                      spdlog::error("Cannot unpack ZeroMQ message of size {0}", zmqMessage.size());
                  }
              }
          }
          catch (const zmq::error_t& ex)
          {
              // recv() throws ETERM when the zmq context is destroyed,
              // signal to exit the worker thread
              if (ex.num() == ETERM) {
                  spdlog::info("ZeroMQ received ETERM and will exit now");
                  break;
              }
              else
              {
                  spdlog::critical("ZeroMQ cannot receive data");
              }
          }
        }
       
        return ZenError_None;
    }
}
