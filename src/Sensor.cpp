#include "Sensor.h"

#include <algorithm>
#include <cstring>
#include <string>

#include "ZenProtocol.h"
#include "SensorProperties.h"
#include "components/ImuComponent.h"
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
    }

    Sensor::Sensor(std::unique_ptr<BaseIoInterface> ioInterface)
        : IIoDataSubscriber(*ioInterface.get())
        , m_ioInterface(std::move(ioInterface))
        , m_updatingFirmware(false)
        , m_updatedFirmware(false)
        , m_updatingIAP(false)
        , m_updatedIAP(false)
        , m_version(0)
    {}

    Sensor::~Sensor()
    {
        // We need to wait for the firmware/IAP upload to stop
        if (m_uploadThread.joinable())
            m_uploadThread.join();
    }

    ZenError Sensor::init()
    {
        // [XXX] Even before this we'd want to know the IOs baudrate, and set the IO Interface baudrate accordingly
        // [XXX] That way we can remove this from the external API

        // [TODO] Determine version based on init
        if (m_version == 0)
        {
            // Legacy version are always Imu sensors
            auto imu = std::make_unique<ImuComponent>(1, 0, *this, m_ioInterface);

            m_properties = std::make_unique<LegacyCoreProperties>(m_ioInterface, *imu.get());
            m_samplingRate = 200;

            m_components.emplace_back(std::move(imu));
        }
        else
        {
            if (auto properties = make_properties(0, m_version, m_ioInterface))
                m_properties = std::move(properties);
            else
                return ZenError_Sensor_VersionNotSupported;

            int32_t samplingRate;
            if (auto error = m_properties->getInt32(ZenSensorProperty_SamplingRate, &samplingRate))
                return error;

            m_samplingRate = samplingRate;

            // [TODO] Add components based on init
        }

        for (auto& component : m_components)
            if (auto error = component->init())
                return error;

        return ZenError_None;
    }

    ZenError Sensor::poll()
    {
        return m_ioInterface.poll();
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

    ZenError Sensor::components(IZenSensorComponent*** outComponents, size_t* outLength) const
    {
        if (outLength == nullptr)
            return ZenError_IsNull;

        if (!m_components.empty())
        {
            IZenSensorComponent** components = new IZenSensorComponent*[m_components.size()];
            for (size_t idx = 0; idx < m_components.size(); ++idx)
                components[idx] = m_components[idx].get();
            
            *outComponents = components;
        }

        *outLength = m_components.size();
        return ZenError_None;
    }

    bool Sensor::equals(const ZenSensorDesc* desc) const
    {
        return m_ioInterface.equals(*desc);
    }

    ZenError Sensor::processData(uint8_t address, uint8_t function, const unsigned char* data, size_t length)
    {
        if (m_version == 0)
        {
            if (auto optInternal = base::v0::internal::map(function))
            {
                switch (*optInternal)
                {
                case EDevicePropertyInternal::Ack:
                    return m_ioInterface.publishAck(ZenSensorProperty_Invalid, ZenError_None);

                case EDevicePropertyInternal::Nack:
                    return m_ioInterface.publishAck(ZenSensorProperty_Invalid, ZenError_FW_FunctionFailed);

                default:
                    break;
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
                    break;
                }
            }

            return m_components.at(0)->processData(function, data, length);
        }
        else
        {
            if (address >= m_components.size())
                return ZenError_Io_MsgCorrupt;

            if (address > 0)
                return m_components.at(address - 1)->processData(function, data, length);

            const ZenProperty_t property = *reinterpret_cast<const ZenProperty_t*>(data);
            data += sizeof(ZenProperty_t);
            const ZenError error = *reinterpret_cast<const ZenError*>(data);
            data += sizeof(ZenError);
            length -= sizeof(ZenProperty_t) + sizeof(ZenError);

            switch (function)
            {
            case ZenProtocolFunction_Ack:
                return properties::publishAck(*m_properties.get(), m_ioInterface, property, error);

            case ZenProtocolFunction_Result:
                return properties::publishResult(*m_properties.get(), m_ioInterface, property, error, data, length);

            default:
                return ZenError_Io_UnsupportedFunction;
            }
        }
    }

    void Sensor::upload(std::vector<unsigned char> firmware)
    {
        constexpr uint32_t PAGE_SIZE = 256;

        auto& error = m_updatingFirmware ? m_updateFirmwareError : m_updateIAPError;
        auto& updated = m_updatingFirmware ? m_updatedFirmware : m_updatedIAP;
        const DeviceProperty_t property = static_cast<DeviceProperty_t>(m_updatingFirmware ? EDevicePropertyInternal::UpdateFirmware : EDevicePropertyInternal::UpdateIAP);
        const uint8_t function = m_version == 0 ? static_cast<uint8_t>(property) : ZenProtocolFunction_Set;

        auto guard = finally([&updated]() {
            updated = true;
        });

        const uint32_t nFullPages = static_cast<uint32_t>(firmware.size() / PAGE_SIZE);
        const uint32_t remainder = firmware.size() % PAGE_SIZE;
        const uint32_t nPages = remainder > 0 ? nFullPages + 1 : nFullPages;
        if (error = m_ioInterface.sendAndWaitForAck(0, function, property, gsl::make_span(reinterpret_cast<const unsigned char*>(&nPages), sizeof(nPages))))
            return;

        for (unsigned idx = 0; idx < nPages; ++idx)
            if (error = m_ioInterface.sendAndWaitForAck(0, function, property, gsl::make_span(firmware.data() + idx * PAGE_SIZE, PAGE_SIZE)))
                return;

        if (remainder > 0)
            if (error = m_ioInterface.sendAndWaitForAck(0, function, property, gsl::make_span(firmware.data() + nFullPages * PAGE_SIZE, remainder)))
                return;
    }
}