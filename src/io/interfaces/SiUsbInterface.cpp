#include "io/interfaces/SiUsbInterface.h"

#include "io/systems/SiUsbSystem.h"

namespace zen
{
    SiUsbInterface::SiUsbInterface(HANDLE handle, OVERLAPPED ioReader, std::unique_ptr<modbus::IFrameFactory> factory, std::unique_ptr<modbus::IFrameParser> parser) noexcept
        : BaseIoInterface(std::move(factory), std::move(parser))
        , m_ioReader(ioReader)
        , m_terminate(false)
        , m_pollingThread(&SiUsbInterface::run, this)
        , m_handle(handle)
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

    ZenError SiUsbInterface::send(std::vector<unsigned char> frame)
    {
        DWORD nBytesWritten = 0;
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

    const char* SiUsbInterface::type() const
    {
        return SiUsbSystem::KEY;
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
                if (auto error = processReceivedData(m_buffer.data(), nReceivedBytes))
                    return error;
        }

        return ZenError_None;
    }
}
