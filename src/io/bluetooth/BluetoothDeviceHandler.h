//===========================================================================//
//
// Copyright (C) 2020 LP-Research Inc.
//
// This file is part of OpenZen, under the MIT License.
// See https://bitbucket.org/lpresearch/openzen/src/master/LICENSE for details
// SPDX-License-Identifier: MIT
//
//===========================================================================//

#ifndef ZEN_IO_BLUETOOTH_BLUETOOTHDEVICEHANDLER_H_
#define ZEN_IO_BLUETOOTH_BLUETOOTHDEVICEHANDLER_H_

#include "utility/bluetooth-serial-port/BTSerialPortBinding.h"

#include "nonstd/expected.hpp"

#include "ZenTypes.h"
#include "utility/Ownership.h"
#include "utility/LockingQueue.h"
#include "utility/ThreadFence.h"

#include <gsl/span>

#include <memory>
#include <string>
#include <vector>
#include <string_view>

namespace zen
{
    class BluetoothDeviceHandler
    {
    public:
        BluetoothDeviceHandler(std::string_view address) noexcept;
        ~BluetoothDeviceHandler();

        ZenSensorInitError initialize() noexcept;

        nonstd::expected<std::vector<std::byte>, ZenError> read() noexcept;

        ZenError send(gsl::span<const std::byte> data) noexcept;

        bool equals(std::string_view address) const noexcept;

    private:
        std::string m_address;
        std::unique_ptr<BTSerialPortBinding> m_device;

        std::vector<std::byte> m_buffer;
    };
}

#endif
