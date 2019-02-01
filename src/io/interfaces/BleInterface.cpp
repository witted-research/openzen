#include "io/interfaces/BleInterface.h"

#include "io/systems/BleSystem.h"

namespace zen
{
    BleInterface::BleInterface(std::unique_ptr<BleDeviceHandler> handler, std::unique_ptr<modbus::IFrameFactory> factory, std::unique_ptr<modbus::IFrameParser> parser) noexcept
        : BaseIoInterface(std::move(factory), std::move(parser))
        , m_handler(std::move(handler))
        , m_terminate(false)
        , m_pollingThread(&BleInterface::run, this)
    {}

    ZenError BleInterface::send(std::vector<unsigned char> frame)
    {
        if (auto error = m_handler->send(frame))
            return error;

        return ZenError_None;
    }

    ZenError BleInterface::baudrate(int32_t& rate) const
    {
        // Not supported
        return ZenError_UnknownProperty;
    }

    ZenError BleInterface::setBaudrate(unsigned int rate)
    {
        // Not supported
        return ZenError_UnknownProperty;
    }

    ZenError BleInterface::supportedBaudrates(std::vector<int32_t>& outBaudrates) const
    {
        // Not supported
        return ZenError_UnknownProperty;
    }

    const char* BleInterface::type() const
    {
        return BleSystem::KEY;
    }

    bool BleInterface::equals(const ZenSensorDesc& desc) const
    {
        if (std::string_view(BleSystem::KEY) != desc.ioType)
            return false;

        return m_handler->equals(desc.handle64);
    }

    int BleInterface::run()
    {
        while (!m_terminate)
        {
            while (auto data = m_handler->tryToGetReceivedData())
                if (auto error = processReceivedData(data->data(), data->size()))
                    return error;

            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }

        return ZenError_None;
    }
}
