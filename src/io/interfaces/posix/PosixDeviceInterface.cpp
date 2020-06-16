//===========================================================================//
//
// Copyright (C) 2020 LP-Research Inc.
//
// This file is part of OpenZen, under the MIT License.
// See https://bitbucket.org/lpresearch/openzen/src/master/LICENSE for details
// SPDX-License-Identifier: MIT
//
//===========================================================================//

#include "io/interfaces/posix/PosixDeviceInterface.h"

#include "utility/Finally.h"

#include <cstring>

#include <sys/errno.h>
#include <sys/ioctl.h>
#ifdef __APPLE__
#include <IOKit/serial/ioss.h>
#endif
#include <termios.h>
#include <unistd.h>

#include "spdlog/spdlog.h"

namespace zen
{
    PosixDeviceInterfaceImpl::PosixDeviceInterfaceImpl(IIoDataSubscriber& subscriber, std::string_view identifier, int fdRead, int fdWrite) noexcept
        : IIoInterface(subscriber)
        , m_identifier(identifier)
        , m_terminate(false)
        , m_pollingThread(&PosixDeviceInterfaceImpl::run, this)
        , m_fdRead(fdRead)
        , m_fdWrite(fdWrite)
    {
    }

    PosixDeviceInterfaceImpl::~PosixDeviceInterfaceImpl()
    {
        m_terminate = true;

        m_pollingThread.join();

        ::close(m_fdRead);
        ::close(m_fdWrite);
    }

    ZenError PosixDeviceInterfaceImpl::send(gsl::span<const std::byte> data) noexcept
    {
        const auto res = ::write(m_fdWrite, data.data(), data.size());
        if (res == -1)
            return ZenError_Io_SendFailed;

        if (res != data.size())
            return ZenError_Io_SendFailed;

        return ZenError_None;
    }

    bool PosixDeviceInterfaceImpl::equals(const ZenSensorDesc& desc) const noexcept
    {
        if (type() != desc.ioType)
            return false;

        if (desc.name != m_identifier)
            return false;

        return true;
    }

    int PosixDeviceInterfaceImpl::run()
    {
        std::array<std::byte, 256> buffer1, buffer2;

        aiocb readCB1 = {};
        readCB1.aio_fildes = m_fdRead;
        readCB1.aio_buf = buffer1.data();
        readCB1.aio_nbytes = buffer1.size();

        aiocb readCB2 = {};
        readCB2.aio_fildes = m_fdRead;
        readCB2.aio_buf = buffer2.data();
        readCB2.aio_nbytes = buffer2.size();

        aiocb* currentCB = &readCB1;
        aiocb* lastCB = &readCB2;

        if (m_terminate)
            return ZenError_None;

        if (::aio_read(currentCB) == -1)
            return ZenError_Io_ReadFailed;

        while (!m_terminate) {
            if (::aio_suspend(&currentCB, 1, nullptr) == -1)
                return ZenError_Io_ReadFailed;

            if (::aio_error(currentCB) != 0)
                return ZenError_Io_ReadFailed;

            const auto nBytesReceived = ::aio_return(currentCB);

            // Start next read, process data (if any) in parallel.
            std::swap(currentCB, lastCB);
            if (::aio_read(currentCB) == -1)
                return ZenError_Io_ReadFailed;

            if (nBytesReceived > 0) {
                if (auto error = publishReceivedData(gsl::make_span((std::byte *)lastCB->aio_buf, nBytesReceived)))
                    return error;
            }
        }

        // cancel and wait for outstanding read operation
        ::aio_cancel(m_fdWrite, currentCB);
        ::aio_suspend(&currentCB, 1, nullptr);

        return ZenError_None;
    }
}
