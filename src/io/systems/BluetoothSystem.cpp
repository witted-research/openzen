//===========================================================================//
//
// Copyright (C) 2020 LP-Research Inc.
//
// This file is part of OpenZen, under the MIT License.
// See https://bitbucket.org/lpresearch/openzen/src/master/LICENSE for details
// SPDX-License-Identifier: MIT
//
//===========================================================================//

#include "io/systems/BluetoothSystem.h"

#include <spdlog/spdlog.h>

#include "io/bluetooth/BluetoothDeviceFinder.h"
#include "io/interfaces/BluetoothInterface.h"

namespace zen
{
    bool BluetoothSystem::available()
    {
        return true;
    }

    ZenError BluetoothSystem::listDevices(std::vector<ZenSensorDesc>& outDevices)
    {
        spdlog::info("Starting listing of Bluetooth devices");
        BluetoothDeviceFinder finder;
        return finder.listDevices(outDevices);
    }

    nonstd::expected<std::unique_ptr<IIoInterface>, ZenSensorInitError> BluetoothSystem::obtain(const ZenSensorDesc& desc, IIoDataSubscriber& subscriber) noexcept
    {
        auto handle = std::make_unique<BluetoothDeviceHandler>(desc.identifier);
        if (auto error = handle->initialize()) {
            spdlog::error("Cannot initialize Blueooth device handler");
            return nonstd::make_unexpected(error);
        }

        return std::make_unique<BluetoothInterface>(subscriber, std::move(handle));
    }
}
