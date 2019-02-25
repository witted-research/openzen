#include "Sensor.h"

#include <algorithm>
#include <cstring>
#include <string>

#include "ZenProtocol.h"
#include "SensorProperties.h"
#include "communication/ConnectionNegotiator.h"
#include "components/ComponentFactoryManager.h"
#include "components/factories/ImuComponentFactory.h"
#include "io/IIoInterface.h"
#include "io/IoManager.h"
#include "properties/BaseSensorPropertiesV0.h"
#include "properties/CorePropertyRulesV1.h"
#include "properties/LegacyCoreProperties.h"
#include "utility/Finally.h"

namespace zen
{
    namespace
    {
        std::unique_ptr<ISensorProperties> make_properties(uint8_t id, unsigned int version, SyncedModbusCommunicator& communicator) noexcept
        {
            switch (version)
            {
            case 1:
                return std::make_unique<SensorProperties<CorePropertyRulesV1>>(id, communicator);

            default:
                return nullptr;
            }
        }

        ISensorProperties& getProperties(Sensor& self, const std::vector<std::unique_ptr<SensorComponent>>& components, uint8_t address) noexcept
        {
            return *(address ? components[address - 1]->properties() : self.properties());
        }

        ZenError parseError(gsl::span<const std::byte>& data) noexcept
        {
            const auto result = *reinterpret_cast<const ZenError*>(data.data());
            data = data.subspan(sizeof(ZenError));
            return result;
        }

        ZenEvent_t parseEvent(gsl::span<const std::byte>& data) noexcept
        {
            const auto result = *reinterpret_cast<const ZenEvent_t*>(data.data());
            data = data.subspan(sizeof(ZenEvent_t));
            return result;
        }

        ZenProperty_t parseProperty(gsl::span<const std::byte>& data) noexcept
        {
            const auto result = *reinterpret_cast<const ZenProperty_t*>(data.data());
            data = data.subspan(sizeof(ZenProperty_t));
            return result;
        }

        ZenEvent make_event(ZenEvent_t type, uintptr_t sensorHandle, uintptr_t componentHandle) noexcept
        {
            ZenEvent event{0};
            event.eventType = type;
            event.sensor.handle = sensorHandle;
            event.component.handle = componentHandle;
            return event;
        }

        std::unique_ptr<modbus::IFrameFactory> getFactory(uint32_t version) noexcept
        {
            if (version == 0)
                return std::make_unique<modbus::LpFrameFactory>();

            return std::make_unique<modbus::RTUFrameFactory>();
        }

        std::unique_ptr<modbus::IFrameParser> getParser(uint32_t version) noexcept
        {
            if (version == 0)
                return std::make_unique<modbus::LpFrameParser>();

            return std::make_unique<modbus::RTUFrameParser>();
        }

        std::unique_ptr<ModbusCommunicator> moveCommunicator(std::unique_ptr<ModbusCommunicator> communicator, IModbusFrameSubscriber& newSubscriber, uint32_t version)
        {
            // [LEGACY] Potentially we need to support ModbusFormat::Lp
            communicator->setSubscriber(newSubscriber);
            communicator->setFrameFactory(getFactory(version));
            communicator->setFrameParser(getParser(version));

            return std::move(communicator);
        }
    }

    static auto imuRegistry = make_registry<ImuComponentFactory>(g_zenSensorType_Imu);

    nonstd::expected<std::shared_ptr<Sensor>, ZenSensorInitError> make_sensor(SensorConfig config, std::unique_ptr<ModbusCommunicator> communicator, uintptr_t token) noexcept
    {
        auto sensor = std::make_shared<Sensor>(std::move(config), std::move(communicator), token);
        if (auto error = sensor->init())
            return nonstd::make_unexpected(error);

        return std::move(sensor);
    }

    Sensor::Sensor(SensorConfig config, std::unique_ptr<ModbusCommunicator> communicator, uintptr_t token)
        : m_config(std::move(config))
        , m_communicator(moveCommunicator(std::move(communicator), *this, m_config.version))
        , m_token(token)
        , m_updatingFirmware(false)
        , m_updatedFirmware(false)
        , m_updatingIAP(false)
        , m_updatedIAP(false)
        , m_initialized(false)
    {
        m_components.reserve(m_config.components.size());
    }

