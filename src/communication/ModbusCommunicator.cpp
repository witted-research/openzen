#include "ModbusCommunicator.h"

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
        while (!data.empty())
        {
            if (modbus::FrameParseError_None != m_parser->parse(data))
            {
                std::cout << "Received corrupt message: ";
                for (auto c : data)
                    std::cout << std::to_integer<unsigned>(c) << ",";
                std::cout << std::endl;

                do
                {
                    m_parser->reset();
                    data = data.subspan(1);
                } while (!data.empty() && modbus::FrameParseError_None != m_parser->parse(data));
                continue;
            }

            if (m_parser->finished())
            {
                const auto& frame = m_parser->frame();
                if (auto error = m_subscriber->processReceivedData(frame.address, frame.function, frame.data))
                {
                    std::cout << "Failed to process message with address '" << std::to_string(frame.address) <<
                        "', function '" << std::to_string(frame.function) <<
                        "', data '";

                    for (auto c : frame.data)
                        std::cout << std::to_integer<unsigned>(c);
                    std::cout << "' due to error '" << error << "'." << std::endl;
                }
                m_parser->reset();
            }
        }

        return ZenError_None;
    }
}