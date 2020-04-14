//===========================================================================//
//
// Copyright (C) 2020 LP-Research Inc.
//
// This file is part of OpenZen, under the MIT License.
// See https://bitbucket.org/lpresearch/openzen/src/master/LICENSE for details
// SPDX-License-Identifier: MIT
//
//===========================================================================//

#include "io/interfaces/BleInterface.h"

#include "io/systems/BleSystem.h"

namespace zen
{
    BleInterface::BleInterface(IIoDataSubscriber& subscriber, std::unique_ptr<BleDeviceHandler> handler) noexcept
        : IIoInterface(subscriber)
        , m_handler(std::move(handler))
        , m_terminate(false)
        , m_pollingThread(&BleInterface::run, this)
    {}

    ZenError BleInterface::send(gsl::span<const std::byte> data) noexcept
    {
        if (auto error = m_handler->send(data))
            return error;

        return ZenError_None;
    }

    std::string_view BleInterface::type() const noexcept
    {
        return BleSystem::KEY;
    }

    bool BleInterface::equals(const ZenSensorDesc& desc) const noexcept
    {
        if (std::string_view(BleSystem::KEY) != desc.ioType)
            return false;

        return m_handler->equals(desc.identifier);
    }

    int BleInterface::run()
    {
        while (!m_terminate)
        {
            while (auto data = m_handler->tryToGetReceivedData())
                if (auto error = publishReceivedData(*data))
                    return error;

            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }

        return ZenError_None;
    }
}
