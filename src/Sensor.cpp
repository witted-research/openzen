//===========================================================================//
//
// Copyright (C) 2020 LP-Research Inc.
//
// This file is part of OpenZen, under the MIT License.
// See https://bitbucket.org/lpresearch/openzen/src/master/LICENSE for details
// SPDX-License-Identifier: MIT
//
//===========================================================================//

#include "Sensor.h"

#include <algorithm>
#include <cstring>
#include <string>

#include <spdlog/spdlog.h>

#include "ZenProtocol.h"
#include "SensorClient.h"
#include "SensorManager.h"
#include "SensorProperties.h"
#include "communication/ConnectionNegotiator.h"
#include "components/ComponentFactoryManager.h"
#include "components/factories/ImuComponentFactory.h"
#include "components/factories/GnssComponentFactory.h"
#include "io/IIoInterface.h"
#include "io/IoManager.h"
#include "properties/BaseSensorPropertiesV0.h"
#include "properties/BaseSensorPropertiesV1.h"
#include "properties/CorePropertyRulesV1.h"
#include "properties/LegacyCoreProperties.h"
#include "properties/Ig1CoreProperties.h"
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
            case 2:
                return std::make_unique<SensorProperties<CorePropertyRulesV1>>(id, communicator);

            default:
                return nullptr;
            }
        }

        std::unique_ptr<modbus::IFrameFactory> getFactory(uint32_t version) noexcept
        {
            if (version == 0)
                return std::make_unique<modbus::LpFrameFactory>();
            else if (version == 1)
                return std::make_unique<modbus::LpFrameFactory>();

            return std::make_unique<modbus::RTUFrameFactory>();
        }

        std::unique_ptr<modbus::IFrameParser> getParser(uint32_t version) noexcept
        {
            if (version == 0)
                return std::make_unique<modbus::LpFrameParser>();
            if (version == 1)
                return std::make_unique<modbus::LpFrameParser>();

            return std::make_unique<modbus::RTUFrameParser>();
        }

        std::unique_ptr<ModbusCommunicator> moveCommunicator(std::unique_ptr<ModbusCommunicator> communicator, IModbusFrameSubscriber& newSubscriber, uint32_t version)
        {
            // [LEGACY] Potentially we need to support ModbusFormat::Lp
            communicator->setSubscriber(newSubscriber);
            communicator->setFrameFactory(getFactory(version));
            communicator->setFrameParser(getParser(version));

            return communicator;
        }
    }

    static auto imuRegistry = make_registry<ImuComponentFactory>(g_zenSensorType_Imu);
    static auto gnssRegistry = make_registry<GnssComponentFactory>(g_zenSensorType_Gnss);

    nonstd::expected<std::shared_ptr<Sensor>, ZenSensorInitError> make_sensor(SensorConfig config, std::unique_ptr<ModbusCommunicator> communicator, uintptr_t token) noexcept
    {
        auto sensor = std::make_shared<Sensor>(std::move(config), std::move(communicator), token);
        if (auto error = sensor->init())
            return nonstd::make_unexpected(error);

        return sensor;
    }

    nonstd::expected<std::shared_ptr<Sensor>, ZenSensorInitError> make_high_level_sensor(SensorConfig config,
        std::unique_ptr<EventCommunicator> evCom,
        uintptr_t token) noexcept
    {
        auto sensor = std::make_shared<Sensor>(std::move(config), std::move(evCom), token);
        if (auto error = sensor->init())
            return nonstd::make_unexpected(error);

        return sensor;
    }

    Sensor::Sensor(SensorConfig config, std::unique_ptr<ModbusCommunicator> communicator, uintptr_t token)
        : m_config(std::move(config))
        , m_token(token)
        , m_initialized(false)
        , m_communicator(moveCommunicator(std::move(communicator), *this, m_config.version))
        , m_updatingFirmware(false)
        , m_updatedFirmware(false)
        , m_updatingIAP(false)
        , m_updatedIAP(false)
    {
        m_components.reserve(m_config.components.size());
    }

    Sensor::Sensor(SensorConfig config, std::unique_ptr<EventCommunicator> eventCommunicator,
        uintptr_t token) : m_config(std::move(config))
        , m_token(token)
        , m_initialized(false)
        , m_eventCommunicator(std::move(eventCommunicator))
        , m_updatingFirmware(false)
        , m_updatedFirmware(false)
        , m_updatingIAP(false)
        , m_updatedIAP(false) {

        m_eventCommunicator->setSubscriber(*this);
    }

    Sensor::~Sensor()
    {
        // First we need to wait for the firmware/IAP upload to stop
        if (m_uploadThread.joinable())
            m_uploadThread.join();

        // closing all sensor components, maybe some components want to
        // download or store configuration before the sensor is closed.
        for (auto & component : m_components) {
            component->close();
        }

        // Then the communicator needs to be destroyed before to guarantee that the
        // underlying IO interface does not call processReceivedData anymore
        if (m_communicator) {
            m_communicator->close();
        }
        if (m_eventCommunicator) {
            m_eventCommunicator->close();
            m_eventCommunicator = nullptr;
        }

        // After that we can guarantee to subscribers that the sensor has shut down
        ZenEventData eventData{};
        eventData.sensorDisconnected.error = ZenError_None;
        ZenEvent disconnected{ ZenEventType_SensorDisconnected, {m_token}, {0}, eventData };

        for (auto subscriber : m_subscribers)
            subscriber.get().push(disconnected);
    }

    void Sensor::addProcessor(std::unique_ptr<DataProcessor> processor) noexcept {
        // subscribe to this sensors event queue
        subscribe(processor->getEventQueue());

        m_processors.emplace_back(std::move(processor));
    }

    void Sensor::releaseProcessors() noexcept {
        // unsubscribe & destroy all processors
        for (auto & proc : m_processors) {
            unsubscribe(proc->getEventQueue());
            proc->release();
        }
        m_processors.clear();
    }

    ZenSensorInitError Sensor::init()
    {
        if (m_communicator) {
            auto& manager = ComponentFactoryManager::get();
            uint8_t idx = 1;
            for (const auto& config : m_config.components)
            {
                SPDLOG_DEBUG("Creating component object for component {0} and version {1}", config.id, config.version);
                auto factory = manager.getFactory(config.id);
                if (!factory) {
                    spdlog::error("Cannot find factory for component {0}", config.id);
                    return ZenSensorInitError_UnsupportedComponent;
                }

                auto component = factory.value()->make_component(config.version, config.specialOptions, idx++, *m_communicator);
                if (!component) {
                    spdlog::error("Cannot create object for component {0} and version {1}", config.id, config.version);
                    return component.error();
                }
                SPDLOG_DEBUG("Created component object for component {0} and version {1}", config.id, config.version);
                m_components.push_back(std::move(*component));
            }

            if ((m_config.version == 0) || (m_config.version == 1)) {
                m_initialized = true;
            }

            for (auto& component : m_components)
                if (auto error = component->init())
                    return error;

            SPDLOG_DEBUG("Components created and initialized");

            // [LEGACY] Fix for sensors that did not support negotiation yet
            // [LEGACY] Swap the order of sensor-component initialization in the future
            if (m_config.version == 0)
                m_properties = std::make_unique<LegacyCoreProperties>(*m_communicator, *m_components[0]->properties());
            else if (m_config.version == 1)
                m_properties = std::make_unique<Ig1CoreProperties>(*m_communicator, *m_components[0]->properties());
            else if (auto properties = make_properties(0, m_config.version, *m_communicator))
                m_properties = std::move(properties);
            else
                return ZenSensorInitError_UnsupportedProtocol;

            SPDLOG_DEBUG("Sensor properties initialized");
        }
        else {
            // high-level sensor dont need initialization
            m_initialized = true;
        }

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
        if (m_communicator) {
            return m_communicator->equals(desc);
        }
        else if (m_eventCommunicator) {
            return m_eventCommunicator->equals(desc);
        }
        else {
            return false;
        }
    }

    bool Sensor::subscribe(LockingQueue<ZenEvent>& queue) noexcept
    {
        std::lock_guard<std::mutex> lock(m_subscribersMutex);
        const auto inserted = m_subscribers.insert(queue);
        return inserted.second;
    }

    void Sensor::unsubscribe(LockingQueue<ZenEvent>& queue) noexcept
    {
        std::lock_guard<std::mutex> lock(m_subscribersMutex);
        m_subscribers.erase(queue);

        if (m_subscribers.empty())
            SensorManager::get().release({ m_token });
    }

    ZenError Sensor::processReceivedData(uint8_t, uint8_t function, gsl::span<const std::byte> data) noexcept
    {
        if (m_config.version == 0)
        {
            if (auto optInternal = base::v0::internal::map(function))
            {
                switch (*optInternal)
                {
                case EDevicePropertyInternal::Ack:
                    return m_communicator->publishAck(ZenSensorProperty_Invalid, ZenError_None);

                case EDevicePropertyInternal::Nack:
                    return m_communicator->publishAck(ZenSensorProperty_Invalid, ZenError_FW_FunctionFailed);

                // this entry is used to forward the OutputDataBitset for IMU and GPS while
                // the component is not created yet.
                case EDevicePropertyInternal::ConfigImuOutputDataBitset:
                    if (data.size() != sizeof(uint32_t))
                        return ZenError_Io_MsgCorrupt;
                    return m_communicator->publishResult(function, ZenError_None, *reinterpret_cast<const uint32_t*>(data.data()));

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
                    return m_communicator->publishResult(function, ZenError_None, *reinterpret_cast<const uint32_t*>(data.data()));

                case EDevicePropertyV0::GetBatteryLevel:
                case EDevicePropertyV0::GetBatteryVoltage:
                    if (data.size() != sizeof(float))
                        return ZenError_Io_MsgCorrupt;
                    return m_communicator->publishResult(function, ZenError_None, *reinterpret_cast<const float*>(data.data()));

                case EDevicePropertyV0::GetSerialNumber:
                case EDevicePropertyV0::GetDeviceName:
                case EDevicePropertyV0::GetFirmwareInfo:
                    return m_communicator->publishArray(function, ZenError_None, gsl::make_span(reinterpret_cast<const std::byte*>(data.data()), data.size()));

                case EDevicePropertyV0::GetFirmwareVersion:
                    if (data.size() != sizeof(uint32_t) * 3)
                        return ZenError_Io_MsgCorrupt;
                    return m_communicator->publishArray(function, ZenError_None, gsl::make_span(reinterpret_cast<const uint32_t*>(data.data()), 3));

                case EDevicePropertyV0::GetRawSensorData:
                    if (m_initialized)
                    {
                        if (auto eventData = m_components[0]->processEventData(ZenEventType_ImuData, data))
                            publishEvent({ ZenEventType_ImuData, {m_token}, {1}, std::move(*eventData) });
                        else
                            return eventData.error();
                    }
                    return ZenError_None;

                default:
                    return m_components[0]->processData(function, data);
                }
            }
        }
        else if (m_config.version == 1)
        {
            if (auto optInternal = base::v1::internal::map(function))
            {
                switch (*optInternal)
                {
                case EDevicePropertyInternal::Ack:
                    return m_communicator->publishAck(ZenSensorProperty_Invalid, ZenError_None);

                case EDevicePropertyInternal::Nack:
                    return m_communicator->publishAck(ZenSensorProperty_Invalid, ZenError_FW_FunctionFailed);

                // this entry is used to forward the OutputDataBitset for IMU and GPS while
                // the component is not created yet.
                case EDevicePropertyInternal::ConfigImuOutputDataBitset:
                    if (data.size() != sizeof(uint32_t))
                        return ZenError_Io_MsgCorrupt;
                    return m_communicator->publishResult(static_cast<ZenProperty_t>(EDevicePropertyInternal::ConfigImuOutputDataBitset),
                        ZenError_None, *reinterpret_cast<const uint32_t*>(data.data()));

                case EDevicePropertyInternal::ConfigGetDegGradOutput:
                    if (data.size() != sizeof(uint32_t))
                        return ZenError_Io_MsgCorrupt;
                    return m_communicator->publishResult(static_cast<ZenProperty_t>(EDevicePropertyInternal::ConfigGetDegGradOutput),
                        ZenError_None, *reinterpret_cast<const uint32_t*>(data.data()));

                case EDevicePropertyInternal::ConfigGpsOutputDataBitset:
                    if (data.size() != sizeof(uint32_t) * 2)
                        return ZenError_Io_MsgCorrupt;
                    return m_communicator->publishArray(static_cast<ZenProperty_t>(EDevicePropertyInternal::ConfigGpsOutputDataBitset),
                        ZenError_None, gsl::make_span(reinterpret_cast<const std::byte*>(data.data()), data.size()));

                default:
                    return ZenError_Io_UnsupportedFunction;
                }
            }
            else
            {
                const auto property = static_cast<EDevicePropertyV1>(function);
                switch (property)
                {
                case EDevicePropertyV1::GetSerialNumber:
                case EDevicePropertyV1::GetSensorModel:
                case EDevicePropertyV1::GetFirmwareInfo:
                    return m_communicator->publishArray(function, ZenError_None, gsl::make_span(reinterpret_cast<const std::byte*>(data.data()), data.size()));

                case EDevicePropertyV1::GetRawImuSensorData:
                    if (m_initialized)
                    {
                        if (auto eventData = m_components[0]->processEventData(ZenEventType_ImuData, data))
                            publishEvent({ ZenEventType_ImuData, {m_token}, {1}, std::move(*eventData) });
                        else
                            return eventData.error();
                    }
                    return ZenError_None;

                case EDevicePropertyV1::GetRawGpsSensorData:
                    if (m_initialized)
                    {
                        if (auto eventData = m_components[1]->processEventData(ZenEventType_GnssData, data))
                            publishEvent({ ZenEventType_GnssData, {m_token}, {2}, std::move(*eventData) });
                        else
                            return eventData.error();
                    }
                    return ZenError_None;

                default:
                    // check if the components have been created at this point !
                    if (m_initialized) {
                        return m_components[0]->processData(function, data);
                    } else {
                        return ZenError_None;
                    }
                }
            }
        }

        return ZenError_Sensor_VersionNotSupported;
    }

    void Sensor::publishEvent(const ZenEvent& event) noexcept
    {
        std::lock_guard<std::mutex> lock(m_subscribersMutex);
        for (auto subscriber : m_subscribers)
            subscriber.get().push(event);
    }

    void Sensor::upload(std::vector<std::byte> firmware)
    {
        constexpr uint32_t PAGE_SIZE = 255;

        auto& outError = m_updatingFirmware ? m_updateFirmwareError : m_updateIAPError;
        auto& updated = m_updatingFirmware ? m_updatedFirmware : m_updatedIAP;
        const DeviceProperty_t property = static_cast<DeviceProperty_t>(m_updatingFirmware ? EDevicePropertyInternal::UpdateFirmware : EDevicePropertyInternal::UpdateIAP);
        const uint8_t function = m_config.version == 0 ? static_cast<uint8_t>(property) : static_cast<uint8_t>(ZenProtocolFunction_Set);

        auto guard = finally([&updated]() {
            updated = true;
        });

        const uint32_t nFullPages = static_cast<uint32_t>(firmware.size() / PAGE_SIZE);
        const uint32_t remainder = firmware.size() % PAGE_SIZE;
        const uint32_t nPages = remainder > 0 ? nFullPages + 1 : nFullPages;
        if (auto error = m_communicator->sendAndWaitForAck(0, function, property, gsl::make_span(reinterpret_cast<const std::byte*>(&nPages), sizeof(nPages))))
        {
            outError = error;
            return;
        }

        for (unsigned idx = 0; idx < nPages; ++idx)
        {
            if (auto error = m_communicator->sendAndWaitForAck(0, function, property, gsl::make_span(firmware.data() + idx * PAGE_SIZE, PAGE_SIZE)))
            {
                outError = error;
                return;
            }
        }

        if (remainder > 0)
            if (auto error = m_communicator->sendAndWaitForAck(0, function, property, gsl::make_span(firmware.data() + nFullPages * PAGE_SIZE, remainder)))
                outError = error;
    }

    ZenError Sensor::processReceivedEvent(ZenEvent evt) noexcept {
        publishEvent(evt);

        return ZenError_None;
    }
}