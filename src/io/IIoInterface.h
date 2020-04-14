//===========================================================================//
//
// Copyright (C) 2020 LP-Research Inc.
//
// This file is part of OpenZen, under the MIT License.
// See https://bitbucket.org/lpresearch/openzen/src/master/LICENSE for details
// SPDX-License-Identifier: MIT
//
//===========================================================================//

#ifndef ZEN_IO_IIOINTERFACE_H_
#define ZEN_IO_IIOINTERFACE_H_

#include <cstdint>
#include <string_view>
#include <vector>

#include <gsl/span>
#include <nonstd/expected.hpp>

#include "ZenTypes.h"

namespace zen
{
    class IIoDataSubscriber
    {
    public:
        virtual ZenError processData(gsl::span<const std::byte> data) noexcept = 0;
    };

    class IIoInterface
    {
    public:
        IIoInterface(IIoDataSubscriber& subscriber) : m_subscriber(subscriber) {}
        virtual ~IIoInterface() = default;

        /** Send data to IO interface */
        virtual ZenError send(gsl::span<const std::byte> data) noexcept = 0;

        /** Returns the IO interface's baudrate (bit/s) */
        virtual nonstd::expected<int32_t, ZenError> baudRate() const noexcept = 0;

        /** Set Baudrate of IO interface (bit/s) */
        virtual ZenError setBaudRate(unsigned int rate) noexcept = 0;

        /** Returns the supported baudrates of the IO interface (bit/s) */
        virtual nonstd::expected<std::vector<int32_t>, ZenError> supportedBaudRates() const noexcept = 0;

        /** Returns the type of IO interface */
        virtual std::string_view type() const noexcept = 0;

        /** Returns whether the IO interface equals the sensor description */
        virtual bool equals(const ZenSensorDesc& desc) const noexcept = 0;

    protected:
        /** Publish received data to the subscriber */
        ZenError publishReceivedData(gsl::span<const std::byte> data) { return m_subscriber.processData(data); }

    private:
        IIoDataSubscriber& m_subscriber;
    };
}

#endif
