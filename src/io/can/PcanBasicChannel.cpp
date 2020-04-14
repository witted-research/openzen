//===========================================================================//
//
// Copyright (C) 2020 LP-Research Inc.
//
// This file is part of OpenZen, under the MIT License.
// See https://bitbucket.org/lpresearch/openzen/src/master/LICENSE for details
// SPDX-License-Identifier: MIT
//
//===========================================================================//

#include "io/can/PcanBasicChannel.h"

#include <limits>
#include <string>

#include "communication/Modbus.h"
#include "io/can/CanManager.h"
#include "io/interfaces/CanInterface.h"
#include "io/systems/PcanBasicSystem.h"

#include "utility/Finally.h"

namespace zen
{
    namespace
    {
        TPCANMsg makeMsg(uint32_t id, unsigned char type, const std::byte* data, uint8_t length) noexcept
        {
            TPCANMsg m;
            m.ID = id;
            m.MSGTYPE = type;
            m.LEN = length;
            std::memcpy(m.DATA, data, length);
            return m;
        }

        ZenError sendMsg(TPCANHandle channel, uint32_t id, const std::byte* data, uint8_t length) noexcept
        {
            TPCANMsg m = makeMsg(id, PCAN_MESSAGE_STANDARD, data, length);
            if (auto error = PcanBasicSystem::fnTable.write(channel, &m))
                return ZenError_Io_SendFailed;

            return ZenError_None;
        }

        unsigned int mapBaudrate(unsigned int rate)
        {
            // Round up to prevent aliasing
            if (rate > 800000)
                return PCAN_BAUD_1M;
            else if (rate > 500000)
                return PCAN_BAUD_800K;
            else if (rate > 250000)
                return PCAN_BAUD_500K;
            else if (rate > 125000)
                return PCAN_BAUD_250K;
            else if (rate > 100000)
                return PCAN_BAUD_125K;
            else if (rate > 95000)
                return PCAN_BAUD_100K;
            else if (rate > 83000)
                return PCAN_BAUD_95K;
            else if (rate > 50000)
                return PCAN_BAUD_83K;
            else if (rate > 47000)
                return PCAN_BAUD_50K;
            else if (rate > 33000)
                return PCAN_BAUD_47K;
            else if (rate > 20000)
                return PCAN_BAUD_33K;
            else if (rate > 10000)
                return PCAN_BAUD_20K;
            else if (rate > 5000)
                return PCAN_BAUD_10K;
            else
                return PCAN_BAUD_5K;
        }
    }

    PcanBasicChannel::PcanBasicChannel(TPCANHandle channel)
        : m_channel(channel)
    {}

    PcanBasicChannel::~PcanBasicChannel()
    {
        CanManager::get().unregisterChannel(*this);

        PcanBasicSystem::fnTable.uninitialize(m_channel);
    }

    bool PcanBasicChannel::subscribe(CanInterface& i) noexcept
    {
        {
            // Did someone already subscribe to this ID?
            auto it = m_subscribers.find(i.id());
            if (it != m_subscribers.cend())
                return false;
        }
        {
            // Are we already subscribed to another ID?
            auto it = m_subscribers2.find(&i);
            if (it != m_subscribers2.cend())
                return false;
        }

        m_subscribers.emplace(i.id(), &i);
        m_subscribers2.emplace(&i, i.id());
        return true;
    }

    void PcanBasicChannel::unsubscribe(CanInterface& i) noexcept
    {
        auto it2 = m_subscribers2.find(&i);
        if (it2 != m_subscribers2.cend())
        {
            m_subscribers.erase(it2->second);
            m_subscribers2.erase(it2);
        }
    }

