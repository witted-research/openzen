#include "io/interfaces/FtdiUsbInterface.h"

#include "io/systems/FtdiUsbSystem.h"

#include "utility/Finally.h"

namespace zen
{
    FtdiUsbInterface::FtdiUsbInterface(FT_HANDLE handle)
        : m_handle(handle)
        , m_baudrate()
    {}

    FtdiUsbInterface::~FtdiUsbInterface()
    {
        FtdiUsbSystem::fnTable.close(m_handle);
    }

    ZenError FtdiUsbInterface::poll()
    {
        bool shouldParse = true;
        while (shouldParse)
        {
            if (auto error = parseBuffer())
                return error;

            if (auto error = receiveInBuffer(shouldParse))
                return error;
        }

        return ZenError_None;
    }

    ZenError FtdiUsbInterface::send(std::vector<unsigned char> frame)
    {
        DWORD nBytesWritten;
        if (auto error = FtdiUsbSystem::fnTable.write(m_handle, frame.data(), static_cast<DWORD>(frame.size()), &nBytesWritten))
            return ZenError_Io_SendFailed;

        if (nBytesWritten != frame.size())
            return ZenError_Io_SendFailed;

        return ZenError_None;
    }

    ZenError FtdiUsbInterface::baudrate(int32_t& rate) const
    {
        rate = m_baudrate;
        return ZenError_None;
    }

    ZenError FtdiUsbInterface::setBaudrate(unsigned int rate)
    {
        // [XXX] Multi-threading
        if (rate == m_baudrate)
            return ZenError_None;

        if (auto error = FtdiUsbSystem::fnTable.setBaudrate(m_handle, rate))
            return ZenError_Io_SetFailed;

        m_baudrate = rate;
        return ZenError_None;
    }

    ZenError FtdiUsbInterface::supportedBaudrates(std::vector<int32_t>& outBaudrates) const
    {
        outBaudrates.reserve(14);
        outBaudrates.emplace_back(FT_BAUD_300);
        outBaudrates.emplace_back(FT_BAUD_600);
        outBaudrates.emplace_back(FT_BAUD_1200);
        outBaudrates.emplace_back(FT_BAUD_2400);
        outBaudrates.emplace_back(FT_BAUD_4800);
        outBaudrates.emplace_back(FT_BAUD_9600);
        outBaudrates.emplace_back(FT_BAUD_14400);
        outBaudrates.emplace_back(FT_BAUD_19200);
        outBaudrates.emplace_back(FT_BAUD_38400);
        outBaudrates.emplace_back(FT_BAUD_57600);
        outBaudrates.emplace_back(FT_BAUD_115200);
        outBaudrates.emplace_back(FT_BAUD_230400);
        outBaudrates.emplace_back(FT_BAUD_460800);
        outBaudrates.emplace_back(FT_BAUD_921600);
        return ZenError_None;
    }

    bool FtdiUsbInterface::equals(const ZenSensorDesc& desc) const
    {
        if (std::string_view(FtdiUsbSystem::KEY) != desc.ioType)
            return false;

        FT_DEVICE device;
        DWORD id;
        char serialNumber[64];
        char description[64];
        if (auto error = FtdiUsbSystem::fnTable.getDeviceInfo(m_handle, &device, &id, serialNumber, description, nullptr))
            return false;

        return std::string_view(desc.serialNumber) == serialNumber;
    }

    ZenError FtdiUsbInterface::receiveInBuffer(bool& received)
    {
        DWORD temp, nReceivedBytes;
        if (auto error = FtdiUsbSystem::fnTable.getStatus(m_handle, &nReceivedBytes, &temp, &temp))
        {
            m_buffer.clear();
            return ZenError_Io_GetFailed;
        }

        m_buffer.resize(nReceivedBytes);

        received = nReceivedBytes > 0;
        if (received && FT_SUCCESS(FtdiUsbSystem::fnTable.read(m_handle, m_buffer.data(), static_cast<DWORD>(m_buffer.size()), &nReceivedBytes)))
        {
            m_buffer.clear();
            return ZenError_Io_ReadFailed;
        }

        return ZenError_None;
    }

    ZenError FtdiUsbInterface::parseBuffer()
    {
        const size_t bufferSize = m_buffer.size();
        size_t length = bufferSize;
        while (length > 0)
        {
            const size_t nParsedBytes = bufferSize - length;
            if (auto error = m_parser.parse(m_buffer.data() + nParsedBytes, length))
                return ZenError_Io_MsgCorrupt;

            if (m_parser.finished())
            {
                auto guard = finally([this]() {
                    m_parser.reset();
                });

                const auto& frame = m_parser.frame();
                if (auto error = process(frame.address, frame.function, frame.data.data(), frame.data.size()))
                {
                    // Remove used data from buffer
                    if (length == 0)
                        m_buffer.clear();
                    else
                        m_buffer.erase(m_buffer.begin(), m_buffer.end() - length);

                    return error;
                }
            }
        }

        return ZenError_None;
    }
}
