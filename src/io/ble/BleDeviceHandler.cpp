#include "io/ble/BleDeviceHandler.h"

namespace zen
{
    BleDeviceHandler::BleDeviceHandler(uint64_t address)
        : m_controller(QBluetoothAddress(address), this)
        , m_disconnected(false)
    {
        connect(&m_controller, &QLowEnergyController::connected, this, [this]() {
            m_controller.discoverServices();
        });

        connect(&m_controller, QOverload<QLowEnergyController::Error>::of(&QLowEnergyController::error), this, [this](QLowEnergyController::Error error) {
            if (error)
            {
                m_error = ZenError_Unknown;
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
                m_error = ZenError_Io_InitFailed;
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

    ZenError BleDeviceHandler::initialize()
    {
        m_controller.connectToDevice();

        if (!m_fence.waitFor(std::chrono::seconds(1)))
        {
            m_controller.disconnectFromDevice();
            reset();
            return ZenError_Io_InitFailed;
        }
        auto result = m_error;
        reset();
        return result;
    }

    ZenError BleDeviceHandler::send(const std::vector<unsigned char>& data)
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

    std::optional<modbus::Frame> BleDeviceHandler::tryToGetFrame()
    {
        return m_queue.tryToPop();
    }

    bool BleDeviceHandler::equals(uint64_t handle) const
    {
        return m_controller.remoteAddress().toUInt64() == handle;
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

            m_error = ZenError_Io_InitFailed;
            m_fence.terminate();
            break;
        }
    }

    void BleDeviceHandler::receiveData(const QLowEnergyCharacteristic& info, const QByteArray& value)
    {
        if (info.uuid() != QUuid(reinterpret_cast<const char*>(ZEN_IMUDATA_UUID)))
            return;

        size_t length = value.size();
        while (length > 0)
        {
            const size_t nParsedBytes = value.size() - length;
            if (auto error = m_parser.parse(reinterpret_cast<const unsigned char*>(value.data() + nParsedBytes), length))
            {
                m_error = ZenError_Io_MsgCorrupt;
                return;
            }

            if (m_parser.finished())
            {
                m_queue.emplace(m_parser.frame());
                m_parser.reset();
            }
        }
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
        m_error = ZenError_None;
        m_fence.reset();
    }
}
