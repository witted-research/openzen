//===========================================================================//
//
// Copyright (C) 2020 LP-Research Inc.
//
// This file is part of OpenZen, under the MIT License.
// See https://bitbucket.org/lpresearch/openzen/src/master/LICENSE for details
// SPDX-License-Identifier: MIT
//
//===========================================================================//

#ifndef ZEN_IO_SYSTEMS_TESTSENSORSYSTEM_H_
#define ZEN_IO_SYSTEMS_TESTSENSORSYSTEM_H_

#include "io/IIoSystem.h"
#include "io/IIoInterface.h"
#include "io/IIoEventInterface.h"
#include <memory>

namespace zmq {
    class context_t;
}

namespace zen
{
    class TestSensorSystem : public IIoSystem
    {
    public:
        constexpr static const char KEY[] = "TestSensor";

        bool available() override;

        bool isHighLevel() override { return true; }

        // this system won't list any devices to connect to, ZenObtainSensorByName can
        // be used to use TestSensor
        ZenError listDevices(std::vector<ZenSensorDesc>& outDevices) override;

        /** If succesful, obtains the IO interface for the provided sensor description. Otherwise, returns an error. */
        nonstd::expected<std::unique_ptr<IIoInterface>, ZenSensorInitError> obtain(const ZenSensorDesc& desc, IIoDataSubscriber& subscriber) noexcept override;

        nonstd::expected<std::unique_ptr<IIoEventInterface>, ZenSensorInitError> obtainEventBased(const ZenSensorDesc& desc, 
            IIoEventSubscriber & ) noexcept override;
    };
}

#endif
