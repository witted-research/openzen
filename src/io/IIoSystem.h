//===========================================================================//
//
// Copyright (C) 2020 LP-Research Inc.
//
// This file is part of OpenZen, under the MIT License.
// See https://bitbucket.org/lpresearch/openzen/src/master/LICENSE for details
// SPDX-License-Identifier: MIT
//
//===========================================================================//

#ifndef ZEN_IO_IIOSYSTEM_H_
#define ZEN_IO_IIOSYSTEM_H_

#include <memory>
#include <vector>

#include <nonstd/expected.hpp>

#include "ZenTypes.h"
#include "io/IIoInterface.h"
#include "io/IIoEventInterface.h"

namespace zen
{
    class IIoSystem
    {
    public:
        virtual ~IIoSystem() = default;

        /** Checks whether the IO system is supported on this platform. */
        virtual bool available() = 0;

        virtual bool isHighLevel() { return false; }

        /** If successful, lists descriptions of the sensors with active IO interfaces. */
        virtual ZenError listDevices(std::vector<ZenSensorDesc>& outDevices) = 0;

        /** If succesful, obtains the IO interface for the provided sensor description. Otherwise, returns an error. */
        virtual nonstd::expected<std::unique_ptr<IIoInterface>, ZenSensorInitError> obtain(const ZenSensorDesc& desc, IIoDataSubscriber& subscriber) noexcept = 0;

        /** If succesful, obtains the IO interface for the provided sensor description. Otherwise, returns an error. */
        virtual nonstd::expected<std::unique_ptr<IIoEventInterface>, ZenSensorInitError> obtainEventBased(const ZenSensorDesc&,
            IIoEventSubscriber&) noexcept {
            return nullptr;
        }

        virtual uint32_t getDefaultBaudrate() { return 0; }
    };
}

#endif
