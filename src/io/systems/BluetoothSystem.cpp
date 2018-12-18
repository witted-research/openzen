#include "io/systems/BluetoothSystem.h"

#include "io/bluetooth/BluetoothDeviceFinder.h"
#include "io/interfaces/BluetoothInterface.h"

namespace zen
{
    bool BluetoothSystem::available()
    {
        if (QBluetoothDeviceDiscoveryAgent::supportedDiscoveryMethods().testFlag(QBluetoothDeviceDiscoveryAgent::ClassicMethod))
            return true;

        return false;
    }

    ZenError BluetoothSystem::listDevices(std::vector<ZenSensorDesc>& outDevices)
    {
        BluetoothDeviceFinder finder;
        return finder.listDevices(outDevices);
    }

    std::unique_ptr<BaseIoInterface> BluetoothSystem::obtain(const ZenSensorDesc& desc, ZenError& outError)
    {
        auto handle = std::make_unique<BluetoothDeviceHandler>(desc.handle64);
        if (outError = handle->initialize())
            return nullptr;

        auto format = modbus::ModbusFormat::LP;
        auto factory = modbus::make_factory(format);
        auto parser = modbus::make_parser(format);
        if (!factory || !parser)
        {
            outError = ZenError_InvalidArgument;
            return nullptr;
        }

        return std::make_unique<BluetoothInterface>(std::move(handle), std::move(factory), std::move(parser));
    }
}
