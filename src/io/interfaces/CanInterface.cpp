//===========================================================================//
//
// Copyright (C) 2020 LP-Research Inc.
//
// This file is part of OpenZen, under the MIT License.
// See https://bitbucket.org/lpresearch/openzen/src/master/LICENSE for details
// SPDX-License-Identifier: MIT
//
//===========================================================================//

#include "io/interfaces/CanInterface.h"

#include <cstring>
#include <string>
#include <vector>

#include "io/can/ICanChannel.h"

using namespace zen;

CanInterface::CanInterface(IIoDataSubscriber& subscriber, ICanChannel& channel, uint32_t id) noexcept
    : IIoInterface(subscriber)
    , m_channel(channel)
    , m_id(id)
{}

CanInterface::~CanInterface()
{
    m_channel.unsubscribe(*this);
}

nonstd::expected<int32_t, ZenError> CanInterface::baudRate() const noexcept
{
    return m_channel.baudRate();
}

ZenError CanInterface::setBaudRate(unsigned int rate) noexcept
{
    return m_channel.setBaudRate(rate);
}

nonstd::expected<std::vector<int32_t>, ZenError> CanInterface::supportedBaudRates() const noexcept
{
    return m_channel.supportedBaudRates();
}

std::string_view CanInterface::type() const noexcept
{
    return m_channel.type();
}

bool CanInterface::equals(const ZenSensorDesc& desc) const noexcept
{
    if (!m_channel.equals(desc.ioType))
        return false;

    char* end = const_cast<char*>(std::strchr(desc.identifier, '\0'));
    const auto deviceId = std::strtoul(desc.identifier, &end, 10);
    if (deviceId == std::numeric_limits<unsigned long>::max())
        return false;

    return m_id == deviceId;
}

ZenError CanInterface::send(gsl::span<const std::byte> data) noexcept
{
    return m_channel.send(m_id, data);
}
