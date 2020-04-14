//===========================================================================//
//
// Copyright (C) 2020 LP-Research Inc.
//
// This file is part of OpenZen, under the MIT License.
// See https://bitbucket.org/lpresearch/openzen/src/master/LICENSE for details
// SPDX-License-Identifier: MIT
//
//===========================================================================//

#ifndef ZEN_IO_CAN_PCANBASICCHANNEL_H_
#define ZEN_IO_CAN_PCANBASICCHANNEL_H_

#include <set>
#include <unordered_map>

#define NOMINMAX
#include <windows.h>
#undef NOMINMAX

#include <PCANBasic.h>

#include "io/can/ICanChannel.h"

namespace zen
{
    class PcanBasicChannel : public ICanChannel
    {
    public:
        PcanBasicChannel(TPCANHandle channel);
        ~PcanBasicChannel();

        /** Subscribe IO Interface to CAN interface  */
        bool subscribe(CanInterface& i) noexcept override;

        /** Unsubscribe IO Interface from CAN interface */
        void unsubscribe(CanInterface& i) noexcept override;

        /** List devices connected to the CAN interface */
        ZenError listDevices(std::vector<ZenSensorDesc>& outDevices) noexcept;

        /** Poll data from CAN bus */
        ZenError poll() noexcept override;

        /** Returns the channel Id */
        unsigned int channel() const noexcept override { return m_channel; }

        /** Returns the type of IO interface */
        std::string_view type() const noexcept override;

        /** Returns whether the CAN channel equals the IO type */
        bool equals(std::string_view ioType) const noexcept override;

    private:
        /** Send data to CAN bus */
        ZenError send(uint32_t id, gsl::span<const std::byte> data) noexcept override;

        /** Returns the CAN bus' baudrate (bit/s) */
        unsigned baudRate() const noexcept { return m_baudrate; }

        /** Set Baudrate of CAN bus (bit/s) */
        ZenError setBaudRate(unsigned int rate) noexcept override;

        /** Returns the supported baudrates of the CAN bus (bit/s) */
        nonstd::expected<std::vector<int32_t>, ZenError> supportedBaudRates() const noexcept override;

        bool hasBusError();

        std::set<uint32_t> m_deviceIds;

        // Simple bi-directional map of pointers
        std::unordered_map<uint32_t, CanInterface*> m_subscribers;
        std::unordered_map<CanInterface*, uint32_t> m_subscribers2;

        TPCANHandle m_channel;
        unsigned int m_baudrate;
    };
}

#endif
