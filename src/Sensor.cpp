#include "Sensor.h"

#include <algorithm>
#include <cstring>
#include <numeric>
#include <string>

#include "ZenProtocol.h"
#include "SensorProperties.h"
#include "components/ComponentFactoryManager.h"
#include "components/factories/ImuComponentFactory.h"
#include "io/IIoInterface.h"
#include "properties/BaseSensorPropertiesV0.h"
#include "properties/CorePropertyRulesV1.h"
#include "properties/LegacyCoreProperties.h"
#include "utility/Finally.h"

namespace zen
{
    namespace
    {
        std::unique_ptr<IZenSensorProperties> make_properties(uint8_t id, unsigned int version, AsyncIoInterface& ioInterface)
        {
            switch (version)
            {
            case 1:
                return std::make_unique<SensorProperties<CorePropertyRulesV1>>(id, ioInterface);

            default:
                return nullptr;
            }
        }

        IZenSensorProperties& getProperties(uint8_t address, Sensor& self, const std::vector<std::unique_ptr<SensorComponent>>& components)
        {
            return *(address ? components[address - 1]->properties() : self.properties());
        }

        ZenError parseError(const unsigned char*& data, size_t& length)
        {
            const auto result = *reinterpret_cast<const ZenError*>(data);
            data += sizeof(ZenError);
            length -= sizeof(ZenError);
            return result;
        }

        ZenEvent_t parseEvent(const unsigned char*& data, size_t& length)
        {
            const auto result = *reinterpret_cast<const ZenEvent_t*>(data);
            data += sizeof(ZenEvent_t);
            length -= sizeof(ZenEvent_t);
            return result;
        }

        ZenProperty_t parseProperty(const unsigned char*& data, size_t& length)
        {
            const auto result = *reinterpret_cast<const ZenProperty_t*>(data);
            data += sizeof(ZenProperty_t);
            length -= sizeof(ZenProperty_t);
            return result;
        }
    }

    static auto imuRegistry = make_registry<ImuComponentFactory>(g_zenSensorType_Imu);

    nonstd::expected<std::unique_ptr<Sensor>, ZenSensorInitError> make_sensor(SensorConfig config, std::unique_ptr<BaseIoInterface> ioInterface)
    {
        auto sensor = std::make_unique<Sensor>(std::move(config), std::move(ioInterface));
        if (auto error = sensor->init())
            return nonstd::make_unexpected(error);

        return std::move(sensor);
    }

    Sensor::Sensor(SensorConfig config, std::unique_ptr<BaseIoInterface> ioInterface)
        : IIoDataSubscriber(*ioInterface.get())
        , m_config(std::move(config))
        , m_ioInterface(std::move(ioInterface))
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

            auto component = factory.value()->make_component(idx++, config.version, m_ioInterface);
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
            m_properties = std::make_unique<LegacyCoreProperties>(m_ioInterface, *m_components[0]->properties());
        else if (auto properties = make_properties(0, m_config.version, m_ioInterface))
            m_properties = std::move(properties);
        else
            return ZenSensorInitError_UnsupportedProtocol;

