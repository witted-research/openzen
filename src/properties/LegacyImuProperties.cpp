#include "properties/LegacyImuProperties.h"

#include <math.h>

#include "SensorProperties.h"
#include "properties/ImuSensorPropertiesV0.h"

namespace zen
{

    LegacyImuProperties::LegacyImuProperties(AsyncIoInterface& ioInterface)
        : m_cache{}
        , m_ioInterface(ioInterface)
    {}

    ZenError LegacyImuProperties::execute(ZenProperty_t command)
    {
        const auto commandV0 = imu::v0::mapCommand(command);
        if (imu::v0::supportsExecutingDeviceCommand(commandV0))
            return m_ioInterface.sendAndWaitForAck(0, static_cast<DeviceProperty_t>(commandV0), 0, {});

        return ZenError_UnknownProperty;
    }

    ZenError LegacyImuProperties::getArray(ZenProperty_t property, ZenPropertyType type, void* const buffer, size_t* const bufferSize)
    {
        if (bufferSize == nullptr)
            return ZenError_IsNull;

        const auto propertyV0 = imu::v0::map(property, true);
        ZenPropertyType expectedType = imu::v0::supportsGettingArrayDeviceProperty(propertyV0);
        if (property == ZenImuProperty_AccSupportedRanges ||
            property == ZenImuProperty_GyrSupportedRanges ||
            property == ZenImuProperty_MagSupportedRanges)
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
                if (property == ZenImuProperty_AccSupportedRanges)
                    return imu::v0::supportedAccRanges(reinterpret_cast<int32_t* const>(buffer), *bufferSize);
                else if (property == ZenImuProperty_GyrSupportedRanges)
                    return imu::v0::supportedGyrRanges(reinterpret_cast<int32_t* const>(buffer), *bufferSize);
                else if (property == ZenImuProperty_MagSupportedRanges)
                    return imu::v0::supportedMagRanges(reinterpret_cast<int32_t* const>(buffer), *bufferSize);
                else
                    return m_ioInterface.sendAndWaitForArray(0, function, function, {}, reinterpret_cast<int32_t*>(buffer), *bufferSize);

            default:
                return ZenError_WrongDataType;
            }
        }

        return ZenError_UnknownProperty;
    }

    ZenError LegacyImuProperties::getBool(ZenProperty_t property, bool* const outValue)
    {
        if (outValue == nullptr)
            return ZenError_IsNull;

        if (property == ZenImuProperty_GyrUseThreshold)
        {
            *outValue = m_cache.gyrUseThreshold;
            return ZenError_None;
        }
        else if (property == ZenImuProperty_OutputLowPrecision)
        {
            *outValue = getConfigDataFlag(22);
            return ZenError_None;
        }
        else if (property == ZenImuProperty_OutputLinearAcc)
        {
            *outValue = getConfigDataFlag(21);
            return ZenError_None;
        }
        else if (property == ZenImuProperty_OutputAltitude)
        {
            *outValue = getConfigDataFlag(19);
            return ZenError_None;
        }
        else if (property == ZenImuProperty_OutputQuat)
        {
            *outValue = getConfigDataFlag(18);
            return ZenError_None;
        }
        else if (property == ZenImuProperty_OutputEuler)
        {
            *outValue = getConfigDataFlag(17);
            return ZenError_None;
        }
        else if (property == ZenImuProperty_OutputAngularVel)
        {
            *outValue = getConfigDataFlag(16);
            return ZenError_None;
        }
        else if (property == ZenImuProperty_OutputHeaveMotion)
        {
            *outValue = getConfigDataFlag(14);
            return ZenError_None;
        }
        else if (property == ZenImuProperty_OutputTemperature)
        {
            *outValue = getConfigDataFlag(13);
            return ZenError_None;
        }
        else if (property == ZenImuProperty_OutputRawGyr)
        {
            *outValue = getConfigDataFlag(12);
            return ZenError_None;
        }
        else if (property == ZenImuProperty_OutputRawAcc)
        {
            *outValue = getConfigDataFlag(11);
            return ZenError_None;
        }
        else if (property == ZenImuProperty_OutputRawMag)
        {
            *outValue = getConfigDataFlag(10);
            return ZenError_None;
        }
        else if (property == ZenImuProperty_OutputPressure)
        {
            *outValue = getConfigDataFlag(9);
            return ZenError_None;
        }

        return ZenError_UnknownProperty;

    }

    ZenError LegacyImuProperties::getFloat(ZenProperty_t property, float* const outValue)
    {
        if (outValue == nullptr)
            return ZenError_IsNull;

        const auto propertyV0 = imu::v0::map(property, true);
        if (propertyV0 == EDevicePropertyV0::GetCentricCompensationRate)
        {
            uint32_t value;
            if (auto error = m_ioInterface.sendAndWaitForResult(0, static_cast<DeviceProperty_t>(propertyV0), static_cast<ZenProperty_t>(propertyV0), {}, value))
                return error;

            *outValue = value > 0 ? 1.f : 0.f;
            return ZenError_None;
        }
        else if (propertyV0 == EDevicePropertyV0::GetLinearCompensationRate)
        {
            uint32_t value;
            if (auto error = m_ioInterface.sendAndWaitForResult(0, static_cast<DeviceProperty_t>(propertyV0), static_cast<ZenProperty_t>(propertyV0), {}, value))
                return error;

            *outValue = static_cast<float>(value);
            return ZenError_None;
        }
        else if (imu::v0::supportsGettingFloatDeviceProperty(propertyV0))
            return m_ioInterface.sendAndWaitForResult(0, static_cast<DeviceProperty_t>(propertyV0), static_cast<ZenProperty_t>(propertyV0), {}, *outValue);

        return ZenError_UnknownProperty;
    }

    ZenError LegacyImuProperties::getInt32(ZenProperty_t property, int32_t* const outValue)
    {
        if (outValue == nullptr)
            return ZenError_IsNull;

        const auto propertyV0 = imu::v0::map(property, true);
        if (imu::v0::supportsGettingInt32DeviceProperty(propertyV0))
        {
            // Communication protocol only supports uint32_t
            uint32_t uiValue;
            if (auto error = m_ioInterface.sendAndWaitForResult(0, static_cast<DeviceProperty_t>(propertyV0), static_cast<ZenProperty_t>(propertyV0), {}, uiValue))
                return error;

            *outValue = static_cast<int32_t>(uiValue);
            return ZenError_None;
        }

        return ZenError_UnknownProperty;
    }

    ZenError LegacyImuProperties::getMatrix33(ZenProperty_t property, ZenMatrix3x3f* const outValue)
    {
        if (outValue == nullptr)
            return ZenError_IsNull;

        const auto propertyV0 = imu::v0::map(property, true);
        if (imu::v0::supportsGettingMatrix33DeviceProperty(propertyV0))
        {
            size_t length = 9;
            return m_ioInterface.sendAndWaitForArray(0, static_cast<DeviceProperty_t>(propertyV0), static_cast<ZenProperty_t>(propertyV0), {}, outValue->data, length);
        }

        return ZenError_UnknownProperty;
    }

    ZenError LegacyImuProperties::getString(ZenProperty_t property, char* const buffer, size_t* const bufferSize)
    {
        if (bufferSize == nullptr)
            return ZenError_IsNull;

        if (property == ZenImuProperty_SupportedFilterModes)
            return imu::v0::supportedFilterModes(buffer, *bufferSize);

        return ZenError_UnknownProperty;
    }

    ZenError LegacyImuProperties::setArray(ZenProperty_t property, ZenPropertyType type, const void* const buffer, size_t bufferSize)
    {
        if (buffer == nullptr)
            return ZenError_IsNull;

        const auto propertyV0 = imu::v0::map(property, false);
        const ZenPropertyType expectedType = imu::v0::supportsSettingArrayDeviceProperty(propertyV0);
        const DeviceProperty_t deviceProperty = static_cast<DeviceProperty_t>(propertyV0);

        if (expectedType)
        {
            if (type != expectedType)
                return ZenError_WrongDataType;

            const size_t typeSize = sizeOfPropertyType(type);
            return m_ioInterface.sendAndWaitForAck(0, deviceProperty, deviceProperty, gsl::make_span(reinterpret_cast<const unsigned char*>(buffer), typeSize * bufferSize));
        }

        return ZenError_UnknownProperty;
    }

    ZenError LegacyImuProperties::setBool(ZenProperty_t property, bool value)
    {
        if (property == ZenImuProperty_StreamData)
        {
            uint32_t temp;
            const auto propertyV0 = value ? EDevicePropertyV0::SetStreamMode : EDevicePropertyV0::SetCommandMode;
            return m_ioInterface.sendAndWaitForAck(0, static_cast<DeviceProperty_t>(propertyV0), static_cast<ZenProperty_t>(propertyV0), gsl::make_span(reinterpret_cast<unsigned char*>(&temp), sizeof(temp)));
        }
        else if (property == ZenImuProperty_OutputLowPrecision)
            return setPrecisionDataFlag(value);
        else if (property == ZenImuProperty_OutputLinearAcc)
            return setOutputDataFlag(21, value);
        else if (property == ZenImuProperty_OutputAltitude)
            return setOutputDataFlag(19, value);
        else if (property == ZenImuProperty_OutputQuat)
            return setOutputDataFlag(18, value);
        else if (property == ZenImuProperty_OutputEuler)
            return setOutputDataFlag(17, value);
        else if (property == ZenImuProperty_OutputAngularVel)
            return setOutputDataFlag(16, value);
        else if (property == ZenImuProperty_OutputHeaveMotion)
            return setOutputDataFlag(14, value);
        else if (property == ZenImuProperty_OutputTemperature)
            return setOutputDataFlag(13, value);
        else if (property == ZenImuProperty_OutputRawGyr)
            return setOutputDataFlag(12, value);
        else if (property == ZenImuProperty_OutputRawAcc)
            return setOutputDataFlag(11, value);
        else if (property == ZenImuProperty_OutputRawMag)
            return setOutputDataFlag(10, value);
        else if (property == ZenImuProperty_OutputPressure)
            return setOutputDataFlag(9, value);
        else
        {
            const auto propertyV0 = imu::v0::map(property, false);
            if (propertyV0 == EDevicePropertyV0::SetGyrUseAutoCalibration)
            {
                uint32_t iValue = value ? 1 : 0;
                if (auto error = m_ioInterface.sendAndWaitForAck(0, static_cast<DeviceProperty_t>(propertyV0), static_cast<ZenProperty_t>(propertyV0), gsl::span(reinterpret_cast<unsigned char*>(&iValue), sizeof(iValue))))
                    return error;

                m_cache.gyrAutoCalibration = value;
                return ZenError_None;
            }
            else if (propertyV0 == EDevicePropertyV0::SetGyrUseThreshold)
            {
                uint32_t iValue = value ? 1 : 0;
                if (auto error = m_ioInterface.sendAndWaitForAck(0, static_cast<DeviceProperty_t>(propertyV0), static_cast<ZenProperty_t>(propertyV0), gsl::span(reinterpret_cast<unsigned char*>(&iValue), sizeof(iValue))))
                    return error;

                m_cache.gyrUseThreshold = value;
                return ZenError_None;
            }
        }

        return ZenError_UnknownProperty;
    }

    ZenError LegacyImuProperties::setFloat(ZenProperty_t property, float value)
    {
        const auto propertyV0 = imu::v0::map(property, false);
        if (propertyV0 == EDevicePropertyV0::SetCentricCompensationRate)
        {
            constexpr float eps = std::numeric_limits<float>::epsilon();
            uint32_t iValue = (-eps <= value && value <= eps) ? 0 : 1; // Account for imprecision of float
            return m_ioInterface.sendAndWaitForAck(0, static_cast<DeviceProperty_t>(propertyV0), static_cast<ZenProperty_t>(propertyV0), gsl::make_span(reinterpret_cast<unsigned char*>(&iValue), sizeof(iValue)));
        }
        else if (propertyV0 == EDevicePropertyV0::SetLinearCompensationRate)
        {
            uint32_t iValue = lroundf(value);
            return m_ioInterface.sendAndWaitForAck(0, static_cast<DeviceProperty_t>(propertyV0), static_cast<ZenProperty_t>(propertyV0), gsl::make_span(reinterpret_cast<unsigned char*>(&iValue), sizeof(iValue)));
        }
        else if (imu::v0::supportsSettingFloatDeviceProperty(propertyV0))
            return m_ioInterface.sendAndWaitForAck(0, static_cast<DeviceProperty_t>(propertyV0), static_cast<ZenProperty_t>(propertyV0), gsl::make_span(reinterpret_cast<unsigned char*>(&value), sizeof(value)));

        return ZenError_UnknownProperty;
    }

    ZenError LegacyImuProperties::setInt32(ZenProperty_t property, int32_t value)
    {
        const auto propertyV0 = imu::v0::map(property, false);
        if (imu::v0::supportsSettingInt32DeviceProperty(propertyV0))
        {
            // Communication protocol only supports uint32_t
            uint32_t uiValue;
            if (property == ZenImuProperty_AccRange)
                uiValue = imu::v0::mapAccRange(value);
            else if (property == ZenImuProperty_GyrRange)
                uiValue = imu::v0::mapGyrRange(value);
            else if (property == ZenImuProperty_MagRange)
                uiValue = imu::v0::mapMagRange(value);
            else
                uiValue = static_cast<uint32_t>(value);

            return m_ioInterface.sendAndWaitForAck(0, static_cast<DeviceProperty_t>(propertyV0), static_cast<ZenProperty_t>(propertyV0), gsl::make_span(reinterpret_cast<unsigned char*>(&uiValue), sizeof(uiValue)));
        }

        return ZenError_UnknownProperty;
    }

    ZenError LegacyImuProperties::setMatrix33(ZenProperty_t property, const ZenMatrix3x3f* const value)
    {
        const auto propertyV0 = imu::v0::map(property, false);
        if (imu::v0::supportsSettingMatrix33DeviceProperty(propertyV0))
            return m_ioInterface.sendAndWaitForAck(0, static_cast<DeviceProperty_t>(propertyV0), static_cast<ZenProperty_t>(propertyV0), gsl::make_span(reinterpret_cast<const unsigned char*>(value->data), 9 * sizeof(float)));

        return ZenError_UnknownProperty;
    }

    ZenError LegacyImuProperties::setString(ZenProperty_t property, const char* buffer, size_t bufferSize)
    {
        // imu::v0 has no string properties that can be set, so no need to add backwards compatibility
        return ZenError_UnknownProperty;
    }

    bool LegacyImuProperties::isArray(ZenProperty_t property) const
    {
        switch (property)
        {
        case ZenImuProperty_SupportedFilterModes:
        case ZenImuProperty_AccBias:
        case ZenImuProperty_AccSupportedRanges:
        case ZenImuProperty_GyrBias:
        case ZenImuProperty_GyrSupportedRanges:
        case ZenImuProperty_MagBias:
        case ZenImuProperty_MagSupportedRanges:
        case ZenImuProperty_MagReference:
        case ZenImuProperty_MagHardIronOffset:
            return true;

        default:
            return false;
        }
    }

    bool LegacyImuProperties::isCommand(ZenProperty_t property) const
    {
        switch (property)
        {
        case ZenImuProperty_PollSensorData:
        case ZenImuProperty_CalibrateGyro:
        case ZenImuProperty_ResetOrientationOffset:
            return true;

        default:
            return false;
        }
    }

    bool LegacyImuProperties::isConstant(ZenProperty_t property) const
    {
        switch (property)
        {
        case ZenImuProperty_SupportedFilterModes:
        case ZenImuProperty_AccSupportedRanges:
        case ZenImuProperty_GyrSupportedRanges:
        case ZenImuProperty_MagSupportedRanges:
            return true;

        default:
            return false;
        }
    }

    ZenPropertyType LegacyImuProperties::type(ZenProperty_t property) const
    {
        switch (property)
        {
        case ZenImuProperty_StreamData:
        case ZenImuProperty_GyrUseAutoCalibration:
        case ZenImuProperty_GyrUseThreshold:
        case ZenImuProperty_OutputLowPrecision:
        case ZenImuProperty_OutputRawAcc:
        case ZenImuProperty_OutputRawGyr:
        case ZenImuProperty_OutputRawMag:
        case ZenImuProperty_OutputEuler:
        case ZenImuProperty_OutputQuat:
        case ZenImuProperty_OutputAngularVel:
        case ZenImuProperty_OutputLinearAcc:
        case ZenImuProperty_OutputHeaveMotion:
        case ZenImuProperty_OutputAltitude:
        case ZenImuProperty_OutputPressure:
        case ZenImuProperty_OutputTemperature:
            return ZenPropertyType_Bool;

        case ZenImuProperty_CentricCompensationRate:
        case ZenImuProperty_LinearCompensationRate:
        case ZenImuProperty_FieldRadius:
        case ZenImuProperty_AccBias:
        case ZenImuProperty_GyrBias:
        case ZenImuProperty_MagBias:
        case ZenImuProperty_MagReference:
        case ZenImuProperty_MagHardIronOffset:
            return ZenPropertyType_Float;

        case ZenImuProperty_FilterMode:
        case ZenImuProperty_FilterPreset:
        case ZenImuProperty_OrientationOffsetMode:
        case ZenImuProperty_AccRange:
        case ZenImuProperty_AccSupportedRanges:
        case ZenImuProperty_GyrRange:
        case ZenImuProperty_GyrSupportedRanges:
        case ZenImuProperty_MagRange:
        case ZenImuProperty_MagSupportedRanges:
            return ZenPropertyType_Int32;

        case ZenImuProperty_AccAlignment:
        case ZenImuProperty_GyrAlignment:
        case ZenImuProperty_MagAlignment:
        case ZenImuProperty_MagSoftIronMatrix:
            return ZenPropertyType_Matrix;

        case ZenImuProperty_SupportedFilterModes:
            return ZenPropertyType_String;

        default:
            return ZenPropertyType_Invalid;
        }
    }

    bool LegacyImuProperties::getConfigDataFlag(unsigned int index)
    {
        return (m_cache.outputDataBitset & (1 << index)) != 0;
    }

    ZenError LegacyImuProperties::setOutputDataFlag(unsigned int index, bool value)
    {
        uint32_t newBitset;
        if (value)
            newBitset = m_cache.outputDataBitset | (1 << index);
        else
            newBitset = m_cache.outputDataBitset & ~(1 << index);

        if (auto error = m_ioInterface.sendAndWaitForAck(0, static_cast<DeviceProperty_t>(EDevicePropertyV0::SetTransmitData), static_cast<ZenProperty_t>(EDevicePropertyV0::SetTransmitData), gsl::make_span(reinterpret_cast<unsigned char*>(&newBitset), sizeof(newBitset))))
            return error;

        m_cache.outputDataBitset = newBitset;
        return ZenError_None;
    }

    ZenError LegacyImuProperties::setPrecisionDataFlag(bool value)
    {
        uint32_t newBitset;
        if (value)
            newBitset = m_cache.outputDataBitset | (1 << 22);
        else
            newBitset = m_cache.outputDataBitset & ~(1 << 22);

        uint32_t iValue = value ? 1 : 0;
        if (auto error = m_ioInterface.sendAndWaitForAck(0, static_cast<DeviceProperty_t>(EDevicePropertyV0::SetDataMode), static_cast<ZenProperty_t>(EDevicePropertyV0::SetDataMode), gsl::make_span(reinterpret_cast<unsigned char*>(&iValue), sizeof(iValue))))
            return error;

        m_cache.outputDataBitset = newBitset;
        return ZenError_None;
    }
}
