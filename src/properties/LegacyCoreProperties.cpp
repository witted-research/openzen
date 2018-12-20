#include "LegacyCoreProperties.h"

#include "properties/BaseSensorPropertiesV0.h"

namespace zen
{
    LegacyCoreProperties::LegacyCoreProperties(AsyncIoInterface& ioInterface)
        : m_ioInterface(ioInterface)
    {}

    ZenError LegacyCoreProperties::execute(ZenProperty_t command)
    {
        const auto commandV0 = base::v0::mapCommand(command);
        if (base::v0::supportsExecutingDeviceCommand(commandV0))
        {
            const auto function = static_cast<DeviceProperty_t>(commandV0);
            return m_ioInterface.sendAndWaitForAck(0, function, function, {});
        }

        return ZenError_UnknownProperty;
    }

    ZenError LegacyCoreProperties::getArray(ZenProperty_t property, ZenPropertyType type, void* const buffer, size_t* const bufferSize)
    {
        if (bufferSize == nullptr)
            return ZenError_IsNull;

        const auto propertyV0 = base::v0::map(property, true);
        ZenPropertyType expectedType = base::v0::supportsGettingArrayDeviceProperty(propertyV0);
        if (property == ZenSensorProperty_SupportedSamplingRates ||
            property == ZenSensorProperty_SupportedBaudRates)
            expectedType = ZenPropertyType_Int32;

        const DeviceProperty_t function = static_cast<DeviceProperty_t>(propertyV0);
        if (expectedType)
        {
            if (type != expectedType)
                return ZenError_WrongDataType;

            switch (type)
            {
            case ZenPropertyType_Bool:
                return m_ioInterface.sendAndWaitForArray(0, function, function, {}, reinterpret_cast<bool*>(buffer), *bufferSize);

            case ZenPropertyType_Float:
                return m_ioInterface.sendAndWaitForArray(0, function, function, {}, reinterpret_cast<float*>(buffer), *bufferSize);

            case ZenPropertyType_Int32:
            {
                if (property == ZenSensorProperty_SupportedBaudRates)
                    return supportedBaudRates(buffer, *bufferSize);
                else if (property == ZenSensorProperty_SupportedSamplingRates)
                    return base::v0::supportedSamplingRates(reinterpret_cast<int32_t* const>(buffer), *bufferSize);

                // As the communication protocol only supports uint32_t for getting arrays, we need to cast all values to guarantee the correct sign
                // This only applies to Sensor, as IMUComponent has no integer array properties
                uint32_t* uiBuffer = reinterpret_cast<uint32_t*>(buffer);
                if (auto error = m_ioInterface.sendAndWaitForArray(0, function, function, {}, uiBuffer, *bufferSize))
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

            default:
                return ZenError_WrongDataType;
            }
        }

        return ZenError_UnknownProperty;
    }

    ZenError LegacyCoreProperties::getBool(ZenProperty_t property, bool* const outValue)
    {
        if (outValue == nullptr)
            return ZenError_IsNull;

        // base::v0 has no boolean properties that can be retrieved, so no need to add backwards compatibility
        const auto propertyV0 = base::v0::map(property, true);
        if (propertyV0 == EDevicePropertyV0::GetBatteryCharging)
        {
            const auto function = static_cast<DeviceProperty_t>(propertyV0);

            uint32_t temp;
            if (auto error = m_ioInterface.sendAndWaitForResult(0, function, function, {}, temp))
                return error;

            *outValue = temp != 0;
            return ZenError_None;
        }

        return ZenError_UnknownProperty;

    }

    ZenError LegacyCoreProperties::getFloat(ZenProperty_t property, float* const outValue)
    {
        if (outValue == nullptr)
            return ZenError_IsNull;

        const auto propertyV0 = base::v0::map(property, true);
        if (base::v0::supportsGettingFloatDeviceProperty(propertyV0))
        {
            const auto function = static_cast<DeviceProperty_t>(propertyV0);
            return m_ioInterface.sendAndWaitForResult(0, function, function, {}, *outValue);
        }

        return ZenError_UnknownProperty;
    }

    ZenError LegacyCoreProperties::getInt32(ZenProperty_t property, int32_t* const outValue)
    {
        if (outValue == nullptr)
            return ZenError_IsNull;

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
                const auto function = static_cast<DeviceProperty_t>(propertyV0);

                // Legacy communication protocol only supports uint32_t
                uint32_t uiValue;
                if (auto error = m_ioInterface.sendAndWaitForResult(0, function, function, {}, uiValue))
                    return error;

                *outValue = static_cast<int32_t>(uiValue);
                return ZenError_None;
            }
        }