        return ZenSensorInitError_None;
    }

    ZenAsyncStatus Sensor::updateFirmwareAsync(const char* const buffer, size_t bufferSize)
    {
        if (buffer == nullptr)
            return ZenAsync_InvalidArgument;

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

        std::vector<unsigned char> firmware(bufferSize);
        std::memcpy(firmware.data(), buffer, bufferSize);

        m_uploadThread = std::thread(&Sensor::upload, this, std::move(firmware));

        return ZenAsync_Updating;
    }

    ZenAsyncStatus Sensor::updateIAPAsync(const char* const buffer, size_t bufferSize)
    {
        if (buffer == nullptr)
            return ZenAsync_InvalidArgument;

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

        std::vector<unsigned char> iap(bufferSize);
        std::memcpy(iap.data(), buffer, bufferSize);

        m_uploadThread = std::thread(&Sensor::upload, this, std::move(iap));

        return ZenAsync_Updating;
    }

    ZenError Sensor::components(IZenSensorComponent*** outComponents, size_t* outLength, const char* type) const
    {
        if (outLength == nullptr)
            return ZenError_IsNull;

        const size_t length = !type ? m_components.size() : std::accumulate(m_components.cbegin(), m_components.cend(), static_cast<size_t>(0), [=](size_t count, const auto& component) {
            return component->type() == std::string_view(type) ? count + 1 : count;
        });

        *outLength = length;
        if (!m_components.empty())
        {
            if (outComponents == nullptr)
                return ZenError_None;

            IZenSensorComponent** components = new IZenSensorComponent*[length];
            size_t idx = 0;
            for (const auto& component : m_components)
                if (!type || component->type() == std::string_view(type))
                    components[idx++] = component.get();
            
            *outComponents = components;
        }

        return ZenError_None;
    }

    bool Sensor::equals(const ZenSensorDesc* desc) const
    {
        return m_ioInterface.equals(*desc);
    }

    ZenError Sensor::processData(uint8_t address, uint8_t function, const unsigned char* data, size_t length)
    {
        if (m_config.version == 0)
        {
            if (auto optInternal = base::v0::internal::map(function))
            {
                switch (*optInternal)
                {
                case EDevicePropertyInternal::Ack:
                    return m_ioInterface.publishAck(ZenSensorProperty_Invalid, ZenError_None);

                case EDevicePropertyInternal::Nack:
                    return m_ioInterface.publishAck(ZenSensorProperty_Invalid, ZenError_FW_FunctionFailed);

                case EDevicePropertyInternal::Config:
                    if (length != sizeof(uint32_t))
                        return ZenError_Io_MsgCorrupt;
                    return m_ioInterface.publishResult(function, ZenError_None, *reinterpret_cast<const uint32_t*>(data));

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
                    if (length != sizeof(uint32_t))
                        return ZenError_Io_MsgCorrupt;
                    return m_ioInterface.publishResult(function, ZenError_None, *reinterpret_cast<const uint32_t*>(data));

                case EDevicePropertyV0::GetBatteryLevel:
                case EDevicePropertyV0::GetBatteryVoltage:
                    if (length != sizeof(float))
                        return ZenError_Io_MsgCorrupt;
                    return m_ioInterface.publishResult(function, ZenError_None, *reinterpret_cast<const float*>(data));

                case EDevicePropertyV0::GetSerialNumber:
                case EDevicePropertyV0::GetDeviceName:
                case EDevicePropertyV0::GetFirmwareInfo:
                    return m_ioInterface.publishArray(function, ZenError_None, reinterpret_cast<const char*>(data), length);

                case EDevicePropertyV0::GetFirmwareVersion:
                    if (length != sizeof(uint32_t) * 3)
                        return ZenError_Io_MsgCorrupt;
                    return m_ioInterface.publishArray(function, ZenError_None, reinterpret_cast<const uint32_t*>(data), 3);

                default:
                    if (m_initialized)
                        return m_components.at(0)->processData(function, data, length);
                    else
                        return ZenError_None;
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
                return properties::publishAck(getProperties(address, *this, m_components), m_ioInterface, parseProperty(data, length), parseError(data, length));

            case ZenProtocolFunction_Result:
                return properties::publishResult(getProperties(address, *this, m_components), m_ioInterface, parseProperty(data, length), parseError(data, length), data, length);

            case ZenProtocolFunction_Event:
                return address ? m_components[address - 1]->processEvent(parseEvent(data, length), data, length) : ZenError_UnsupportedEvent;

            default:
                return ZenError_Io_UnsupportedFunction;
            }
        }
    }

    void Sensor::upload(std::vector<unsigned char> firmware)
    {
        constexpr uint32_t PAGE_SIZE = 256;

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
        if (auto error = m_ioInterface.sendAndWaitForAck(0, function, property, gsl::make_span(reinterpret_cast<const unsigned char*>(&nPages), sizeof(nPages))))
        {
            outError = error;
            return;
        }

        for (unsigned idx = 0; idx < nPages; ++idx)
        {
            if (auto error = m_ioInterface.sendAndWaitForAck(0, function, property, gsl::make_span(firmware.data() + idx * PAGE_SIZE, PAGE_SIZE)))
            {
                outError = error;
                return;
            }
        }


        if (remainder > 0)
            if (auto error = m_ioInterface.sendAndWaitForAck(0, function, property, gsl::make_span(firmware.data() + nFullPages * PAGE_SIZE, remainder)))
                outError = error;
    }
}