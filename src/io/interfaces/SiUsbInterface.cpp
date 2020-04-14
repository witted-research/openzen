//===========================================================================//
//
// Copyright (C) 2020 LP-Research Inc.
//
// This file is part of OpenZen, under the MIT License.
// See https://bitbucket.org/lpresearch/openzen/src/master/LICENSE for details
// SPDX-License-Identifier: MIT
//
//===========================================================================//

#include "io/interfaces/SiUsbInterface.h"

#include "io/systems/SiUsbSystem.h"

namespace zen
{
    SiUsbInterface::SiUsbInterface(IIoDataSubscriber& subscriber, HANDLE handle, OVERLAPPED ioReader) noexcept
        : IIoInterface(subscriber)
        , m_handle(handle)
        , m_ioReader(ioReader)
        , m_terminate(false)
        , m_pollingThread(&SiUsbInterface::run, this)
        , m_baudrate(0)
    {}

    SiUsbInterface::~SiUsbInterface()
    {
        m_terminate = true;
        
        // Terminate wait for the interrupt
        SiUsbSystem::fnTable.cancelIo(m_handle);
        ::PulseEvent(m_ioReader.hEvent);

        m_pollingThread.join();

        SiUsbSystem::fnTable.close(m_handle);
        ::CloseHandle(m_ioReader.hEvent);
    }

    ZenError SiUsbInterface::send(gsl::span<const std::byte> data) noexcept
    {
        DWORD nBytesWritten = 0;
        // Need to cast do a non-const because the SI_Write interface expects a void pointer
        if (auto error = SiUsbSystem::fnTable.write(m_handle, const_cast<std::byte*>(data.data()), static_cast<DWORD>(data.size()), &nBytesWritten, nullptr))
            return ZenError_Io_SendFailed;

        if (nBytesWritten != static_cast<DWORD>(data.size()))
            return ZenError_Io_SendFailed;

        return ZenError_None;
    }

    nonstd::expected<int32_t, ZenError> SiUsbInterface::baudRate() const noexcept
    {
        return m_baudrate;
    }

    ZenError SiUsbInterface::setBaudRate(unsigned int rate) noexcept
    {
        if (rate == m_baudrate)
            return ZenError_None;

        m_baudrate = rate;

        // [XXX] Make sure it is safe to change this!!
        if (auto error = SiUsbSystem::fnTable.setBaudrate(m_handle, m_baudrate))
            return ZenError_Io_SetFailed;

        // [XXX] Is there any old data left in the receive/transmit queues?
        return ZenError_None;
    }

    nonstd::expected<std::vector<int32_t>, ZenError> SiUsbInterface::supportedBaudRates() const noexcept
    {
        return nonstd::make_unexpected(ZenError_Io_BaudratesUnknown);
    }

    std::string_view SiUsbInterface::type() const noexcept
    {
        return SiUsbSystem::KEY;
    }

    bool SiUsbInterface::equals(const ZenSensorDesc& desc) const noexcept
    {
        if (std::string_view(SiUsbSystem::KEY) != desc.ioType)
            return false;

        SI_DEVICE_STRING serialNumber;
        BYTE length;
        if (auto error = SiUsbSystem::fnTable.getDeviceProductString(m_handle, &serialNumber, &length))
            return false;

        return std::string_view(serialNumber, length) == desc.serialNumber;
    }

    int SiUsbInterface::run()
    {
        while (!m_terminate)
        {
            DWORD nReceivedBytes = 0;
            if (auto error = SiUsbSystem::fnTable.read(m_handle, m_buffer.data(), static_cast<DWORD>(m_buffer.size()), &nReceivedBytes, &m_ioReader))
            {
                if (error != SI_IO_PENDING)
                    return ZenError_Io_ReadFailed;

                if (::WaitForSingleObject(m_ioReader.hEvent, INFINITE) != WAIT_OBJECT_0)
                    return ZenError_Io_ReadFailed;

                if (!::GetOverlappedResult(m_handle, &m_ioReader, &nReceivedBytes, false))
                    return ZenError_Io_ReadFailed;
            }

            if (nReceivedBytes > 0)
                if (auto error = publishReceivedData(gsl::make_span(m_buffer.data(), nReceivedBytes)))
                    return error;
        }

        return ZenError_None;
    }
}
