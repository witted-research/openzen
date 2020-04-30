//===========================================================================//
//
// Copyright (C) 2020 LP-Research Inc.
//
// This file is part of OpenZen, under the MIT License.
// See https://bitbucket.org/lpresearch/openzen/src/master/LICENSE for details
// SPDX-License-Identifier: MIT
//
//===========================================================================//

#include "io/bluetooth/BluetoothDeviceFinder.h"

#include <algorithm>
#include <cstring>

#include "utility/bluetooth-serial-port/DeviceINQ.h"

#include "io/systems/BluetoothSystem.h"

namespace zen
{
    ZenError BluetoothDeviceFinder::listDevices(std::vector<ZenSensorDesc>& outDevices, bool applyWhitlelist)
    {
        DeviceINQ* di = DeviceINQ::Create();

        const auto devices = di->Inquire();
        for (const auto& device : devices) {
            if (applyWhitlelist
                && !inWhitelist(device.address)) {
                continue;
            }

            const auto& name = device.name;
            const auto& address = device.address;

            ZenSensorDesc desc;
            auto length = std::min(sizeof(ZenSensorDesc::name) - 1, name.size());
            std::memcpy(desc.name, name.c_str(), length);
            desc.name[length] = '\0';

            length = std::min(sizeof(ZenSensorDesc::serialNumber) - 1, static_cast<size_t>(address.size()));
            std::memcpy(desc.serialNumber, address.c_str(), length);
            desc.serialNumber[length] = '\0';

            std::memcpy(desc.ioType, BluetoothSystem::KEY, sizeof(BluetoothSystem::KEY));

            const auto identifier = address;
            std::memcpy(desc.identifier, identifier.data(), identifier.size());
            desc.identifier[identifier.size()] = '\0';

            desc.baudRate = 921600; // Not applicable.

            outDevices.emplace_back(desc);
        }

        return ZenError_None;
    }

    bool BluetoothDeviceFinder::inWhitelist(std::string const& address) const {
        std::string addr;
        std::transform(address.begin(), address.end(), std::back_inserter(addr),
            [](const char& c)->char { return (char)::toupper(c); });
        return std::any_of(m_whitelistAddresses.begin(), m_whitelistAddresses.end(),
            [&addr](auto& prefix) { return addr.find(prefix) == 0; });
    }
}
