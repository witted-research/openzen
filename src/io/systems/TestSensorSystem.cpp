//===========================================================================//
//
// Copyright (C) 2020 LP-Research Inc.
//
// This file is part of OpenZen, under the MIT License.
// See https://bitbucket.org/lpresearch/openzen/src/master/LICENSE for details
// SPDX-License-Identifier: MIT
//
//===========================================================================//

#include "io/systems/TestSensorSystem.h"
#include "io/interfaces/TestSensorInterface.h"

#include <spdlog/spdlog.h>

namespace zen
{
    namespace
    {
        nonstd::expected<std::unique_ptr<IIoEventInterface>, ZenSensorInitError> make_interface(IIoEventSubscriber& subscriber,
             std::string const& endpoint)
        {
            return std::make_unique<TestSensorInterface>(subscriber, endpoint);
        }
    }

    bool TestSensorSystem::available()
    {
        return true;
    }

    ZenError TestSensorSystem::listDevices(std::vector<ZenSensorDesc>&)
    {
        return ZenError_None;
    }

    nonstd::expected<std::unique_ptr<IIoEventInterface>, ZenSensorInitError> TestSensorSystem::obtainEventBased(const ZenSensorDesc& desc,
        IIoEventSubscriber& subscriber) noexcept
    {
        return make_interface(subscriber, desc.identifier);
    }

    nonstd::expected<std::unique_ptr<IIoInterface>, ZenSensorInitError> TestSensorSystem::obtain(const ZenSensorDesc&, IIoDataSubscriber&) noexcept {
        return nonstd::make_unexpected(ZenSensorInitError_UnsupportedFunction);
    }
}
