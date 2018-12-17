#include "io/interfaces/BluetoothInterface.h"

#include "io/systems/BluetoothSystem.h"

namespace zen
{
    BluetoothInterface::BluetoothInterface(std::unique_ptr<BluetoothDeviceHandler> handler)
        : m_handler(std::move(handler))
    {}

    ZenError BluetoothInterface::poll()
    {
        while (auto data = m_handler->tryToGetReceivedData())
            if (auto error = processReceivedData(data->data(), data->size()))
                return error;

        return ZenError_None;
    }

    ZenError BluetoothInterface::send(std::vector<unsigned char> frame)
    {
        if (auto error = m_handler->send(frame))
            return error;

        return ZenError_None;
    }

    ZenError BluetoothInterface::baudrate(int32_t& rate) const
    {
        // Not supported
        return ZenError_UnknownProperty;
    }

    ZenError BluetoothInterface::setBaudrate(unsigned int rate)
    {
        // Not supported
        return ZenError_UnknownProperty;
    }

    ZenError BluetoothInterface::supportedBaudrates(std::vector<int32_t>& outBaudrates) const
    {
        // Not supported
        return ZenError_UnknownProperty;
    }

    bool BluetoothInterface::equals(const ZenSensorDesc& desc) const
    {
        if (std::string_view(BluetoothSystem::KEY) != desc.ioType)
            return false;

        return m_handler->equals(desc.handle64);
    }
}