        return ZenError_UnknownProperty;
    }

    ZenError LegacyCoreProperties::getMatrix33(ZenProperty_t property, ZenMatrix3x3f* const outValue)
    {
        // base::v0 has no matrix properties that can be retrieved, so no need to add backwards compatibility
        return ZenError_UnknownProperty;
    }

    ZenError LegacyCoreProperties::getString(ZenProperty_t property, char* const buffer, size_t* const bufferSize)
    {
        // base::v0 has no string properties that can be retrieved, so no need to add backwards compatibility
        return ZenError_UnknownProperty;
    }

    ZenError LegacyCoreProperties::setArray(ZenProperty_t property, ZenPropertyType type, const void* const buffer, size_t bufferSize)
    {
        // base::v0 has no array properties that can be set, so no need to add backwards compatibility
        return ZenError_UnknownProperty;
    }

    ZenError LegacyCoreProperties::setBool(ZenProperty_t property, bool value)
    {
        // base::v0 has no boolean properties that can be set, so no need to add backwards compatibility
        return ZenError_UnknownProperty;
    }

    ZenError LegacyCoreProperties::setFloat(ZenProperty_t property, float value)
    {
        // base::v0 has no floating-point properties that can be set, so no need to add backwards compatibility
        return ZenError_UnknownProperty;
    }

    ZenError LegacyCoreProperties::setInt32(ZenProperty_t property, int32_t value)
    {
        if (property == ZenSensorProperty_BaudRate)
            return m_ioInterface.setBaudrate(value);
        else
        {
            const auto propertyV0 = base::v0::map(property, false);
            const auto function = static_cast<DeviceProperty_t>(propertyV0);
            if (propertyV0 == EDevicePropertyV0::SetSamplingRate)
            {
                const uint32_t uiValue = base::v0::roundSamplingRate(value);

                const auto* src = reinterpret_cast<const unsigned char*>(&uiValue);
                std::vector<unsigned char> copy(src, src + sizeof(uiValue));
                if (auto error = m_ioInterface.sendAndWaitForAck(0, function, function, copy))
                    return error;
                m_samplingRate = value;
            }
            else if (base::v0::supportsSettingInt32DeviceProperty(propertyV0))
            {
                // Legacy communication protocol only supports uint32_t
                const uint32_t uiValue = static_cast<uint32_t>(value);

                const auto* src = reinterpret_cast<const unsigned char*>(&uiValue);
                std::vector<unsigned char> copy(src, src + sizeof(uiValue));
                return m_ioInterface.sendAndWaitForAck(0, function, function, copy);
            }
        }

        return ZenError_UnknownProperty;
    }

    ZenError LegacyCoreProperties::setMatrix33(ZenProperty_t property, const ZenMatrix3x3f* const value)
    {
        // base::v0 has no matrix properties that can be set, so no need to add backwards compatibility
        return ZenError_UnknownProperty;
    }

    ZenError LegacyCoreProperties::setString(ZenProperty_t property, const char* buffer, size_t bufferSize)
    {
        // base::v0 has no string properties that can be set, so no need to add backwards compatibility
        return ZenError_UnknownProperty;
    }

    bool LegacyCoreProperties::isArray(ZenProperty_t property) const
    {
        switch (property)
        {
        case ZenSensorProperty_SerialNumber:
        case ZenSensorProperty_DeviceName:
        case ZenSensorProperty_FirmwareInfo:
        case ZenSensorProperty_FirmwareVersion:
        case ZenSensorProperty_SupportedBaudRates:
        case ZenSensorProperty_SupportedSamplingRates:
            return true;

        default:
            return false;
        }
    }

    bool LegacyCoreProperties::isCommand(ZenProperty_t property) const
    {
        switch (property)
        {
        case ZenSensorProperty_RestoreFactorySettings:
        case ZenSensorProperty_StoreSettingsInFlash:
            return true;

        default:
            return false;
        }
    }

    bool LegacyCoreProperties::isConstant(ZenProperty_t property) const
    {
        switch (property)
        {
        case ZenSensorProperty_SerialNumber:
        case ZenSensorProperty_DeviceName:
        case ZenSensorProperty_FirmwareInfo:
        case ZenSensorProperty_FirmwareVersion:
        case ZenSensorProperty_SupportedBaudRates:
        case ZenSensorProperty_SupportedSamplingRates:
        case ZenSensorProperty_BatteryLevel:
        case ZenSensorProperty_BatteryVoltage:
            return true;

        default:
            return false;
        }
    }

    ZenPropertyType LegacyCoreProperties::type(ZenProperty_t property) const
    {
        switch (property)
        {
        case ZenSensorProperty_BatteryLevel:
        case ZenSensorProperty_BatteryVoltage:
            return ZenPropertyType_Float;

        case ZenSensorProperty_FirmwareVersion:
        case ZenSensorProperty_SupportedBaudRates:
        case ZenSensorProperty_SupportedSamplingRates:
        case ZenSensorProperty_TimeOffset:
        case ZenSensorProperty_DataMode:
        case ZenSensorProperty_SamplingRate:
            return ZenPropertyType_Int32;

        case ZenSensorProperty_DeviceName:
        case ZenSensorProperty_FirmwareInfo:
        case ZenSensorProperty_SerialNumber:
            return ZenPropertyType_String;
        }
    }

    ZenError LegacyCoreProperties::supportedBaudRates(void* buffer, size_t& bufferSize) const
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
}
