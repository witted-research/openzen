//===========================================================================//
//
// Copyright (C) 2020 LP-Research Inc.
//
// This file is part of OpenZen, under the MIT License.
// See https://bitbucket.org/lpresearch/openzen/src/master/LICENSE for details
// SPDX-License-Identifier: MIT
//
//===========================================================================//

#include "io/interfaces/FtdiUsbInterface.h"

#include "io/systems/FtdiUsbSystem.h"

#include "utility/Finally.h"

namespace zen
{
    FtdiUsbInterface::FtdiUsbInterface(IIoDataSubscriber& subscriber, FT_HANDLE handle) noexcept
        : IIoInterface(subscriber)
        , m_terminate(false)
        , m_pollingThread(&FtdiUsbInterface::run, this)
        , m_handle(handle)
        , m_baudrate()
    {}

    FtdiUsbInterface::~FtdiUsbInterface()
    {
        FtdiUsbSystem::fnTable.close(m_handle);
    }

    ZenError FtdiUsbInterface::send(gsl::span<const std::byte> data) noexcept
    {
        DWORD nBytesWritten;
        // Need to cast do a non-const because the FT_Write interface expects a void pointer
        if (!FT_SUCCESS(FtdiUsbSystem::fnTable.write(m_handle, const_cast<std::byte*>(data.data()), static_cast<DWORD>(data.size()), &nBytesWritten)))
            return ZenError_Io_SendFailed;

        if (nBytesWritten != static_cast<DWORD>(data.size()))
            return ZenError_Io_SendFailed;

        return ZenError_None;
    }

    nonstd::expected<int32_t, ZenError> FtdiUsbInterface::baudRate() const noexcept
    {
        return m_baudrate;
    }

    ZenError FtdiUsbInterface::setBaudRate(unsigned int rate) noexcept
    {
        // [XXX] Multi-threading
        if (rate == m_baudrate)
            return ZenError_None;

        if (!FT_SUCCESS(FtdiUsbSystem::fnTable.setBaudrate(m_handle, rate)))
            return ZenError_Io_SetFailed;

        m_baudrate = rate;
        return ZenError_None;
    }

    nonstd::expected<std::vector<int32_t>, ZenError> FtdiUsbInterface::supportedBaudRates() const noexcept
    {
        std::vector<int32_t> baudRates;
        baudRates.reserve(14);

        baudRates.emplace_back(FT_BAUD_300);
        baudRates.emplace_back(FT_BAUD_600);
        baudRates.emplace_back(FT_BAUD_1200);
        baudRates.emplace_back(FT_BAUD_2400);
        baudRates.emplace_back(FT_BAUD_4800);
        baudRates.emplace_back(FT_BAUD_9600);
        baudRates.emplace_back(FT_BAUD_14400);
        baudRates.emplace_back(FT_BAUD_19200);
        baudRates.emplace_back(FT_BAUD_38400);
        baudRates.emplace_back(FT_BAUD_57600);
        baudRates.emplace_back(FT_BAUD_115200);
        baudRates.emplace_back(FT_BAUD_230400);
        baudRates.emplace_back(FT_BAUD_460800);
        baudRates.emplace_back(FT_BAUD_921600);

        return std::move(baudRates);
    }

    std::string_view FtdiUsbInterface::type() const noexcept
    {
        return FtdiUsbSystem::KEY;
    }

    bool FtdiUsbInterface::equals(const ZenSensorDesc& desc) const noexcept
    {
        if (std::string_view(FtdiUsbSystem::KEY) != desc.ioType)
            return false;

        FT_DEVICE device;
        DWORD id;
        char serialNumber[64];
        char description[64];
        if (!FT_SUCCESS(FtdiUsbSystem::fnTable.getDeviceInfo(m_handle, &device, &id, serialNumber, description, nullptr)))
            return false;

        return std::string_view(desc.serialNumber) == serialNumber;
    }

    int FtdiUsbInterface::run()
    {
        while (!m_terminate)
        {
            DWORD temp, nReceivedBytes;
            if (!FT_SUCCESS(FtdiUsbSystem::fnTable.getStatus(m_handle, &nReceivedBytes, &temp, &temp)))
                return ZenError_Io_GetFailed;

            m_buffer.resize(nReceivedBytes);
            if (nReceivedBytes > 0 && !FT_SUCCESS(FtdiUsbSystem::fnTable.read(m_handle, m_buffer.data(), static_cast<DWORD>(m_buffer.size()), &nReceivedBytes)))
                return ZenError_Io_ReadFailed;

            publishReceivedData(m_buffer);

            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }

        return ZenError_None;
    }
}