    ZenError PcanBasicChannel::listDevices(std::vector<ZenSensorDesc>& outDevices) noexcept
    {
        for (uint32_t deviceId : m_deviceIds)
        {
            const std::string identifier = std::to_string(deviceId);

            ZenSensorDesc desc;
            std::memcpy(desc.name, identifier.c_str(), identifier.size());
            desc.name[identifier.size()] = '\0';

            desc.serialNumber[0] = '\0';
            std::memcpy(desc.ioType, PcanBasicSystem::KEY, sizeof(PcanBasicSystem::KEY));

            std::memcpy(desc.identifier, identifier.c_str(), identifier.size());
            desc.identifier[identifier.size()] = '\0';

            desc.baudRate = 921600;
            outDevices.emplace_back(desc);
        }

        // As we do not garbage collect device ids, reset and collect again
        m_deviceIds.clear();

        return ZenError_None;
    }

    ZenError PcanBasicChannel::poll() noexcept
    {
        TPCANMsg m;
        TPCANTimestamp t;

        for (;;)
        {
            if (auto error = PcanBasicSystem::fnTable.read(m_channel, &m, &t))
                return error == PCAN_ERROR_QRCVEMPTY ? ZenError_None : ZenError_Io_ReadFailed;
             
            auto it = m_subscribers.find(static_cast<uint32_t>(m.ID));
            if (it == m_subscribers.cend())
            {
                m_deviceIds.emplace(m.ID);
                continue;
            }

            if (auto error = publishReceivedData(*it->second, gsl::make_span(reinterpret_cast<std::byte*>(m.DATA), static_cast<size_t>(m.LEN))))
                return error;
        }
    }

    bool PcanBasicChannel::equals(std::string_view ioType) const noexcept
    {
        return ioType == PcanBasicSystem::KEY;
    }

    ZenError PcanBasicChannel::send(uint32_t id, gsl::span<const std::byte> data) noexcept
    {
        // [XXX] Should we sleep to see if the hasBusError() dissappears?
        if (hasBusError())
            return ZenError_Can_BusError;

        const auto nFullFrames = data.size() / 8;
        const uint8_t remainder = data.size() % 8;

        auto it = data.data();
        for (auto i = 0; i < nFullFrames; ++i, it += 8)
            if (auto error = sendMsg(m_channel, id, it, 8))
                return error;

        if (remainder > 0)
            if (auto error = sendMsg(m_channel, id, it, remainder))
                return error;

        return ZenError_None;
    }

    ZenError PcanBasicChannel::setBaudRate(unsigned int rate) noexcept
    {
        // [XXX] Make sure there are no messages being send
        //if (auto error = close())
        //    return error;

        rate = mapBaudrate(rate);
        if (rate == m_baudrate)
            return ZenError_None;

        m_baudrate = rate;
        //if (auto error = open())
        //    return error;

        // [XXX] Changed order. Does it cause errors?
        if (auto error = PcanBasicSystem::fnTable.reset(m_channel))
            return ZenError_Can_ResetFailed;

        return ZenError_None;
    }

    nonstd::expected<std::vector<int32_t>, ZenError> PcanBasicChannel::supportedBaudRates() const noexcept
    {
        std::vector<int32_t> baudRates;
        baudRates.reserve(14);

        baudRates.emplace_back(5000);
        baudRates.emplace_back(10000);
        baudRates.emplace_back(20000);
        baudRates.emplace_back(33000);
        baudRates.emplace_back(47000);
        baudRates.emplace_back(50000);
        baudRates.emplace_back(83000);
        baudRates.emplace_back(95000);
        baudRates.emplace_back(100000);
        baudRates.emplace_back(125000);
        baudRates.emplace_back(250000);
        baudRates.emplace_back(500000);
        baudRates.emplace_back(800000);
        baudRates.emplace_back(1000000);
        return std::move(baudRates);
    }

    std::string_view PcanBasicChannel::type() const noexcept
    {
        return PcanBasicSystem::KEY;
    }

    bool PcanBasicChannel::hasBusError()
    {
        return PcanBasicSystem::fnTable.getStatus(m_channel) == PCAN_ERROR_ANYBUSERR;
    }
}
