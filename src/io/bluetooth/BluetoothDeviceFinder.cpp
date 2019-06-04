#include "io/bluetooth/BluetoothDeviceFinder.h"

#include <cstring>

#include "io/systems/BluetoothSystem.h"

namespace zen
{
    ZenError BluetoothDeviceFinder::listDevices(std::vector<ZenSensorDesc>& outDevices, bool applyWhitlelist)
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
                if (applyWhitlelist && !inWhitelist(device.address().toString())) {
                    continue;
                }

                const auto name = device.name().toStdString();
                const auto address = device.address().toString().toStdString();

                ZenSensorDesc desc;
                auto length = std::min(sizeof(ZenSensorDesc::name) - 1, name.size());
                std::memcpy(desc.name, name.c_str(), length);
                desc.name[length] = '\0';

                length = std::min(sizeof(ZenSensorDesc::serialNumber) - 1, static_cast<size_t>(address.size()));
                std::memcpy(desc.serialNumber, address.c_str(), length);
                desc.serialNumber[length] = '\0';

                std::memcpy(desc.ioType, BluetoothSystem::KEY, sizeof(BluetoothSystem::KEY));

                const auto identifier = device.address().toString().toUtf8();
                std::memcpy(desc.identifier, identifier.data(), identifier.size());
                desc.identifier[identifier.size()] = '\0';

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

    bool BluetoothDeviceFinder::inWhitelist(QString const& address) const {
#ifdef Q_OS_MAC
        // MacOS does not forward the actual bluetooth address to the
        // user code, so here devices cannot be white-listed by
        // address
        return true;
#else
        for (auto const& item : m_whitelistAddresses) {
            if (address.startsWith(item, Qt::CaseInsensitive))
                return true;
        }
        return false;
#endif
    }

}
