#include "io/interfaces/BleInterface.h"

#include "io/systems/BleSystem.h"

namespace zen
{
    BleInterface::BleInterface(std::unique_ptr<BleDeviceHandler> handler)
        : m_handler(std::move(handler))
    {}

    ZenError BleInterface::poll()
    {
        while (auto data = m_handler->tryToGetReceivedData())
            if (auto error = processReceivedData(data->data(), data->size()))
                return error;

        return ZenError_None;
    }

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

    bool BleInterface::equals(const ZenSensorDesc& desc) const
    {
        if (std::string_view(BleSystem::KEY) != desc.ioType)
            return false;

        return m_handler->equals(desc.handle64);
    }
}
