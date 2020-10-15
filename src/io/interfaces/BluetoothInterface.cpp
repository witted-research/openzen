//===========================================================================//
//
// Copyright (C) 2020 LP-Research Inc.
//
// This file is part of OpenZen, under the MIT License.
// See https://bitbucket.org/lpresearch/openzen/src/master/LICENSE for details
// SPDX-License-Identifier: MIT
//
//===========================================================================//

#include "io/interfaces/BluetoothInterface.h"

#include "io/systems/BluetoothSystem.h"

namespace zen
{
    BluetoothInterface::BluetoothInterface(IIoDataSubscriber& subscriber, std::unique_ptr<BluetoothDeviceHandler> handler) noexcept
        : IIoInterface(subscriber)
        , m_terminate(false)
        , m_handler(std::move(handler))
        , m_ioReader(&BluetoothInterface::run, this)
    {}

    BluetoothInterface::~BluetoothInterface()
    {
        m_terminate = true;
        // connection needs to be closed to abort pending reads
        // are terminated so we can join the reader thread
        m_handler->close();
        m_ioReader.join();
        m_handler.reset();
    }

    ZenError BluetoothInterface::send(gsl::span<const std::byte> data) noexcept
    {
        return m_handler->send(data);
    }

    nonstd::expected<int32_t, ZenError> BluetoothInterface::baudRate() const noexcept
    {
        // Not supported
        return nonstd::make_unexpected(ZenError_UnknownProperty);
    }

    ZenError BluetoothInterface::setBaudRate(unsigned int) noexcept
    {
        // Not supported
        return ZenError_UnknownProperty;
    }

    nonstd::expected<std::vector<int32_t>, ZenError> BluetoothInterface::supportedBaudRates() const noexcept
    {
        // Not supported
        return nonstd::make_unexpected(ZenError_UnknownProperty);
    }

    std::string_view BluetoothInterface::type() const noexcept
    {
        return BluetoothSystem::KEY;
    }

    bool BluetoothInterface::equals(const ZenSensorDesc& desc) const noexcept
    {
        if (std::string_view(BluetoothSystem::KEY) != desc.ioType)
            return false;

        return m_handler->equals(desc.identifier);
    }

    int BluetoothInterface::run()
    {
        while (!m_terminate)
        {
            // This is the only place where we read asynchronously, so no need to check the expected
            auto buffer = m_handler->read();

            try
            {
                if (buffer.has_value())
                {
                    if (auto error = publishReceivedData(*buffer))
                        return error;
                }
                else
                {
                    if (buffer.error() == ZenError_Io_NotInitialized)
                    {
                        // [TODO] Try to reconnect
                    }
                    return buffer.error();
                }
            }
            catch (...)
            {
                return ZenError_Unknown;
            }
        }

        return ZenError_None;
    }
}
