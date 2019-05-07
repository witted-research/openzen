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

    nonstd::expected<std::unique_ptr<IIoInterface>, ZenSensorInitError> BluetoothSystem::obtain(const ZenSensorDesc& desc, IIoDataSubscriber& subscriber) noexcept
    {
        auto handle = std::make_unique<BluetoothDeviceHandler>(desc.identifier);
        if (auto error = handle->initialize())
            return nonstd::make_unexpected(error);

        return std::make_unique<BluetoothInterface>(subscriber, std::move(handle));
    }
}
