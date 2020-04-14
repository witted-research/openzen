//===========================================================================//
//
// Copyright (C) 2020 LP-Research Inc.
//
// This file is part of OpenZen, under the MIT License.
// See https://bitbucket.org/lpresearch/openzen/src/master/LICENSE for details
// SPDX-License-Identifier: MIT
//
//===========================================================================//

#ifndef ZEN_IO_IIOEVENTINTERFACE_H_
#define ZEN_IO_IIOEVENTINTERFACE_H_

#include <cstdint>
#include <string_view>
#include <vector>

#include <gsl/span>
#include <nonstd/expected.hpp>

#include "ZenTypes.h"

namespace zen
{
    class IIoEventSubscriber
    {
    public:
        virtual ZenError processEvent(ZenEvent evt) noexcept = 0;
    };

    /**
    A type of sensor interface that only delivers high-level OpenZen
    events and now raw sensor data that needs parsing.
    */
    class IIoEventInterface
    {
    public:
        IIoEventInterface(IIoEventSubscriber& subscriber) : m_subscriber(subscriber) {}
        virtual ~IIoEventInterface() = default;

        /** Returns the type of IO interface */
        virtual std::string_view type() const noexcept = 0;

        /** Returns whether the IO interface equals the sensor description */
        virtual bool equals(const ZenSensorDesc& desc) const noexcept = 0;

    protected:
        /** Publish received data to the subscriber */
        virtual ZenError publishReceivedData(ZenEvent evt) { return m_subscriber.processEvent(evt); }
    private:
        IIoEventSubscriber& m_subscriber;
    };
}

#endif