    Sensor::~Sensor()
    {
        // We need to wait for the firmware/IAP upload to stop
        if (m_uploadThread.joinable())
            m_uploadThread.join();
    }

    ZenSensorInitError Sensor::init()
    {
        auto& manager = ComponentFactoryManager::get();
        uint8_t idx = 1;
        for (const auto& config : m_config.components)
        {
            auto factory = manager.getFactory(config.id);
            if (!factory)
                return ZenSensorInitError_UnsupportedComponent;

            auto component = factory.value()->make_component(config.version, idx++, m_communicator);
            if (!component)
                return component.error();

            m_components.push_back(std::move(*component));
        }

        if (m_config.version == 0)
            m_initialized = true;

        for (auto& component : m_components)
            if (auto error = component->init())
                return error;

        // [LEGACY] Fix for sensors that did not support negotiation yet
        // [LEGACY] Swap the order of sensor-component initialization in the future
        if (m_config.version == 0)
            m_properties = std::make_unique<LegacyCoreProperties>(m_communicator, *m_components[0]->properties());
        else if (auto properties = make_properties(0, m_config.version, m_communicator))
            m_properties = std::move(properties);
        else
            return ZenSensorInitError_UnsupportedProtocol;

        return ZenSensorInitError_None;
    }

    ZenAsyncStatus Sensor::updateFirmwareAsync(gsl::span<const std::byte> buffer) noexcept
    {
        if (m_updatingFirmware.exchange(true))
        {
            if (m_updatedFirmware.exchange(false))
            {
                m_uploadThread.join();

                const bool error = m_updateFirmwareError != ZenError_None;
                m_updatingFirmware = false;
                return error ? ZenAsync_Failed : ZenAsync_Finished;
            }

            return ZenAsync_Updating;
        }

        if (m_updatingIAP)
        {
            m_updatingFirmware = false;
            return ZenAsync_ThreadBusy;
        }

        if (buffer.data() == nullptr)
        {
            m_updatingFirmware = false;
            return ZenAsync_InvalidArgument;
        }

        std::vector<std::byte> firmware(buffer.begin(), buffer.end());
        m_uploadThread = std::thread(&Sensor::upload, this, std::move(firmware));

        return ZenAsync_Updating;
    }

    ZenAsyncStatus Sensor::updateIAPAsync(gsl::span<const std::byte> buffer) noexcept
    {
        if (m_updatingIAP.exchange(true))
        {
            if (m_updatedIAP.exchange(false))
            {
                m_uploadThread.join();

                const bool error = m_updateIAPError != ZenError_None;
                m_updatingIAP = false;
                return error ? ZenAsync_Failed : ZenAsync_Finished;
            }

            return ZenAsync_Updating;
        }

        if (m_updatingFirmware)
        {
            m_updatingIAP = false;
            return ZenAsync_ThreadBusy;
        }

        if (buffer.data() == nullptr)
        {
            m_updatingFirmware = false;
            return ZenAsync_InvalidArgument;
        }

        std::vector<std::byte> iap(buffer.begin(), buffer.end());
        m_uploadThread = std::thread(&Sensor::upload, this, std::move(iap));

        return ZenAsync_Updating;
    }

    bool Sensor::equals(const ZenSensorDesc& desc) const
    {
        return m_communicator.equals(desc);
    }

