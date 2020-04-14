//===========================================================================//
//
// Copyright (C) 2020 LP-Research Inc.
//
// This file is part of OpenZen, under the MIT License.
// See https://bitbucket.org/lpresearch/openzen/src/master/LICENSE for details
// SPDX-License-Identifier: MIT
//
//===========================================================================//

#ifndef ZEN_IO_BLUETOOTH_BLUETOOTHDEVICEFINDER_H_
#define ZEN_IO_BLUETOOTH_BLUETOOTHDEVICEFINDER_H_

#include <memory>
#include <vector>

#include <QBluetoothDeviceDiscoveryAgent>
#include <QThread>
#include <QString>

#include "ZenTypes.h"
#include "utility/ThreadFence.h"

namespace zen
{
    class BluetoothDeviceFinder : public QThread
    {
        Q_OBJECT

    public:
        ZenError listDevices(std::vector<ZenSensorDesc>& outDevices,
            bool applyWhitlelist = true);

    private:
        void run();

        /*
        Returns true for all addresses which begin an element from the
        whitelist.
        */
        bool inWhitelist(QString const& address) const;

        ThreadFence m_fence;
        std::unique_ptr<QBluetoothDeviceDiscoveryAgent> m_agent;

        // Contains the beginning of the Bluetooth addresses which
        // are white listed in the bluetooth sensor search.
        const std::vector<QString> m_whitelistAddresses = {
            // LPMS-B2 sensor
            {"00:04:3E"}
        };
    };
}

#endif
