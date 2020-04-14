//===========================================================================//
//
// Copyright (C) 2020 LP-Research Inc.
//
// This file is part of OpenZen, under the MIT License.
// See https://bitbucket.org/lpresearch/openzen/src/master/LICENSE for details
// SPDX-License-Identifier: MIT
//
//===========================================================================//

#ifndef ZEN_IO_INTERFACES_BLEINTERFACE_H_
#define ZEN_IO_INTERFACES_BLEINTERFACE_H_

#include <atomic>
#include <string>
#include <string_view>
#include <thread>

#include "io/IIoInterface.h"
#include "io/ble/BleDeviceHandler.h"

namespace zen
{
    class BleInterface : public IIoInterface
    {
    public:
        BleInterface(IIoDataSubscriber& subscriber, std::unique_ptr<BleDeviceHandler> handler) noexcept;
        ~BleInterface() = default;

        /** Send data to IO interface */
        ZenError send(gsl::span<const std::byte> data) noexcept override;

        /** Returns the IO interface's baudrate (bit/s) */
        nonstd::expected<int32_t, ZenError> baudRate() const noexcept override { return nonstd::make_unexpected(ZenError_UnknownProperty); }

        /** Set Baudrate of IO interface (bit/s) */
        ZenError setBaudRate(unsigned int) noexcept override { return ZenError_UnknownProperty; }

        /** Returns the supported baudrates of the IO interface (bit/s) */
        nonstd::expected<std::vector<int32_t>, ZenError> supportedBaudRates() const noexcept override { return nonstd::make_unexpected(ZenError_UnknownProperty); }

        /** Returns the type of IO interface */
        std::string_view type() const noexcept override;

        /** Returns whether the IO interface equals the sensor description */
        bool equals(const ZenSensorDesc& desc) const noexcept override;

    private:
        int run();

        std::unique_ptr<BleDeviceHandler> m_handler;
        std::atomic_bool m_terminate;
        std::thread m_pollingThread;
    };
}

#endif
