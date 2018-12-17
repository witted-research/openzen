#ifndef ZEN_IO_BLE_BLEDEVICEHANDLER_H_
#define ZEN_IO_BLE_BLEDEVICEHANDLER_H_

#include <atomic>
#include <optional>
#include <string>

#include <QLowEnergyController>

#include "ZenTypes.h"
#include "io/Modbus.h"
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
        BleDeviceHandler(uint64_t address);
        ~BleDeviceHandler();

        ZenError initialize();

        ZenError send(const std::vector<unsigned char>& data);
        std::optional<modbus::Frame> tryToGetFrame();

        bool equals(uint64_t handle) const;

    private slots:
        void serviceDiscovered(const QBluetoothUuid& service);
        void serviceStateChanged(QLowEnergyService::ServiceState state);

        void receiveData(const QLowEnergyCharacteristic& info, const QByteArray& value);
        void descriptorWritten(const QLowEnergyDescriptor& info, const QByteArray& value);

    private:
        void reset();

        modbus::RTUFrameParser m_parser;
        LockingQueue<modbus::Frame> m_queue;

        QLowEnergyController m_controller;
        QLowEnergyCharacteristic m_characteristic;
        QLowEnergyDescriptor m_configDescriptor;
        std::unique_ptr<QLowEnergyService> m_service;

        ThreadFence m_fence;

        std::atomic_bool m_disconnected;
        ZenError m_error;
    };
}

#endif