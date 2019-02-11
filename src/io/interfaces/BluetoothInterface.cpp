#include "io/interfaces/BluetoothInterface.h"

#include "io/systems/BluetoothSystem.h"

namespace zen
{
    BluetoothInterface::BluetoothInterface(std::unique_ptr<BluetoothDeviceHandler> handler, std::unique_ptr<modbus::IFrameFactory> factory, std::unique_ptr<modbus::IFrameParser> parser) noexcept
        : BaseIoInterface(std::move(factory), std::move(parser))
        , m_handler(std::move(handler))
        , m_terminate(false)
        , m_ioReader(&BluetoothInterface::run, this)
    {}

    BluetoothInterface::~BluetoothInterface()
    {
        m_terminate = true;
        m_handler.reset();
        m_ioReader.join();
    }

    ZenError BluetoothInterface::send(std::vector<unsigned char> frame)
    {
        return m_handler->send(frame);
    }

    ZenError BluetoothInterface::baudrate(int32_t&) const
    {
        // Not supported
        return ZenError_UnknownProperty;
    }

    ZenError BluetoothInterface::setBaudrate(unsigned int)
    {
        // Not supported
        return ZenError_UnknownProperty;
    }

    ZenError BluetoothInterface::supportedBaudrates(std::vector<int32_t>&) const
    {
        // Not supported
        return ZenError_UnknownProperty;
    }

    const char* BluetoothInterface::type() const
    {
        return BluetoothSystem::KEY;
    }

    bool BluetoothInterface::equals(const ZenSensorDesc& desc) const
    {
        if (std::string_view(BluetoothSystem::KEY) != desc.ioType)
            return false;

        return m_handler->equals(desc.handle64);
    }

    int BluetoothInterface::run()
    {
        while (!m_terminate)
        {
            // This is the only place where we read asynchronously, so no need to check the expected
            auto future = m_handler->readAsync();
            future->wait();

            auto buffer = future->get();
            if (buffer.has_value())
            {
                if (auto error = processReceivedData(buffer->data(), buffer->size()))
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

        return ZenError_None;
    }
}