    ZenError Sensor::processReceivedData(uint8_t address, uint8_t function, gsl::span<const std::byte> data) noexcept
    {
        if (m_config.version == 0)
        {
            if (auto optInternal = base::v0::internal::map(function))
            {
                switch (*optInternal)
                {
                case EDevicePropertyInternal::Ack:
                    return m_communicator.publishAck(ZenSensorProperty_Invalid, ZenError_None);

                case EDevicePropertyInternal::Nack:
                    return m_communicator.publishAck(ZenSensorProperty_Invalid, ZenError_FW_FunctionFailed);

                case EDevicePropertyInternal::Config:
                    if (data.size() != sizeof(uint32_t))
                        return ZenError_Io_MsgCorrupt;
                    return m_communicator.publishResult(function, ZenError_None, *reinterpret_cast<const uint32_t*>(data.data()));

                default:
                    return ZenError_Io_UnsupportedFunction;
                }
            }
            else
            {
                const auto property = static_cast<EDevicePropertyV0>(function);
                switch (property)
                {
                case EDevicePropertyV0::GetBatteryCharging:
                case EDevicePropertyV0::GetPing:
                    if (data.size() != sizeof(uint32_t))
                        return ZenError_Io_MsgCorrupt;
                    return m_communicator.publishResult(function, ZenError_None, *reinterpret_cast<const uint32_t*>(data.data()));

                case EDevicePropertyV0::GetBatteryLevel:
                case EDevicePropertyV0::GetBatteryVoltage:
                    if (data.size() != sizeof(float))
                        return ZenError_Io_MsgCorrupt;
                    return m_communicator.publishResult(function, ZenError_None, *reinterpret_cast<const float*>(data.data()));

                case EDevicePropertyV0::GetSerialNumber:
                case EDevicePropertyV0::GetDeviceName:
                case EDevicePropertyV0::GetFirmwareInfo:
                    return m_communicator.publishArray(function, ZenError_None, gsl::make_span(reinterpret_cast<const char*>(data.data()), data.size()));

                case EDevicePropertyV0::GetFirmwareVersion:
                    if (data.size() != sizeof(uint32_t) * 3)
                        return ZenError_Io_MsgCorrupt;
                    return m_communicator.publishArray(function, ZenError_None, gsl::make_span(reinterpret_cast<const uint32_t*>(data.data()), 3));

                case EDevicePropertyV0::GetRawSensorData:
                    if (m_initialized)
                        return m_components[0]->processEvent(make_event(ZenImuEvent_Sample, m_token, 1), data);
                    else
                        return ZenError_None;

                default:
                    return m_components[0]->processData(function, data);
                }
            }
        }
        else
        {
            if (address > m_components.size())
                return ZenError_Io_MsgCorrupt;

            switch (function)
            {
            case ZenProtocolFunction_Ack:
                return properties::publishAck(getProperties(*this, m_components, address), m_communicator, parseProperty(data), parseError(data));

            case ZenProtocolFunction_Result:
                return properties::publishResult(getProperties(*this, m_components, address), m_communicator, parseProperty(data), parseError(data), data);

            case ZenProtocolFunction_Event:
                return address ? m_components[address - 1]->processEvent(make_event(parseEvent(data), m_token, address - 1), data) : ZenError_UnsupportedEvent;

            default:
                return ZenError_Io_UnsupportedFunction;
            }
        }
    }

    void Sensor::upload(std::vector<std::byte> firmware)
    {
        constexpr uint32_t PAGE_SIZE = 255;

        auto& outError = m_updatingFirmware ? m_updateFirmwareError : m_updateIAPError;
        auto& updated = m_updatingFirmware ? m_updatedFirmware : m_updatedIAP;
        const DeviceProperty_t property = static_cast<DeviceProperty_t>(m_updatingFirmware ? EDevicePropertyInternal::UpdateFirmware : EDevicePropertyInternal::UpdateIAP);
        const uint8_t function = m_config.version == 0 ? static_cast<uint8_t>(property) : ZenProtocolFunction_Set;

        auto guard = finally([&updated]() {
            updated = true;
        });

        const uint32_t nFullPages = static_cast<uint32_t>(firmware.size() / PAGE_SIZE);
        const uint32_t remainder = firmware.size() % PAGE_SIZE;
        const uint32_t nPages = remainder > 0 ? nFullPages + 1 : nFullPages;
        if (auto error = m_communicator.sendAndWaitForAck(0, function, property, gsl::make_span(reinterpret_cast<const std::byte*>(&nPages), sizeof(nPages))))
        {
            outError = error;
            return;
        }

        for (unsigned idx = 0; idx < nPages; ++idx)
        {
            if (auto error = m_communicator.sendAndWaitForAck(0, function, property, gsl::make_span(firmware.data() + idx * PAGE_SIZE, PAGE_SIZE)))
            {
                outError = error;
                return;
            }
        }

        if (remainder > 0)
            if (auto error = m_communicator.sendAndWaitForAck(0, function, property, gsl::make_span(firmware.data() + nFullPages * PAGE_SIZE, remainder)))
                outError = error;
    }
}