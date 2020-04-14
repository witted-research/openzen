//===========================================================================//
//
// Copyright (C) 2020 LP-Research Inc.
//
// This file is part of OpenZen, under the MIT License.
// See https://bitbucket.org/lpresearch/openzen/src/master/LICENSE for details
// SPDX-License-Identifier: MIT
//
//===========================================================================//

#ifndef ZEN_IO_INTERFACES_FTDIUSBINTERFACE_H_
#define ZEN_IO_INTERFACES_FTDIUSBINTERFACE_H_

#include <atomic>
#include <string>
#include <string_view>
#include <thread>

#define NOMINMAX
#include "ftd2xx.h"
#undef NOMINMAX

#include "io/IIoInterface.h"

namespace zen
{
    class FtdiUsbInterface : public IIoInterface
    {
    public:
        FtdiUsbInterface(IIoDataSubscriber& subscriber, FT_HANDLE handle) noexcept;
        ~FtdiUsbInterface();

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

        std::atomic_bool m_terminate;
        std::thread m_pollingThread;
        std::vector<std::byte> m_buffer;

        FT_HANDLE m_handle;

        unsigned int m_baudrate;
    };
}

#endif
