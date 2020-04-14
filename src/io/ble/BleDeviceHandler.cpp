//===========================================================================//
//
// Copyright (C) 2020 LP-Research Inc.
//
// This file is part of OpenZen, under the MIT License.
// See https://bitbucket.org/lpresearch/openzen/src/master/LICENSE for details
// SPDX-License-Identifier: MIT
//
//===========================================================================//

#include "io/ble/BleDeviceHandler.h"

namespace zen
{
    BleDeviceHandler::BleDeviceHandler(std::string_view address)
        : m_controller(QBluetoothAddress(QString::fromLocal8Bit(address.data(), static_cast<int>(address.size()))), this)
        , m_disconnected(false)
    {
        connect(&m_controller, &QLowEnergyController::connected, this, [this]() {
            m_controller.discoverServices();
        });

        connect(&m_controller, QOverload<QLowEnergyController::Error>::of(&QLowEnergyController::error), this, [this]() {
            if (!m_service)
            {
                m_error = ZenSensorInitError_ConnectFailed;
                m_fence.terminate();
            }
        });

        connect(&m_controller, &QLowEnergyController::disconnected, this, [this]() {
            m_disconnected = true;
        });

        connect(&m_controller, &QLowEnergyController::serviceDiscovered, this, &BleDeviceHandler::serviceDiscovered);

        connect(&m_controller, &QLowEnergyController::discoveryFinished, this, [this]() {
            if (!m_service)
            {
                m_error = ZenSensorInitError_ConnectFailed;
                m_fence.terminate();
            }
        });
    }

    BleDeviceHandler::~BleDeviceHandler()
    {
        if (m_service && m_configDescriptor.isValid() && m_configDescriptor.value() == QByteArray::fromHex("0100"))
            m_service->writeDescriptor(m_configDescriptor, QByteArray::fromHex("0000"));

        m_controller.disconnectFromDevice();
    }

    ZenSensorInitError BleDeviceHandler::initialize()
    {
        m_controller.connectToDevice();

        if (!m_fence.waitFor(std::chrono::seconds(1)))
        {
            m_controller.disconnectFromDevice();
            reset();
            return ZenSensorInitError_ConnectFailed;
        }
        auto result = m_error;
        reset();
        return result;
    }

    ZenError BleDeviceHandler::send(gsl::span<const std::byte> data)
    {
        if (!m_service)
            return ZenError_Io_NotInitialized;

        const auto nFullFrames = data.size() / 20;
        const int remainder = data.size() % 20;

        auto it = data.data();
        QByteArray newValue;
        for (auto i = 0; i < nFullFrames; ++i, it += 8)
        {
            newValue = QByteArray::fromRawData(reinterpret_cast<const char*>(it), 20);
            m_service->writeCharacteristic(m_characteristic, newValue, QLowEnergyService::WriteWithoutResponse);
        }

        if (remainder > 0)
        {
            newValue = QByteArray::fromRawData(reinterpret_cast<const char*>(it), remainder);
            m_service->writeCharacteristic(m_characteristic, newValue, QLowEnergyService::WriteWithoutResponse);
        }

        return ZenError_None;
    }

    std::optional<std::vector<std::byte>> BleDeviceHandler::tryToGetReceivedData()
    {
        return m_queue.tryToPop();
    }

    bool BleDeviceHandler::equals(std::string_view address) const
    {
        return m_controller.remoteAddress().toString().toStdString() == address;
    }

    void BleDeviceHandler::serviceDiscovered(const QBluetoothUuid& service)
    {
        if (m_service)
            return;

        if (service == QUuid(reinterpret_cast<const char*>(ZEN_SENSOR_UUID)))
        {
            m_service.reset(m_controller.createServiceObject(service, this));

            if (m_service)
            {
                connect(m_service.get(), &QLowEnergyService::stateChanged, this, &BleDeviceHandler::serviceStateChanged);
                connect(m_service.get(), &QLowEnergyService::characteristicChanged, this, &BleDeviceHandler::receiveData);
                connect(m_service.get(), &QLowEnergyService::descriptorWritten, this, &BleDeviceHandler::descriptorWritten);
                m_service->discoverDetails();
            }
        }
    }

    void BleDeviceHandler::serviceStateChanged(QLowEnergyService::ServiceState state)
    {
        switch (state)
        {
        case QLowEnergyService::ServiceDiscovered:
            m_characteristic = m_service->characteristic(QUuid(reinterpret_cast<const char*>(ZEN_IMUDATA_UUID)));
            if (m_characteristic.isValid())
            {
                // [XXX] Notifications instead of indications. Was there a reason why we were using indications?
                m_configDescriptor = m_characteristic.descriptor(QBluetoothUuid::ClientCharacteristicConfiguration);
                if (m_configDescriptor.isValid())
                {
                    m_service->writeDescriptor(m_configDescriptor, QByteArray::fromHex("0100"));
                    break;
                }
            }

            m_error = ZenSensorInitError_InvalidAddress;
            m_fence.terminate();
            break;

        default:
            break;
        }
    }

    void BleDeviceHandler::receiveData(const QLowEnergyCharacteristic& info, const QByteArray& value)
    {
        if (info.uuid() != QUuid(reinterpret_cast<const char*>(ZEN_IMUDATA_UUID)))
            return;

        std::vector<std::byte> buffer(reinterpret_cast<const std::byte*>(value.begin()), reinterpret_cast<const std::byte*>(value.end()));
        m_queue.emplace(std::move(buffer));
    }

    void BleDeviceHandler::descriptorWritten(const QLowEnergyDescriptor& info, const QByteArray& value)
    {
        if (!info.isValid())
            return;

        if (info == m_configDescriptor && value == QByteArray::fromHex("0000"))
        {
            // Disable notifications
            m_controller.disconnectFromDevice();
            m_service.reset();
        }
    }

    void BleDeviceHandler::reset()
    {
        m_error = ZenSensorInitError_None;
        m_fence.reset();
    }
}
