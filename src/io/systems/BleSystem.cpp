#include "io/systems/BleSystem.h"

#include "io/ble/BleDeviceFinder.h"
#include "io/interfaces/BleInterface.h"

namespace zen
{
    bool BleSystem::available()
    {
        return QBluetoothDeviceDiscoveryAgent::supportedDiscoveryMethods().testFlag(QBluetoothDeviceDiscoveryAgent::LowEnergyMethod);
    }

    ZenError BleSystem::listDevices(std::vector<ZenSensorDesc>& outDevices)
    {
        BleDeviceFinder finder;
        return finder.listDevices(outDevices);
    }

    nonstd::expected<std::unique_ptr<IIoInterface>, ZenSensorInitError> BleSystem::obtain(const ZenSensorDesc& desc, IIoDataSubscriber& subscriber) noexcept
    {
        auto handle = std::make_unique<BleDeviceHandler>(desc.identifier);
        if (auto error = handle->initialize())
            return nonstd::make_unexpected(error);

        return std::make_unique<BleInterface>(subscriber, std::move(handle));
    }
}
