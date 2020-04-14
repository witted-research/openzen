//===========================================================================//
//
// Copyright (C) 2020 LP-Research Inc.
//
// This file is part of OpenZen, under the MIT License.
// See https://bitbucket.org/lpresearch/openzen/src/master/LICENSE for details
// SPDX-License-Identifier: MIT
//
//===========================================================================//

#ifndef ZEN_IO_INTERFACES_ZEROMQINTERFACE_H_
#define ZEN_IO_INTERFACES_ZEROMQINTERFACE_H_

#include <array>
#include <atomic>
#include <thread>

#include "io/IIoEventInterface.h"

#include <zmq.hpp>

namespace zen
{
    class ZeroMQInterface : public IIoEventInterface
    {
    public:
        ZeroMQInterface(IIoEventSubscriber& subscriber);
        ~ZeroMQInterface();

        bool connect(std::string const& endpoint);

        /** Returns the type of IO interface */
        std::string_view type() const noexcept override;

        /** Returns whether the IO interface equals the sensor description */
        bool equals(const ZenSensorDesc& desc) const noexcept override;

    private:
        int run();

        std::unique_ptr< zmq::socket_t> m_subscriber;
        std::unique_ptr< zmq::context_t > m_context;

        std::atomic_bool m_terminate;
        std::thread m_pollingThread;

        std::string m_endpoint;
    };
}

#endif
