//===========================================================================//
//
// Copyright (C) 2020 LP-Research Inc.
//
// This file is part of OpenZen, under the MIT License.
// See https://bitbucket.org/lpresearch/openzen/src/master/LICENSE for details
// SPDX-License-Identifier: MIT
//
//===========================================================================//

#include "ModbusCommunicator.h"

#include "utility/Finally.h"
#include "utility/StringView.h"

#include <spdlog/spdlog.h>
#include <iostream>
#include <string>

namespace zen
{
    ModbusCommunicator::ModbusCommunicator(IModbusFrameSubscriber& subscriber, std::unique_ptr<modbus::IFrameFactory> factory, std::unique_ptr<modbus::IFrameParser> parser) noexcept
        : m_subscriber(&subscriber)
        , m_factory(std::move(factory))
        , m_parser(std::move(parser))
    {}

    void ModbusCommunicator::init(std::unique_ptr<IIoInterface> ioInterface) noexcept
    {
        m_ioInterface = std::move(ioInterface);
    }

    ZenError ModbusCommunicator::send(uint8_t address, uint8_t function, gsl::span<const std::byte> data) noexcept
    {
        SPDLOG_DEBUG("sending address: {0} function: {1} data size: {2} data: {3}",
            address, function, data.size(), util::spanToString(data));
        if (!data.empty() && data.data() == nullptr)
            return ZenError_IsNull;

        // [TODO] Split up into smaller messages
        if (data.size() > std::numeric_limits<uint8_t>::max())
            return ZenError_Io_MsgTooBig;

        const auto frame = m_factory->makeFrame(address, function, data.data(), static_cast<uint8_t>(data.size()));
        return m_ioInterface->send(frame);
    }

    ZenError ModbusCommunicator::processData(gsl::span<const std::byte> data) noexcept
    {
        // enable this for low-level communication debugging
        SPDLOG_DEBUG("received data of size: {0}", data.size());

        while (!data.empty())
        {
            while (m_parserBusy.test_and_set(std::memory_order_acquire)) { /*spin lock*/ }
            auto guard = finally([this]() {
                m_parserBusy.clear(std::memory_order_release);
            });

            if (modbus::FrameParseError_None != m_parser->parse(data))
            {
                SPDLOG_DEBUG("Parsing of packet failed, can happen when OpenZen started to parse in the middle of a package.");
                // drop first byte and look for new start character
                m_parser->reset();
                data = data.subspan(1);

                continue;
            }

            if (m_parser->finished())
            {
                const auto& frame = m_parser->frame();

                SPDLOG_DEBUG("Received and parsed message with address {} function {} and data size {}",
                    std::to_string(frame.address), std::to_string(frame.function), frame.data.size());

                if (m_subscriber->processReceivedData(frame.address, frame.function, frame.data))
                {
                    spdlog::error("Failed to process message with address {} function {} data {}",
                        std::to_string(frame.address), std::to_string(frame.function), util::spanToString(frame.data));
                }
                m_parser->reset();
            }
        }

        return ZenError_None;
    }
}