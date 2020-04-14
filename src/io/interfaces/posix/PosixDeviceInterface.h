//===========================================================================//
//
// Copyright (C) 2020 LP-Research Inc.
//
// This file is part of OpenZen, under the MIT License.
// See https://bitbucket.org/lpresearch/openzen/src/master/LICENSE for details
// SPDX-License-Identifier: MIT
//
//===========================================================================//

#ifndef ZEN_IO_INTERFACES_LINUX_LINUXDEVICEINTERFACE_H_
#define ZEN_IO_INTERFACES_LINUX_LINUXDEVICEINTERFACE_H_

#include <array>
#include <atomic>
#include <string>
#include <string_view>
#include <thread>

#include <aio.h>

#include "io/IIoInterface.h"

namespace zen
{
    /*
    The Linux device interface uses the POSIX Asynchronous IO interface
    to read from a virtual com port device. If an LPMS sensor gets connected,
    the linux cp210x will map the USB device to a file in /dev/ttyUSB
    which is opened by this sub-system.

    For regular users to be able to open this virtual file, they need to be
    a member of the tty group. Add a user with the following command:

    sudo usermod -a -G tty <user name>

    */
    class PosixDeviceInterfaceImpl : public IIoInterface
    {
    public:
        PosixDeviceInterfaceImpl(IIoDataSubscriber& subscriber, std::string_view identifier, int fdRead, int fdWrite) noexcept;
        ~PosixDeviceInterfaceImpl();

        /** Send data to IO interface */
        ZenError send(gsl::span<const std::byte> data) noexcept override;

        /** Returns whether the IO interface equals the sensor description */
        bool equals(const ZenSensorDesc& desc) const noexcept override;

    private:
        int run();

        std::string m_identifier;

        std::atomic_bool m_terminate;
        std::thread m_pollingThread;

    protected:
        int m_fdRead, m_fdWrite;
    };

    template <class TSystem>
    class PosixDeviceInterface final : public PosixDeviceInterfaceImpl {
    public:
        PosixDeviceInterface(IIoDataSubscriber& subscriber, std::string_view identifier, int fdRead, int fdWrite) noexcept
            : PosixDeviceInterfaceImpl(subscriber, identifier, fdRead, fdWrite)
            , m_baudRate(~0u)
        {}

        /** Returns the IO interface's baudrate (bit/s) */
        nonstd::expected<int32_t, ZenError> baudRate() const noexcept override
        {
            return m_baudRate;
        }

        /** Set Baudrate of IO interface (bit/s) */
        ZenError setBaudRate(unsigned int rate) noexcept override
        {
            const auto speed = TSystem::mapBaudRate(rate);
            if (m_baudRate == speed)
                return ZenError_None;

            if (ZenError error = TSystem::setBaudRateForFD(m_fdRead, speed); error != ZenError_None)
                return error;
            if (ZenError error = TSystem::setBaudRateForFD(m_fdWrite, speed); error != ZenError_None)
                return error;

            m_baudRate = speed;
            return ZenError_None;
        }

        /** Returns the supported baudrates of the IO interface (bit/s) */
        nonstd::expected<std::vector<int32_t>, ZenError> supportedBaudRates() const noexcept override
        {
            return TSystem::supportedBaudRates();
        }

        /** Returns the type of IO interface */
        std::string_view type() const noexcept override
        {
            return TSystem::KEY;
        }

    private:
        int32_t m_baudRate;
    };    
}

#endif
