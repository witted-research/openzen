#include "io/bluetooth/BluetoothDeviceFinder.h"

#include <cstring>

#include "io/systems/BluetoothSystem.h"

namespace zen
{
    ZenError BluetoothDeviceFinder::listDevices(std::vector<ZenSensorDesc>& outDevices)
    {
        start();

        // There is no way of detecting whether Bluetooth is turned on, so need to manually timeout
        if (!m_fence.waitFor(std::chrono::seconds(5)))
        {
            quit();
            wait();

            return ZenError_None;
        }

        if (m_agent->error() != QBluetoothDeviceDiscoveryAgent::NoError)
            return ZenError_Device_ListingFailed;

        const auto devices = m_agent->discoveredDevices();
        for (const auto& device : devices)
        {
            if (device.coreConfigurations() & QBluetoothDeviceInfo::BaseRateCoreConfiguration)
            {
                const std::string name = device.name().toStdString();
                const auto size = std::min(sizeof(ZenSensorDesc::name) - 1, name.size());

                ZenSensorDesc desc;
                std::memcpy(desc.name, name.c_str(), size);
                desc.name[size] = '\0';
                desc.serialNumber[0] = '\0';
                desc.handle64 = device.address().toUInt64();
                std::memcpy(desc.ioType, BluetoothSystem::KEY, sizeof(BluetoothSystem::KEY));

                outDevices.emplace_back(desc);
            }
        }

        return ZenError_None;
    }

    void BluetoothDeviceFinder::run()
    {
        m_agent = std::make_unique<QBluetoothDeviceDiscoveryAgent>();

        connect(m_agent.get(), QOverload<QBluetoothDeviceDiscoveryAgent::Error>::of(&QBluetoothDeviceDiscoveryAgent::error), this, &QThread::quit, Qt::DirectConnection);
        connect(m_agent.get(), &QBluetoothDeviceDiscoveryAgent::finished, this, &QThread::quit, Qt::DirectConnection);

        m_agent->start(QBluetoothDeviceDiscoveryAgent::ClassicMethod);

        // Exec, so we can process events
        exec();

        m_fence.terminate();
    }
}
