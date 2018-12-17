#include "io/interfaces/windows/WindowsDeviceInterface.h"

#include "utility/Finally.h"

namespace zen
{
    namespace
    {
        constexpr unsigned int mapBaudrate(unsigned int baudrate)
        {
            if (baudrate > CBR_128000)
                return CBR_256000;
            else if (baudrate > CBR_115200)
                return CBR_128000;
            else if (baudrate > CBR_57600)
                return CBR_115200;
            else if (baudrate > CBR_38400)
                return CBR_57600;
            else if (baudrate > CBR_19200)
                return CBR_38400;
            else if (baudrate > CBR_14400)
                return CBR_19200;
            else if (baudrate > CBR_9600)
                return CBR_14400;
            else if (baudrate > CBR_4800)
                return CBR_9600;
            else if (baudrate > CBR_2400)
                return CBR_4800;
            else if (baudrate > CBR_1200)
                return CBR_2400;
            else if (baudrate > CBR_600)
                return CBR_1200;
            else if (baudrate > CBR_300)
                return CBR_600;
            else if (baudrate > CBR_110)
                return CBR_300;
            else
                return CBR_110;
        }
    }

    WindowsDeviceInterface::WindowsDeviceInterface(HANDLE handle)
        : m_handle(handle)
    {}

    WindowsDeviceInterface::~WindowsDeviceInterface()
    {
        ::CloseHandle(m_handle);
    }

    ZenError WindowsDeviceInterface::poll()
    {
        bool shouldParse = true;
        while (shouldParse)
        {
            if (auto error = processReceivedData(m_buffer.data(), m_usedBufferSize))
                return error;

            if (auto error = receiveInBuffer(shouldParse))
                return error;
        }
        
        return ZenError_None;
    }

    ZenError WindowsDeviceInterface::send(std::vector<unsigned char> frame)
    {
        DWORD nBytesWritten;
        if (!::WriteFile(m_handle, frame.data(), static_cast<DWORD>(frame.size()), &nBytesWritten, nullptr))
            return ZenError_Io_SendFailed;

        return ZenError_None;
    }

    ZenError WindowsDeviceInterface::baudrate(int32_t& rate) const
    {
        rate = m_baudrate;
        return ZenError_None;
    }

    ZenError WindowsDeviceInterface::setBaudrate(unsigned int rate)
    {
        if (m_baudrate == rate)
            return ZenError_None;

        DCB config;
        if (!::GetCommState(m_handle, &config))
            return ZenError_Io_GetFailed;

        config.BaudRate = rate;

        if (!::SetCommState(m_handle, &config))
            return ZenError_Io_SetFailed;

        m_baudrate = rate;
        return ZenError_None;
    }

    ZenError WindowsDeviceInterface::supportedBaudrates(std::vector<int32_t>& outBaudrates) const
    {
        outBaudrates.reserve(14);
        outBaudrates.emplace_back(CBR_110);
        outBaudrates.emplace_back(CBR_300);
        outBaudrates.emplace_back(CBR_600);
        outBaudrates.emplace_back(CBR_1200);
        outBaudrates.emplace_back(CBR_2400);
        outBaudrates.emplace_back(CBR_4800);
        outBaudrates.emplace_back(CBR_9600);
        outBaudrates.emplace_back(CBR_14400);
        outBaudrates.emplace_back(CBR_19200);
        outBaudrates.emplace_back(CBR_38400);
        outBaudrates.emplace_back(CBR_57600);
        outBaudrates.emplace_back(CBR_115200);
        outBaudrates.emplace_back(CBR_128000);
        outBaudrates.emplace_back(CBR_256000);
        return ZenError_None;
    }

    bool WindowsDeviceInterface::equals(const ZenSensorDesc& desc) const
    {
        // [XXX] TODO
        return false;
    }

    ZenError WindowsDeviceInterface::receiveInBuffer(bool& received)
    {
        DWORD nReceivedBytes;
        if (!::ReadFile(m_handle, m_buffer.data(), static_cast<DWORD>(m_buffer.size()), &nReceivedBytes, nullptr))
            return ZenError_Io_ReadFailed;

        m_usedBufferSize = nReceivedBytes;
        received = nReceivedBytes > 0;
        return ZenError_None;
    }
}