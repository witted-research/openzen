//===========================================================================//
//
// Copyright (C) 2020 LP-Research Inc.
//
// This file is part of OpenZen, under the MIT License.
// See https://bitbucket.org/lpresearch/openzen/src/master/LICENSE for details
// SPDX-License-Identifier: MIT
//
//===========================================================================//

#ifndef ZEN_IO_BLE_BLEDEVICEHANDLER_H_
#define ZEN_IO_BLE_BLEDEVICEHANDLER_H_

#include <atomic>
#include <optional>
#include <string>

#include <gsl/span>
#include <QLowEnergyController>

#include "ZenTypes.h"
#include "utility/LockingQueue.h"
#include "utility/ThreadFence.h"

namespace zen
{
    class BleDeviceHandler : public QObject
    {
        Q_OBJECT

        static constexpr const unsigned char ZEN_SENSOR_UUID[16] { 0xcc, 0x77, 0xba, 0xf1, 0x42, 0x27, 0x4d, 0x8e, 0x9b, 0x46, 0xcb, 0xe7, 0x66, 0x16, 0xd5, 0x0b };
        static constexpr const unsigned char ZEN_IMUDATA_UUID[16] { 0xc1, 0x3c, 0x35, 0x55, 0x28, 0x11, 0xe1, 0xaa, 0x76, 0x48, 0x42, 0xb0, 0x80, 0xd7, 0xad, 0xe7 };

    public:
        BleDeviceHandler(std::string_view address);
        ~BleDeviceHandler();

        ZenSensorInitError initialize();

        ZenError send(gsl::span<const std::byte> data);
        std::optional<std::vector<std::byte>> tryToGetReceivedData();

        bool equals(std::string_view address) const;

    private slots:
        void serviceDiscovered(const QBluetoothUuid& service);
        void serviceStateChanged(QLowEnergyService::ServiceState state);

        void receiveData(const QLowEnergyCharacteristic& info, const QByteArray& value);
        void descriptorWritten(const QLowEnergyDescriptor& info, const QByteArray& value);

    private:
        void reset();

        LockingQueue<std::vector<std::byte>> m_queue;

        QLowEnergyController m_controller;
        QLowEnergyCharacteristic m_characteristic;
        QLowEnergyDescriptor m_configDescriptor;
        std::unique_ptr<QLowEnergyService> m_service;

        ThreadFence m_fence;

        std::atomic_bool m_disconnected;
        ZenSensorInitError m_error;
    };
}

#endif
