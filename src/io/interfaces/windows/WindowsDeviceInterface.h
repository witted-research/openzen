//===========================================================================//
//
// Copyright (C) 2020 LP-Research Inc.
//
// This file is part of OpenZen, under the MIT License.
// See https://bitbucket.org/lpresearch/openzen/src/master/LICENSE for details
// SPDX-License-Identifier: MIT
//
//===========================================================================//

#ifndef ZEN_IO_INTERFACES_WINDOWS_WINDOWSDEVICEINTERFACE_H_
#define ZEN_IO_INTERFACES_WINDOWS_WINDOWSDEVICEINTERFACE_H_

#include <array>
#include <atomic>
#include <string>
#include <string_view>
#include <thread>

#define NOMINMAX
#include <Windows.h>
#undef NOMINMAX

#include "io/IIoInterface.h"

namespace zen
{
    class WindowsDeviceInterface : public IIoInterface
    {
    public:
        WindowsDeviceInterface(IIoDataSubscriber& subscriber, std::string_view identifier, HANDLE handle, OVERLAPPED ioReader, OVERLAPPED ioWriter) noexcept;
        ~WindowsDeviceInterface();

        /** Send data to IO interface */
        ZenError send(gsl::span<const std::byte> data) noexcept override;

        /** Returns the IO interface's baudrate (bit/s) */
        nonstd::expected<int32_t, ZenError> baudRate() const noexcept override;

        /** Set Baudrate of IO interface (bit/s) */
        ZenError setBaudRate(unsigned int rate) noexcept override;

        /** Returns the supported baudrates of the IO interface (bit/s) */
        nonstd::expected<std::vector<int32_t>, ZenError> supportedBaudRates() const noexcept override;

        /** Returns the type of IO interface */
        std::string_view type() const noexcept override;

        /** Returns whether the IO interface equals the sensor description */
        bool equals(const ZenSensorDesc& desc) const noexcept override;

    private:
        int run();

        std::array<std::byte, 256> m_buffer;

        std::string m_identifier;

        DCB m_config;
        HANDLE m_handle;
        OVERLAPPED m_ioReader;
        OVERLAPPED m_ioWriter;

        std::atomic_bool m_terminate;
        std::thread m_pollingThread;

        unsigned int m_baudrate;
    };
}

#endif
