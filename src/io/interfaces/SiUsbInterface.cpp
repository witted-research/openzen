#include "io/interfaces/SiUsbInterface.h"

#include "io/systems/SiUsbSystem.h"
#include "utility/Finally.h"

namespace zen
{
    SiUsbInterface::SiUsbInterface(HANDLE handle, std::unique_ptr<modbus::IFrameFactory> factory, std::unique_ptr<modbus::IFrameParser> parser) noexcept
        : BaseIoInterface(std::move(factory), std::move(parser))
        , m_handle(handle)
        , m_baudrate(0)
    {}

    SiUsbInterface::~SiUsbInterface()
    {
        SiUsbSystem::fnTable.close(m_handle);
    }

    ZenError SiUsbInterface::poll()
    {
        bool shouldParse = true;
        while (shouldParse)
        {
            if (auto error = processReceivedData(m_buffer.data(), m_buffer.size()))
                return error;

            if (auto error = receiveInBuffer(shouldParse))
                return error;
        }

        return ZenError_None;
    }

    ZenError SiUsbInterface::send(std::vector<unsigned char> frame)
    {
        DWORD nBytesWritten;
        if (auto error = SiUsbSystem::fnTable.write(m_handle, frame.data(), static_cast<DWORD>(frame.size()), &nBytesWritten, nullptr))
            return ZenError_Io_SendFailed;

        if (nBytesWritten != frame.size())
            return ZenError_Io_SendFailed;

        return ZenError_None;
    }

    ZenError SiUsbInterface::baudrate(int32_t& rate) const
    {
        rate = m_baudrate;
        return ZenError_None;
    }

    ZenError SiUsbInterface::setBaudrate(unsigned int rate)
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

    ZenError SiUsbInterface::supportedBaudrates(std::vector<int32_t>& outBaudrates) const
    {
        return ZenError_Io_BaudratesUnknown;
    }

    bool SiUsbInterface::equals(const ZenSensorDesc& desc) const
    {
        if (std::string_view(SiUsbSystem::KEY) != desc.ioType)
            return false;

        SI_DEVICE_STRING serialNumber;
        BYTE length;
        if (auto error = SiUsbSystem::fnTable.getDeviceProductString(m_handle, &serialNumber, &length))
            return false;

        return std::string_view(serialNumber, length) == desc.serialNumber;
    }

    ZenError SiUsbInterface::receiveInBuffer(bool& received)
    {
        DWORD temp, nReceivedBytes;
        if (auto error = SiUsbSystem::fnTable.checkRxQueue(m_handle, &nReceivedBytes, &temp))
            return ZenError_Io_GetFailed;

        m_buffer.resize(nReceivedBytes);

        received = nReceivedBytes > 0;
        if (received && SiUsbSystem::fnTable.read(m_handle, m_buffer.data(), static_cast<DWORD>(m_buffer.size()), &nReceivedBytes, nullptr) != SI_SUCCESS)
        {
            m_buffer.clear();
            return ZenError_Io_ReadFailed;
        }

        return ZenError_None;
    }
}
