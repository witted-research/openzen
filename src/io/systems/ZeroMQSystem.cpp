//===========================================================================//
//
// Copyright (C) 2020 LP-Research Inc.
//
// This file is part of OpenZen, under the MIT License.
// See https://bitbucket.org/lpresearch/openzen/src/master/LICENSE for details
// SPDX-License-Identifier: MIT
//
//===========================================================================//

#include "io/systems/ZeroMQSystem.h"

#include "io/interfaces/ZeroMQInterface.h"

#include <spdlog/spdlog.h>

namespace zen
{
    namespace
    {
        nonstd::expected<std::unique_ptr<IIoEventInterface>, ZenSensorInitError> make_interface(IIoEventSubscriber& subscriber,
             std::string const& endpoint)
        {
            auto ioInterface = std::make_unique<ZeroMQInterface>(subscriber);
            if (!ioInterface->connect(endpoint)) {
                return nonstd::make_unexpected(ZenSensorInitError_ConnectFailed);
            }

            return ioInterface;
        }
    }

    bool ZeroMQSystem::available()
    {
        return true;
    }

    ZenError ZeroMQSystem::listDevices(std::vector<ZenSensorDesc>&)
    {
        return ZenError_None;
    }

    nonstd::expected<std::unique_ptr<IIoEventInterface>, ZenSensorInitError> ZeroMQSystem::obtainEventBased(const ZenSensorDesc& desc, 
        IIoEventSubscriber& subscriber) noexcept
    {
        return make_interface(subscriber, desc.identifier);
    }

    nonstd::expected<std::unique_ptr<IIoInterface>, ZenSensorInitError> ZeroMQSystem::obtain(const ZenSensorDesc&, IIoDataSubscriber&) noexcept {
        return nonstd::make_unexpected(ZenSensorInitError_UnsupportedFunction);
    }
}
