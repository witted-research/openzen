//===========================================================================//
//
// Copyright (C) 2020 LP-Research Inc.
//
// This file is part of OpenZen, under the MIT License.
// See https://bitbucket.org/lpresearch/openzen/src/master/LICENSE for details
// SPDX-License-Identifier: MIT
//
//===========================================================================//

#ifndef ZEN_IO_INTERFACES_CANINTERFACE_H_
#define ZEN_IO_INTERFACES_CANINTERFACE_H_

#include <cstdint>

#include "ZenTypes.h"

#include "io/IIoInterface.h"

namespace zen
{
    class ICanChannel;

    class CanInterface : public IIoInterface
    {
    public:
        friend class ICanChannel;

        CanInterface(IIoDataSubscriber& subscriber, ICanChannel& channel, uint32_t id) noexcept;
        ~CanInterface();

        /** Send data to IO interface */
        ZenError send(gsl::span<const std::byte> data) noexcept override;

        /** Returns the CAN interface's baudrate (bit/s) */
        nonstd::expected<int32_t, ZenError> baudRate() const noexcept override;

        /** Set Baudrate of CAN interface (bit/s) */
        ZenError setBaudRate(unsigned int rate) noexcept override;

        /** Returns the supported baudrates of the CAN interface (bit/s) */
        nonstd::expected<std::vector<int32_t>, ZenError> supportedBaudRates() const noexcept override;

        /** Returns the type of IO interface */
        std::string_view type() const noexcept override;

        /** Returns whether the IO interface equals the sensor description */
        bool equals(const ZenSensorDesc& desc) const noexcept override;

        /** Returns the CAN ID */
        uint32_t id() const { return m_id; }

    private:
        ICanChannel& m_channel;

        uint32_t m_id;
    };
}

#endif
