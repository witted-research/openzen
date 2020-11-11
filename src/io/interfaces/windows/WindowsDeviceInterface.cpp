//===========================================================================//
//
// Copyright (C) 2020 LP-Research Inc.
//
// This file is part of OpenZen, under the MIT License.
// See https://bitbucket.org/lpresearch/openzen/src/master/LICENSE for details
// SPDX-License-Identifier: MIT
//
//===========================================================================//

#include "io/interfaces/windows/WindowsDeviceInterface.h"

#include "utility/Finally.h"

#include "io/systems/windows/WindowsDeviceSystem.h"

#include <spdlog/spdlog.h>

namespace zen
{
    WindowsDeviceInterface::WindowsDeviceInterface(IIoDataSubscriber& subscriber, std::string_view identifier, HANDLE handle, OVERLAPPED ioReader, OVERLAPPED ioWriter) noexcept
        : IIoInterface(subscriber)
        , m_identifier(identifier)
        , m_handle(handle)
        , m_ioReader(ioReader)
        , m_ioWriter(ioWriter)
        , m_terminate(false)
        , m_pollingThread(&WindowsDeviceInterface::run, this)
    {}

    WindowsDeviceInterface::~WindowsDeviceInterface()
    {
        m_terminate = true;

        // Terminate wait for the interrupt
        ::CancelIoEx(m_handle, &m_ioReader);
        ::PulseEvent(m_ioReader.hEvent);

        m_pollingThread.join();

        ::CloseHandle(m_handle);
        ::CloseHandle(m_ioReader.hEvent);
        ::CloseHandle(m_ioWriter.hEvent);
    }

    ZenError WindowsDeviceInterface::send(gsl::span<const std::byte> data) noexcept
    {
        DWORD nBytesWritten;
        if (!::WriteFile(m_handle, data.data(), static_cast<DWORD>(data.size()), &nBytesWritten, &m_ioWriter))
        {
            if (::GetLastError() != ERROR_IO_PENDING)
                return ZenError_Io_SendFailed;

            if (::WaitForSingleObject(m_ioWriter.hEvent, INFINITE) != WAIT_OBJECT_0)
                return ZenError_Io_SendFailed;

            if (!::GetOverlappedResult(m_handle, &m_ioWriter, &nBytesWritten, false))
                return ZenError_Io_SendFailed;
        }

        if (nBytesWritten != static_cast<DWORD>(data.size()))
            return ZenError_Io_SendFailed;

        return ZenError_None;
    }

    nonstd::expected<int32_t, ZenError> WindowsDeviceInterface::baudRate() const noexcept
    {
        return m_baudrate;
    }

    ZenError WindowsDeviceInterface::setBaudRate(unsigned int rate) noexcept
    {
        if (m_baudrate == rate)
            return ZenError_None;

        DCB config;
        if (!::GetCommState(m_handle, &config)) {
            spdlog::error("Cannot load COM port settings");
            return ZenError_Io_GetFailed;
        }

        config.BaudRate = rate;

        if (!::SetCommState(m_handle, &config)) {
            spdlog::error("Cannot set COM port settings");
            return ZenError_Io_SetFailed;
        }

        m_baudrate = rate;
        return ZenError_None;
    }

    nonstd::expected<std::vector<int32_t>, ZenError> WindowsDeviceInterface::supportedBaudRates() const noexcept
    {
        std::vector<int32_t> baudRates;
        baudRates.reserve(14);

        baudRates.emplace_back(CBR_110);
        baudRates.emplace_back(CBR_300);
        baudRates.emplace_back(CBR_600);
        baudRates.emplace_back(CBR_1200);
        baudRates.emplace_back(CBR_2400);
        baudRates.emplace_back(CBR_4800);
        baudRates.emplace_back(CBR_9600);
        baudRates.emplace_back(CBR_14400);
        baudRates.emplace_back(CBR_19200);
        baudRates.emplace_back(CBR_38400);
        baudRates.emplace_back(CBR_57600);
        baudRates.emplace_back(CBR_115200);
        baudRates.emplace_back(CBR_128000);
        baudRates.emplace_back(CBR_256000);

        return std::move(baudRates);
    }

    std::string_view WindowsDeviceInterface::type() const noexcept
    {
        return WindowsDeviceSystem::KEY;
    }

    bool WindowsDeviceInterface::equals(const ZenSensorDesc& desc) const noexcept
    {
        if (std::string_view(WindowsDeviceSystem::KEY) != desc.ioType)
            return false;

        if (desc.name != m_identifier)
            return false;

        return true;
    }

    int WindowsDeviceInterface::run()
    {
        while (!m_terminate)
        {
            DWORD nReceivedBytes = 0;
            if (!::ReadFile(m_handle, m_buffer.data(), static_cast<DWORD>(m_buffer.size()), &nReceivedBytes, &m_ioReader))
            {
                if (::GetLastError() != ERROR_IO_PENDING)
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