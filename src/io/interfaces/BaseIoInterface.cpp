#include "io/interfaces/BaseIoInterface.h"

#include <iostream>
#include <string>

#include "Sensor.h"
#include "io/Modbus.h"
#include "utility/Finally.h"

namespace zen
{
    BaseIoInterface::BaseIoInterface(std::unique_ptr<modbus::IFrameFactory> factory, std::unique_ptr<modbus::IFrameParser> parser) noexcept
        : m_factory(std::move(factory))
        , m_parser(std::move(parser))
        , m_subscriber(nullptr)
    {}

    ZenError BaseIoInterface::send(uint8_t address, uint8_t function, const unsigned char* data, size_t length)
    {
        if (data == nullptr && length > 0)
            return ZenError_IsNull;

        if (length > std::numeric_limits<uint8_t>::max())
            return ZenError_Io_MsgTooBig;

        return send(m_factory->makeFrame(address, function, data, static_cast<uint8_t>(length)));
    }

    ZenError BaseIoInterface::processReceivedData(const unsigned char* data, size_t length)
    {
        const size_t bufferSize = length;
        while (length > 0)
        {
            const size_t nParsedBytes = bufferSize - length;
            if (auto error = m_parser->parse(data + nParsedBytes, length))
            {
                std::cout << "Received corrupt message:" << std::endl;
                std::cout << std::string_view(reinterpret_cast<const char*>(data), length) << std::endl;
                length -= 1;
                continue;
                // [XXX] Is this a valid approach?
                //return ZenError_Io_MsgCorrupt;
            }

            if (m_parser->finished())
            {
                auto guard = finally([this]() {
                    m_parser->reset();
                });

                const auto& frame = m_parser->frame();
                if (auto error = m_subscriber->processData(frame.address, frame.function, frame.data.data(), frame.data.size()))
                    std::cout << "Received unexpected message (" << std::to_string(frame.function) << ": " << frame.data.data() << ") from " << std::to_string(frame.address) << std::endl;
            }
        }

        return ZenError_None;
    }
}
