#ifndef ZEN_IO_BLUETOOTH_BLUETOOTHDEVICEFINDER_H_
#define ZEN_IO_BLUETOOTH_BLUETOOTHDEVICEFINDER_H_

#include <memory>

#include <QBluetoothDeviceDiscoveryAgent>
#include <QThread>

#include "ZenTypes.h"
#include "utility/ThreadFence.h"

namespace zen
{
    class BluetoothDeviceFinder : public QThread
    {
        Q_OBJECT

    public:
        ZenError listDevices(std::vector<ZenSensorDesc>& outDevices);

    private:
        void run();

        ThreadFence m_fence;
        std::unique_ptr<QBluetoothDeviceDiscoveryAgent> m_agent;
    };
}

#endif
