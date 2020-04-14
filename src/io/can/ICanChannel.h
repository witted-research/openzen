//===========================================================================//
//
// Copyright (C) 2020 LP-Research Inc.
//
// This file is part of OpenZen, under the MIT License.
// See https://bitbucket.org/lpresearch/openzen/src/master/LICENSE for details
// SPDX-License-Identifier: MIT
//
//===========================================================================//

#ifndef ZEN_IO_CAN_ICANCHANNEL_H_
#define ZEN_IO_CAN_ICANCHANNEL_H_

#include <cstdint>
#include <optional>
#include <string_view>
#include <vector>

#include <gsl/span>
#include <nonstd/expected.hpp>

#include "ZenTypes.h"
#include "io/interfaces/CanInterface.h"

namespace zen
{
    class CanManager;

    class ICanChannel
    {
    protected:
        /** Protected destructor to prevent usage of pointer to base class */
        ~ICanChannel() = default;

    public:
        friend class CanInterface;
        friend class CanManager;

        /** Subscribe IO Interface to CAN interface  */
        virtual bool subscribe(CanInterface& i) noexcept = 0;

        /** Unsubscribe IO Interface from CAN interface */
        virtual void unsubscribe(CanInterface& i) noexcept = 0;

        /** List devices connected to the CAN interface */
        virtual ZenError listDevices(std::vector<ZenSensorDesc>& outDevices) noexcept = 0;

        /** Poll data from CAN bus */
        virtual ZenError poll() noexcept = 0;

        /** Returns the channel Id */
        virtual unsigned int channel() const noexcept = 0;

        /** Returns the type of IO used by the CAN channel */
        virtual std::string_view type() const noexcept = 0;

        /** Returns whether the CAN channel equals the IO type */
        virtual bool equals(std::string_view ioType) const noexcept = 0;

    protected:
        ZenError publishReceivedData(CanInterface& canInterface, gsl::span<const std::byte> data) { return canInterface.publishReceivedData(data); }

    private:
        /** Send data to CAN bus */
        virtual ZenError send(uint32_t id, gsl::span<const std::byte> data) noexcept = 0;

        /** Returns the IO interface's baudrate (bit/s) */
        virtual unsigned baudRate() const noexcept = 0;

        /** Set Baudrate of CAN bus (bit/s) */
        virtual ZenError setBaudRate(unsigned int rate) noexcept = 0;

        /** Returns the supported baudrates of the IO interface (bit/s) */
        virtual nonstd::expected<std::vector<int32_t>, ZenError> supportedBaudRates() const noexcept = 0;
    };
}

#endif
