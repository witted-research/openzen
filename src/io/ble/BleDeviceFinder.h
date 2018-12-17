#ifndef ZEN_IO_BLE_BLEDEVICEFINDER_H_
#define ZEN_IO_BLE_BLEDEVICEFINDER_H_

#include <memory>

#include <QBluetoothDeviceDiscoveryAgent>
#include <QThread>

#include "ZenTypes.h"

namespace zen
{
    class BleDeviceFinder : public QThread
    {
        Q_OBJECT

    public:
        ZenError listDevices(std::vector<ZenSensorDesc>& outDevices);

    private:
        void run();

        std::unique_ptr<QBluetoothDeviceDiscoveryAgent> m_agent;
    };
}

#endif
