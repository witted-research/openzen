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

        return std::make_unique<BluetoothInterface>(std::move(handle));
    }
}
