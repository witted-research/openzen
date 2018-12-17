#include "io/ble/BleDeviceFinder.h"

#include <cstring>

#include "io/systems/BleSystem.h"

namespace zen
{
    ZenError BleDeviceFinder::listDevices(std::vector<ZenSensorDesc>& outDevices)
    {
        start();
        wait();

        if (m_agent->error() != QBluetoothDeviceDiscoveryAgent::NoError)
            return ZenError_Device_ListingFailed;

        ZenSensorDesc desc;
        std::memcpy(desc.ioType, BleSystem::KEY, sizeof(BleSystem::KEY));

        const auto devices = m_agent->discoveredDevices();
        for (const auto& device : devices)
        {
            if (device.coreConfigurations() & QBluetoothDeviceInfo::LowEnergyCoreConfiguration)
            {
                const std::string name = device.name().toStdString();
                const auto size = std::min(sizeof(ZenSensorDesc::name) - 1, name.size());

                std::memcpy(desc.name, name.c_str(), size);
                desc.name[size] = '\0';
                desc.handle64 = device.address().toUInt64();

                outDevices.emplace_back(desc);
            }
        }
        return ZenError_None;
    }

    void BleDeviceFinder::run()
    {
        m_agent = std::make_unique<QBluetoothDeviceDiscoveryAgent>();
        m_agent->setLowEnergyDiscoveryTimeout(5000);

        connect(m_agent.get(), &QBluetoothDeviceDiscoveryAgent::finished, this, &QThread::quit, Qt::DirectConnection);

        m_agent->start(QBluetoothDeviceDiscoveryAgent::LowEnergyMethod);

        // Exec, so we can process events
        exec();
    }
}
