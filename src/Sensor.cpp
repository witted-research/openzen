#include "Sensor.h"

#include <algorithm>
#include <cstring>
#include <string>

#include "components/ImuComponent.h"
#include "io/IIoInterface.h"
#include "properties/BaseSensorPropertiesV0.h"
#include "utility/Finally.h"

namespace zen
{
    namespace
    {
        constexpr size_t sizeOfPropertyType(ZenPropertyType type)
        {
            switch (type)
            {
            case ZenPropertyType_Byte:
                return sizeof(unsigned char);

            case ZenPropertyType_Float:
                return sizeof(float);

            case ZenPropertyType_Int32:
                return sizeof(int32_t);

            default:
                return 0;
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
        m_samplingRate = 200;

        // Legacy version are always Imu sensors
        m_components.emplace_back(std::make_unique<ImuComponent>(*this, m_ioInterface));

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

    ZenError Sensor::executeDeviceCommand(ZenCommand_t command)
    {
        switch (m_version)
        {
        case 0:
        {
            const auto commandV0 = base::v0::mapCommand(command);
            if (base::v0::supportsExecutingDeviceCommand(commandV0))
                return m_ioInterface.sendAndWaitForAck(static_cast<DeviceProperty_t>(commandV0), 0, nullptr, 0);
            else
                break;
        }

        default:
            return ZenError_Unknown;
        }

        for (auto& component : m_components)
            if (auto optError = component->executeDeviceCommand(command))
                return *optError;

        return ZenError_UnknownProperty;
    }

    ZenError Sensor::getArrayDeviceProperty(ZenProperty_t property, ZenPropertyType type, void* const buffer, size_t* const bufferSize)
    {
        if (bufferSize == nullptr)
            return ZenError_IsNull;

        ZenPropertyType expectedType;
        DeviceProperty_t deviceProperty;
        switch (m_version)
        {
        case 0:
        {
            const auto propertyV0 = base::v0::map(property, true);
            expectedType = base::v0::supportsGettingArrayDeviceProperty(propertyV0);
            if (property == ZenSensorProperty_SupportedSamplingRates)
                expectedType = ZenPropertyType_Int32;

            deviceProperty = static_cast<DeviceProperty_t>(propertyV0);
            break;
        }

        default:
            return ZenError_Unknown;
        }

        if (property == ZenSensorProperty_SupportedBaudRates)
            expectedType = ZenPropertyType_Int32;

        if (expectedType)
        {
            if (type != expectedType)
                return ZenError_WrongDataType;

            switch (type)
            {
            case ZenPropertyType_Byte:
                return m_ioInterface.requestAndWaitForArray<unsigned char>(deviceProperty, 0, reinterpret_cast<unsigned char*>(buffer), *bufferSize);

            case ZenPropertyType_Float:
                return m_ioInterface.requestAndWaitForArray<float>(deviceProperty, 0, reinterpret_cast<float*>(buffer), *bufferSize);

            case ZenPropertyType_Int32:
                if (property == ZenSensorProperty_SupportedBaudRates)
                    return supportedBaudRates(buffer, *bufferSize);
                else if (property == ZenSensorProperty_SupportedSamplingRates)
                    return base::v0::supportedSamplingRates(reinterpret_cast<int32_t* const>(buffer), *bufferSize);
                else if (m_version == 0)
                {
                    // As the communication protocol only supports uint32_t for getting arrays, we need to cast all values to guarantee the correct sign
                    // This only applies to Sensor, as IMUComponent has no integer array properties
                    uint32_t* uiBuffer = reinterpret_cast<uint32_t*>(buffer);
                    if (auto error = m_ioInterface.requestAndWaitForArray<uint32_t>(deviceProperty, 0, uiBuffer, *bufferSize))
                        return error;

                    int32_t* iBuffer = reinterpret_cast<int32_t*>(buffer);
                    for (size_t idx = 0; idx < *bufferSize; ++idx)
                        iBuffer[idx] = static_cast<int32_t>(uiBuffer[idx]);

                    // Some properties need to be reversed
                    const bool reverse = property == ZenSensorProperty_FirmwareVersion;
                    if (reverse)
                        std::reverse(iBuffer, iBuffer + *bufferSize);

                    return ZenError_None;
                }
                else
                    return m_ioInterface.requestAndWaitForArray<int32_t>(deviceProperty, 0, reinterpret_cast<int32_t*>(buffer), *bufferSize);

            default:
                return ZenError_WrongDataType;
            }
        }

        for (auto& component : m_components)
            if (auto optError = component->getArrayDeviceProperty(property, type, buffer, *bufferSize))
                return *optError;

        return ZenError_UnknownProperty;
    }

    ZenError Sensor::getBoolDeviceProperty(ZenProperty_t property, bool* const outValue)
    {
        if (outValue == nullptr)
            return ZenError_IsNull;

        switch (m_version)
        {
        case 0:
        {
            const auto propertyV0 = base::v0::map(property, true);
            if (propertyV0 == EDevicePropertyV0::GetBatteryCharging)
            {
                uint32_t temp;
                if (auto error = m_ioInterface.requestAndWaitForResult<uint32_t>(static_cast<DeviceProperty_t>(propertyV0), 0, temp))
                    return error;

                *outValue = temp != 0;
                return ZenError_None;
            }
            else if (base::v0::supportsGettingBoolDeviceProperty(propertyV0))
                return m_ioInterface.requestAndWaitForResult<bool>(static_cast<DeviceProperty_t>(propertyV0), 0, *outValue);
            else
                break;
        }

        default:
            return ZenError_Unknown;
        }

        for (auto& component : m_components)
            if (auto optError = component->getBoolDeviceProperty(property, *outValue))
                return *optError;

        return ZenError_UnknownProperty;
        
    }

    ZenError Sensor::getFloatDeviceProperty(ZenProperty_t property, float* const outValue)
    {
        if (outValue == nullptr)
            return ZenError_IsNull;

        switch (m_version)
        {
        case 0:
        {
            const auto propertyV0 = base::v0::map(property, true);
            if (base::v0::supportsGettingFloatDeviceProperty(propertyV0))
                return m_ioInterface.requestAndWaitForResult<float>(static_cast<DeviceProperty_t>(propertyV0), 0, *outValue);
            else
                break;
        }

        default:
            return ZenError_Unknown;
        }

        for (auto& component : m_components)
            if (auto optError = component->getFloatDeviceProperty(property, *outValue))
                return *optError;

        return ZenError_UnknownProperty;
    }

    ZenError Sensor::getInt32DeviceProperty(ZenProperty_t property, int32_t* const outValue)
    {
        if (outValue == nullptr)
            return ZenError_IsNull;

        switch (m_version)
        {
        case 0:
            if (property == ZenSensorProperty_BaudRate)
                return m_ioInterface.baudrate(*outValue);
            else if (property == ZenSensorProperty_SamplingRate)
            {
                *outValue = m_samplingRate;
                return ZenError_None;
            }
            else
            {
                const auto propertyV0 = base::v0::map(property, true);
                if (base::v0::supportsGettingInt32DeviceProperty(propertyV0))
                {
                    // Communication protocol only supports uint32_t
                    uint32_t uiValue;
                    if (auto error = m_ioInterface.requestAndWaitForResult<uint32_t>(static_cast<DeviceProperty_t>(propertyV0), 0, uiValue))
                        return error;

                    *outValue = static_cast<int32_t>(uiValue);
                    return ZenError_None;
                }
                else
                    break;
            }

        default:
            return ZenError_UnknownProperty;
        }

        for (auto& component : m_components)
            if (auto optError = component->getInt32DeviceProperty(property, *outValue))
                return *optError;

        return ZenError_UnknownProperty;
    }

    ZenError Sensor::getMatrix33DeviceProperty(ZenProperty_t property, ZenMatrix3x3f* const outValue)
    {
        if (outValue == nullptr)
            return ZenError_IsNull;

        size_t length = 9;
        switch (m_version)
        {
        case 0:
        {
            const auto propertyV0 = base::v0::map(property, true);
            if (base::v0::supportsGettingMatrix33DeviceProperty(propertyV0))
                return m_ioInterface.requestAndWaitForArray<float>(static_cast<DeviceProperty_t>(propertyV0), 0, outValue->data, length);
            else
                break;
        }

        default:
            return ZenError_Unknown;
        }

        for (auto& component : m_components)
            if (auto optError = component->getMatrix33DeviceProperty(property, *outValue))
                return *optError;

        return ZenError_UnknownProperty;
    }

    ZenError Sensor::getStringDeviceProperty(ZenProperty_t property, char* const buffer, size_t* const bufferSize)
    {
        if (bufferSize == nullptr)
            return ZenError_IsNull;

        switch (m_version)
        {
        case 0:
        {
            const auto propertyV0 = base::v0::map(property, true);
            if (base::v0::supportsGettingStringDeviceProperty(propertyV0))
                return m_ioInterface.requestAndWaitForArray<char>(static_cast<DeviceProperty_t>(propertyV0), 0, buffer, *bufferSize);
            else
                break;
        }

        default:
            return ZenError_Unknown;
        }

        for (auto& component : m_components)
            if (auto optError = component->getStringDeviceProperty(property, buffer, *bufferSize))
                return *optError;

        return ZenError_UnknownProperty;
    }

    ZenError Sensor::setArrayDeviceProperty(ZenProperty_t property, ZenPropertyType type, const void* const buffer, size_t bufferSize)
    {
        if (buffer == nullptr)
            return ZenError_IsNull;

        ZenPropertyType expectedType;
        DeviceProperty_t deviceProperty;
        switch (m_version)
        {
        case 0:
        {
            const auto propertyV0 = base::v0::map(property, false);
            expectedType = base::v0::supportsSettingArrayDeviceProperty(propertyV0);
            deviceProperty = static_cast<DeviceProperty_t>(propertyV0);
            break;
        }
            break;

        default:
            return ZenError_Unknown;
        }

        if (expectedType)
        {
            if (type != expectedType)
                return ZenError_WrongDataType;

            const size_t typeSize = sizeOfPropertyType(type);
            return m_ioInterface.sendAndWaitForAck(deviceProperty, 0, reinterpret_cast<const unsigned char*>(buffer), typeSize * bufferSize);
        }

        for (auto& component : m_components)
            if (auto optError = component->setArrayDeviceProperty(property, type, buffer, bufferSize))
                return *optError;

        return ZenError_UnknownProperty;
    }

    ZenError Sensor::setBoolDeviceProperty(ZenProperty_t property, bool value)
    {
        switch (m_version)
        {
        case 0:
        {
            const auto propertyV0 = base::v0::map(property, false);
            if (base::v0::supportsSettingBoolDeviceProperty(propertyV0))
                return m_ioInterface.sendAndWaitForAck(static_cast<DeviceProperty_t>(propertyV0), 0, reinterpret_cast<unsigned char*>(&value), sizeof(value));
            else
                break;
        }

        default:
            return ZenError_Unknown;
        }

        for (auto& component : m_components)
            if (auto optError = component->setBoolDeviceProperty(property, value))
                return *optError;

        return ZenError_UnknownProperty;
    }

    ZenError Sensor::setFloatDeviceProperty(ZenProperty_t property, float value)
    {
        switch (m_version)
        {
        case 0:
        {
            const auto propertyV0 = base::v0::map(property, false);
            if (base::v0::supportsSettingFloatDeviceProperty(propertyV0))
                return m_ioInterface.sendAndWaitForAck(static_cast<DeviceProperty_t>(propertyV0), 0, reinterpret_cast<unsigned char*>(&value), sizeof(value));
            else
                break;
        }

        default:
            return ZenError_Unknown;
        }

        for (auto& component : m_components)
            if (auto optError = component->setFloatDeviceProperty(property, value))
                return *optError;

        return ZenError_UnknownProperty;
    }

    ZenError Sensor::setInt32DeviceProperty(ZenProperty_t property, int32_t value)
    {
        switch (m_version)
        {
        case 0:
            if (property == ZenSensorProperty_BaudRate)
                return m_ioInterface.setBaudrate(value);
            else
            {
                const auto propertyV0 = base::v0::map(property, false);
                if (propertyV0 == EDevicePropertyV0::SetSamplingRate)
                {
                    const uint32_t uiValue = base::v0::roundSamplingRate(value);
                    if (auto error = m_ioInterface.sendAndWaitForAck(static_cast<DeviceProperty_t>(propertyV0), 0, reinterpret_cast<const unsigned char*>(&uiValue), sizeof(uiValue)))
                        return error;
                    m_samplingRate = value;
                }
                else if (base::v0::supportsSettingInt32DeviceProperty(propertyV0))
                {
                    // Communication protocol only supports uint32_t
                    const uint32_t uiValue = static_cast<uint32_t>(value);
                    return m_ioInterface.sendAndWaitForAck(static_cast<DeviceProperty_t>(propertyV0), 0, reinterpret_cast<const unsigned char*>(&uiValue), sizeof(uiValue));
                }
                else
                    break;
            }

        default:
            return ZenError_Unknown;
        }

        for (auto& component : m_components)
            if (auto optError = component->setInt32DeviceProperty(property, value))
                return *optError;

        return ZenError_UnknownProperty;
    }

    ZenError Sensor::setMatrix33DeviceProperty(ZenProperty_t property, const ZenMatrix3x3f* const value)
    {
        if (value == nullptr)
            return ZenError_IsNull;

        switch (m_version)
        {
        case 0:
        {
            const auto propertyV0 = base::v0::map(property, false);
            if (base::v0::supportsSettingMatrix33DeviceProperty(propertyV0))
                return m_ioInterface.sendAndWaitForAck(static_cast<DeviceProperty_t>(propertyV0), 0, reinterpret_cast<const unsigned char*>(value->data), 9 * sizeof(float));
            else
                break;
        }

        default:
            return ZenError_Unknown;
        }

        for (auto& component : m_components)
            if (auto optError = component->setMatrix33DeviceProperty(property, *value))
                return *optError;

        return ZenError_UnknownProperty;
    }

    ZenError Sensor::setStringDeviceProperty(ZenProperty_t property, const char* buffer, size_t bufferSize)
    {
        if (buffer == nullptr)
            return ZenError_IsNull;

        switch (m_version)
        {
        case 0:
        {
            const auto propertyV0 = base::v0::map(property, false);
            if (base::v0::supportsSettingStringDeviceProperty(propertyV0))
                return m_ioInterface.sendAndWaitForAck(static_cast<DeviceProperty_t>(propertyV0), 0, reinterpret_cast<const unsigned char*>(buffer), bufferSize);
            else
                break;
        }

        default:
            return ZenError_Unknown;
        }

        for (auto& component : m_components)
            if (auto optError = component->setStringDeviceProperty(property, buffer, bufferSize))
                return *optError;

        return ZenError_UnknownProperty;
    }

    ZenError Sensor::componentTypes(ZenSensorType** outTypes, size_t* outLength) const
    {
        if (outLength == nullptr)
            return ZenError_IsNull;

        ZenSensorType* types = new ZenSensorType[m_components.size()];
        for (size_t idx = 0; idx < m_components.size(); ++idx)
            types[idx] = m_components[idx]->type();

        *outLength = m_components.size();
        return ZenError_None;
    }

    bool Sensor::equals(const ZenSensorDesc* desc) const
    {
        return m_ioInterface.equals(*desc);
    }

    ZenError Sensor::processData(uint8_t, uint8_t function, const unsigned char* data, size_t length)
    {
        if (auto optInternal = base::v0::internal::map(function))
        {
            switch (*optInternal)
            {
            case EDevicePropertyInternal::Ack:
                return m_ioInterface.publishAck(ZenSensorProperty_Invalid, 0, true);

            case EDevicePropertyInternal::Nack:
                return m_ioInterface.publishAck(ZenSensorProperty_Invalid, 0, false);

            default:
                throw std::invalid_argument(std::to_string(static_cast<DeviceProperty_t>(*optInternal)));
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
                return m_ioInterface.publishResult<uint32_t>(function, 0, *reinterpret_cast<const uint32_t*>(data));

            case EDevicePropertyV0::GetBatteryLevel:
            case EDevicePropertyV0::GetBatteryVoltage:
                if (length != sizeof(float))
                    return ZenError_Io_MsgCorrupt;
                return m_ioInterface.publishResult<float>(function, 0, *reinterpret_cast<const float*>(data));

            case EDevicePropertyV0::GetSerialNumber:
            case EDevicePropertyV0::GetDeviceName:
            case EDevicePropertyV0::GetFirmwareInfo:
                return m_ioInterface.publishArray<char>(function, 0, reinterpret_cast<const char*>(data), length);

            case EDevicePropertyV0::GetFirmwareVersion:
                if (length != sizeof(uint32_t) * 3)
                    return ZenError_Io_MsgCorrupt;
                return m_ioInterface.publishArray<uint32_t>(function, 0, reinterpret_cast<const uint32_t*>(data), 3);

            default:
                for (auto& component : m_components)
                    if (auto optError = component->processData(function, data, length))
                        return *optError;

                return ZenError_Io_UnsupportedFunction;
            }
        }
    }

    ZenError Sensor::supportedBaudRates(void* buffer, size_t& bufferSize) const
    {
        std::vector<int32_t> baudrates;
        if (auto error = m_ioInterface.supportedBaudrates(baudrates))
            return error;

        if (bufferSize < baudrates.size())
        {
            bufferSize = baudrates.size();
            return ZenError_BufferTooSmall;
        }

        if (buffer == nullptr)
            return ZenError_IsNull;

        std::memcpy(buffer, baudrates.data(), baudrates.size() * sizeof(uint32_t));
        return ZenError_None;
    }

    void Sensor::upload(std::vector<unsigned char> firmware)
    {
        constexpr uint32_t PAGE_SIZE = 256;

        auto& error = m_updatingFirmware ? m_updateFirmwareError : m_updateIAPError;
        auto& updated = m_updatingFirmware ? m_updatedFirmware : m_updatedIAP;
        const DeviceProperty_t property = static_cast<DeviceProperty_t>(m_updatingFirmware ? EDevicePropertyInternal::UpdateFirmware : EDevicePropertyInternal::UpdateIAP);

        auto guard = finally([&updated]() {
            updated = true;
        });

        const uint32_t nFullPages = static_cast<uint32_t>(firmware.size() / PAGE_SIZE);
        const uint32_t remainder = firmware.size() % PAGE_SIZE;
        const uint32_t nPages = remainder > 0 ? nFullPages + 1 : nFullPages;
        if (error = m_ioInterface.sendAndWaitForAck(property, 0, reinterpret_cast<const unsigned char*>(&nPages), sizeof(uint32_t)))
            return;

        for (unsigned idx = 0; idx < nPages; ++idx)
            if (error = m_ioInterface.sendAndWaitForAck(property, 0, firmware.data() + idx * PAGE_SIZE, PAGE_SIZE))
                return;

        if (remainder > 0)
            if (error = m_ioInterface.sendAndWaitForAck(property, 0, firmware.data() + nFullPages * PAGE_SIZE, remainder))
                return;
    }
}