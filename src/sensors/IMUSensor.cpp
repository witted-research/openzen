#include "sensors/IMUSensor.h"

#define _USE_MATH_DEFINES
#include <math.h>
#include <string>

#include "SensorManager.h"
#include "ImuHelpers.h"
#include "properties/ImuSensorPropertiesV0.h"

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

        float parseFloat16(const unsigned char*& it, float denominator)
        {
            const int16_t temp = int16_t(it[0]) + int16_t(it[1]) * 256;
            it += sizeof(int16_t);
            return static_cast<float>(temp) / denominator;
        }

        float parseFloat32(const unsigned char*& it)
        {
            const int32_t temp = ((int32_t(it[3]) * 256 + int32_t(it[2])) * 256 + int32_t(it[1])) * 256 + int32_t(it[0]);
            it += sizeof(int32_t);
            return *reinterpret_cast<const float*>(&temp);
        }
    }

    ImuSensor::ImuSensor(std::unique_ptr<BaseIoInterface> ioInterface)
        : BaseSensor(std::move(ioInterface))
        , m_version(0)
        , m_cache{}
        , m_gyrUseThreshold(false)
        , m_gyrAutoCalibration(false)
    {}

    ZenError ImuSensor::initExtension()
    {
        if (auto error = setBoolDeviceProperty(ZenImuProperty_StreamData, false))
            return error;

        {
            ZenMatrix3x3f matrix;
            if (auto error = getMatrix33DeviceProperty(ZenImuProperty_AccAlignment, &matrix))
                return error;

            LpMatrix3x3f temp;
            convertArrayToLpMatrix(matrix.data, &temp);
            m_cache.accAlignMatrix = temp;
            if (auto error = getMatrix33DeviceProperty(ZenImuProperty_GyrAlignment, &matrix))
                return error;

            convertArrayToLpMatrix(matrix.data, &temp);
            m_cache.gyrAlignMatrix = temp;
            if (auto error = getMatrix33DeviceProperty(ZenImuProperty_MagSoftIronMatrix, &matrix))
                return error;

            convertArrayToLpMatrix(matrix.data, &temp);
            m_cache.softIronMatrix = temp;
        }
        {
            LpVector3f vector;
            size_t size = 3 * sizeof(float);
            if (auto error = getArrayDeviceProperty(ZenImuProperty_AccBias, ZenPropertyType_Float, vector.data, &size))
                return error;

            m_cache.accBias = vector;
            size = 3 * sizeof(float);
            if (auto error = getArrayDeviceProperty(ZenImuProperty_GyrBias, ZenPropertyType_Float, vector.data, &size))
                return error;

            m_cache.gyrBias = vector;
            size = 3 * sizeof(float);
            if (auto error = getArrayDeviceProperty(ZenImuProperty_MagHardIronOffset, ZenPropertyType_Float, vector.data, &size))
                return error;

            m_cache.hardIronOffset = vector;
        }

        uint32_t newBitset;
        if (auto error = requestAndWaitForResult<uint32_t>(static_cast<DeviceProperty_t>(EDevicePropertyInternal::Config), newBitset))
            return error;

        m_cache.outputDataBitset = newBitset;

        return setBoolDeviceProperty(ZenImuProperty_StreamData, true);
    }

    ZenError ImuSensor::executeExtensionDeviceCommand(ZenCommand_t command)
    {
        switch (m_version)
        {
        case 0:
        {
            const auto commandV0 = imu::v0::mapCommand(command);
            if (imu::v0::supportsExecutingDeviceCommand(commandV0))
                return sendAndWaitForAck(static_cast<DeviceProperty_t>(commandV0), nullptr, 0);
        }

        default:
            ZenError_Unknown;
        }

        return ZenError_UnknownProperty;
    }

    ZenError ImuSensor::getExtensionArrayDeviceProperty(ZenProperty_t property, ZenPropertyType type, void* const buffer, size_t& bufferSize)
    {
        ZenPropertyType expectedType;
        DeviceProperty_t deviceProperty;
        switch (m_version)
        {
        case 0:
        {
            const auto propertyV0 = imu::v0::map(property, true);
            expectedType = imu::v0::supportsGettingArrayDeviceProperty(propertyV0);
            if (property == ZenImuProperty_AccSupportedRanges ||
                property == ZenImuProperty_GyrSupportedRanges ||
                property == ZenImuProperty_MagSupportedRanges)
                expectedType = ZenPropertyType_Int32;

            deviceProperty = static_cast<DeviceProperty_t>(propertyV0);
            break;
        }

        default:
            deviceProperty = 0;
            return ZenError_Unknown;
        }

        if (!expectedType)
            return ZenError_UnknownProperty;

        if (type != expectedType)
            return ZenError_WrongDataType;

        switch (type)
        {
        case ZenPropertyType_Byte:
            return requestAndWaitForArray<unsigned char>(deviceProperty, reinterpret_cast<unsigned char*>(buffer), bufferSize);

        case ZenPropertyType_Float:
            return requestAndWaitForArray<float>(deviceProperty, reinterpret_cast<float*>(buffer), bufferSize);

        case ZenPropertyType_Int32:
            if (property == ZenImuProperty_AccSupportedRanges)
                return imu::v0::supportedAccRanges(reinterpret_cast<int32_t* const>(buffer), bufferSize);
            else if (property == ZenImuProperty_GyrSupportedRanges)
                return imu::v0::supportedGyrRanges(reinterpret_cast<int32_t* const>(buffer), bufferSize);
            else if (property == ZenImuProperty_MagSupportedRanges)
                return imu::v0::supportedMagRanges(reinterpret_cast<int32_t* const>(buffer), bufferSize);
            else
                return requestAndWaitForArray<int32_t>(deviceProperty, reinterpret_cast<int32_t*>(buffer), bufferSize);

        default:
            return ZenError_WrongDataType;
        }
    }

    ZenError ImuSensor::getExtensionBoolDeviceProperty(ZenProperty_t property, bool& outValue)
    {
        switch (m_version)
        {
        case 0:
            if (property == ZenImuProperty_GyrUseThreshold)
            {
                outValue = m_gyrUseThreshold;
                return ZenError_None;
            }
            else if (property == ZenImuProperty_OutputLowPrecision)
            {
                outValue = getConfigDataFlag(22);
                return ZenError_None;
            }
            else if (property == ZenImuProperty_OutputLinearAcc)
            {
                outValue = getConfigDataFlag(21);
                return ZenError_None;
            }
            else if (property == ZenImuProperty_OutputAltitude)
            {
                outValue = getConfigDataFlag(19);
                return ZenError_None;
            }
            else if (property == ZenImuProperty_OutputQuat)
            {
                outValue = getConfigDataFlag(18);
                return ZenError_None;
            }
            else if (property == ZenImuProperty_OutputEuler)
            {
                outValue = getConfigDataFlag(17);
                return ZenError_None;
            }
            else if (property == ZenImuProperty_OutputAngularVel)
            {
                outValue = getConfigDataFlag(16);
                return ZenError_None;
            }
            else if (property == ZenImuProperty_OutputHeaveMotion)
            {
                outValue = getConfigDataFlag(14);
                return ZenError_None;
            }
            else if (property == ZenImuProperty_OutputTemperature)
            {
                outValue = getConfigDataFlag(13);
                return ZenError_None;
            }
            else if (property == ZenImuProperty_OutputRawGyr)
            {
                outValue = getConfigDataFlag(12);
                return ZenError_None;
            }
            else if (property == ZenImuProperty_OutputRawAcc)
            {
                outValue = getConfigDataFlag(11);
                return ZenError_None;
            }
            else if (property == ZenImuProperty_OutputRawMag)
            {
                outValue = getConfigDataFlag(10);
                return ZenError_None;
            }
            else if (property == ZenImuProperty_OutputPressure)
            {
                outValue = getConfigDataFlag(9);
                return ZenError_None;
            }
            else
            {
                const auto propertyV0 = imu::v0::map(property, true);
                if (imu::v0::supportsGettingBoolDeviceProperty(propertyV0))
                    return requestAndWaitForResult<bool>(static_cast<DeviceProperty_t>(propertyV0), outValue);
                else
                    break;
            }

        default:
            return ZenError_Unknown;
        }

        return ZenError_UnknownProperty;
    }

    ZenError ImuSensor::getExtensionFloatDeviceProperty(ZenProperty_t property, float& outValue)
    {
        switch (m_version)
        {
        case 0:
        {
            const auto propertyV0 = imu::v0::map(property, true);
            if (propertyV0 == EDevicePropertyV0::GetCentricCompensationRate)
            {
                uint32_t value;
                if (auto error = requestAndWaitForResult<uint32_t>(static_cast<DeviceProperty_t>(propertyV0), value))
                    return error;

                outValue = value > 0 ? 1.f : 0.f;
                return ZenError_None;
            }
            else if (imu::v0::supportsGettingFloatDeviceProperty(propertyV0))
                return requestAndWaitForResult<float>(static_cast<DeviceProperty_t>(propertyV0), outValue);
            else
                break;
        }

        default:
            return ZenError_Unknown;
        }

        return ZenError_UnknownProperty;
    }

    ZenError ImuSensor::getExtensionInt32DeviceProperty(ZenProperty_t property, int32_t& outValue)
    {
        switch (m_version)
        {
        case 0:
        {
            const auto propertyV0 = imu::v0::map(property, true);
            if (imu::v0::supportsGettingInt32DeviceProperty(propertyV0))
            {
                // Communication protocol only supports uint32_t
                uint32_t uiValue;
                if (auto error = requestAndWaitForResult<uint32_t>(static_cast<DeviceProperty_t>(propertyV0), uiValue))
                    return error;

                outValue = static_cast<int32_t>(uiValue);
                return ZenError_None;
            }
            else
                break;
        }

        default:
            return ZenError_Unknown;
        }

        return ZenError_UnknownProperty;
    }

    ZenError ImuSensor::getExtensionMatrix33DeviceProperty(ZenProperty_t property, ZenMatrix3x3f& outValue)
    {
        size_t length = 9;
        switch (m_version)
        {
        case 0:
        {
            const auto propertyV0 = imu::v0::map(property, true);
            if (imu::v0::supportsGettingMatrix33DeviceProperty(propertyV0))
                return requestAndWaitForArray<float>(static_cast<DeviceProperty_t>(propertyV0), outValue.data, length);
            else
                break;
        }

        default:
            return ZenError_Unknown;
        }

        return ZenError_UnknownProperty;
    }

    ZenError ImuSensor::getExtensionStringDeviceProperty(ZenProperty_t property, char* const buffer, size_t& bufferSize)
    {
        switch (m_version)
        {
        case 0:
            if (property == ZenImuProperty_SupportedFilterModes)
                return imu::v0::supportedFilterModes(buffer, bufferSize);
            else
            {
                const auto propertyV0 = imu::v0::map(property, true);
                if (imu::v0::supportsGettingStringDeviceProperty(propertyV0))
                    return requestAndWaitForArray<char>(static_cast<DeviceProperty_t>(propertyV0), buffer, bufferSize);
                else
                    break;
            }

        default:
            return ZenError_Unknown;
        }

        return ZenError_UnknownProperty;
    }

    ZenError ImuSensor::setExtensionArrayDeviceProperty(ZenProperty_t property, ZenPropertyType type, const void* buffer, size_t bufferSize)
    {
        ZenPropertyType expectedType;
        DeviceProperty_t deviceProperty;
        switch (m_version)
        {
        case 0:
        {
            const auto propertyV0 = imu::v0::map(property, false);
            expectedType = imu::v0::supportsSettingArrayDeviceProperty(propertyV0);
            deviceProperty = static_cast<DeviceProperty_t>(propertyV0);
            break;
        }

        default:
            deviceProperty = 0;
            return ZenError_Unknown;
        }

        if (!expectedType)
            return ZenError_UnknownProperty;

        if (type != expectedType)
            return ZenError_WrongDataType;

        const size_t typeSize = sizeOfPropertyType(type);
        return sendAndWaitForAck(property, reinterpret_cast<const unsigned char*>(buffer), typeSize * bufferSize);
    }

    ZenError ImuSensor::setExtensionBoolDeviceProperty(ZenProperty_t property, bool value)
    {
        switch (m_version)
        {
        case 0:
            if (property == ZenImuProperty_StreamData)
            {
                uint32_t temp;
                const auto propertyV0 = value ? EDevicePropertyV0::SetStreamMode : EDevicePropertyV0::SetCommandMode;
                return sendAndWaitForAck(static_cast<DeviceProperty_t>(propertyV0), reinterpret_cast<unsigned char*>(&temp), sizeof(temp));
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
                    if (auto error = sendAndWaitForAck(static_cast<DeviceProperty_t>(propertyV0), reinterpret_cast<unsigned char*>(&iValue), sizeof(iValue)))
                        return error;

                    m_gyrAutoCalibration = value;
                    return ZenError_None;
                }
                else if (propertyV0 == EDevicePropertyV0::SetGyrUseThreshold)
                {
                    uint32_t iValue = value ? 1 : 0;
                    if (auto error = sendAndWaitForAck(static_cast<DeviceProperty_t>(propertyV0), reinterpret_cast<unsigned char*>(&iValue), sizeof(iValue)))
                        return error;

                    m_gyrUseThreshold = value;
                    return ZenError_None;
                }
                else if (imu::v0::supportsSettingBoolDeviceProperty(propertyV0))
                    return sendAndWaitForAck(static_cast<DeviceProperty_t>(propertyV0), reinterpret_cast<unsigned char*>(&value), sizeof(value));
                else
                    break;
            }

        default:
            return ZenError_Unknown;
        }

        return ZenError_UnknownProperty;
    }

    ZenError ImuSensor::setExtensionFloatDeviceProperty(ZenProperty_t property, float value)
    {
        switch (m_version)
        {
        case 0:
        {
            const auto propertyV0 = imu::v0::map(property, false);
            if (propertyV0 == EDevicePropertyV0::SetCentricCompensationRate)
            {
                constexpr float eps = std::numeric_limits<float>::epsilon();
                uint32_t iValue = (-eps <= value && value <= eps) ? 0 : 1; // Account for imprecision of float
                return sendAndWaitForAck(static_cast<DeviceProperty_t>(propertyV0), reinterpret_cast<unsigned char*>(&iValue), sizeof(iValue));
            }
            else if (imu::v0::supportsSettingFloatDeviceProperty(propertyV0))
                return sendAndWaitForAck(static_cast<DeviceProperty_t>(propertyV0), reinterpret_cast<unsigned char*>(&value), sizeof(value));
            else
                break;
        }

        default:
            return ZenError_Unknown;
        }

        return ZenError_UnknownProperty;
    }

    ZenError ImuSensor::setExtensionInt32DeviceProperty(ZenProperty_t property, int32_t value)
    {
        switch (m_version)
        {
        case 0:
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
                    uiValue = static_cast<int32_t>(value);

                return sendAndWaitForAck(static_cast<DeviceProperty_t>(propertyV0), reinterpret_cast<unsigned char*>(&uiValue), sizeof(uiValue));
            }
            else
                break;
        }

        default:
            return ZenError_Unknown;
        }

        return ZenError_UnknownProperty;
    }

    ZenError ImuSensor::setExtensionMatrix33DeviceProperty(ZenProperty_t property, const ZenMatrix3x3f& m)
    {
        switch (m_version)
        {
        case 0:
        {
            const auto propertyV0 = imu::v0::map(property, false);
            if (imu::v0::supportsSettingMatrix33DeviceProperty(propertyV0))
                return sendAndWaitForAck(static_cast<DeviceProperty_t>(propertyV0), reinterpret_cast<const unsigned char*>(m.data), 9 * sizeof(float));
            else
                break;
        }

        default:
            return ZenError_Unknown;
        }

        return ZenError_UnknownProperty;
    }

    ZenError ImuSensor::setExtensionStringDeviceProperty(ZenProperty_t property, const char* buffer, size_t bufferSize)
    {
        switch (m_version)
        {
        case 0:
        {
            const auto propertyV0 = imu::v0::map(property, false);
            if (imu::v0::supportsSettingStringDeviceProperty(propertyV0))
                return sendAndWaitForAck(static_cast<DeviceProperty_t>(propertyV0), reinterpret_cast<const unsigned char*>(buffer), bufferSize);
            else
                break;
        }

        default:
            return ZenError_Unknown;
        }

        return ZenError_UnknownProperty;
    }

    ZenError ImuSensor::extensionProcessData(uint8_t function, const unsigned char* data, size_t length)
    {
        if (auto optInternal = imu::v0::internal::map(function))
        {
            switch (*optInternal)
            {
            case EDevicePropertyInternal::Config:
                if (length != sizeof(uint32_t))
                    return ZenError_Io_MsgCorrupt;
                return publishResult<uint32_t>(function, *reinterpret_cast<const uint32_t*>(data));

            default:
                throw std::invalid_argument(std::to_string(static_cast<DeviceProperty_t>(*optInternal)));
            }
        }
        else
        {
            const auto property = static_cast<EDevicePropertyV0>(function);
            switch (property)
            {
            case EDevicePropertyV0::GetLinearCompensationRate:
            case EDevicePropertyV0::GetFilterMode:
            case EDevicePropertyV0::GetFilterPreset:
            case EDevicePropertyV0::GetAccRange:
            case EDevicePropertyV0::GetGyrRange:
            case EDevicePropertyV0::GetMagRange:
                if (length != sizeof(uint32_t))
                    return ZenError_Io_MsgCorrupt;
                return publishResult<uint32_t>(function, *reinterpret_cast<const uint32_t*>(data));

            case EDevicePropertyV0::GetCentricCompensationRate:
            case EDevicePropertyV0::GetFieldRadius:
                if (length != sizeof(float))
                    return ZenError_Io_MsgCorrupt;
                return publishResult<float>(function, *reinterpret_cast<const float*>(data));

            case EDevicePropertyV0::GetRawSensorData: // [XXX] Streaming or on request? Split or remove on request!
                return processSensorData(data, length);

            case EDevicePropertyV0::GetAccBias:
            case EDevicePropertyV0::GetGyrBias:
            case EDevicePropertyV0::GetMagBias:
            case EDevicePropertyV0::GetMagReference:
            case EDevicePropertyV0::GetMagHardIronOffset:
                if (length != sizeof(float) * 3)
                    return ZenError_Io_MsgCorrupt;
                return publishArray<float>(function, reinterpret_cast<const float*>(data), 3);

            case EDevicePropertyV0::GetAccAlignment:
            case EDevicePropertyV0::GetGyrAlignment:
            case EDevicePropertyV0::GetMagAlignment:
            case EDevicePropertyV0::GetMagSoftIronMatrix:
                // Is this valid? Row-major? Column-major transmission?
                if (length != sizeof(float) * 9)
                    return ZenError_Io_MsgCorrupt;
                return publishArray<float>(function, reinterpret_cast<const float*>(data), 9);

            default:
                return ZenError_Io_UnsupportedFunction;
            }
        }
    }

    ZenError ImuSensor::processSensorData(const unsigned char* data, size_t length)
    {
        ZenEvent event{0};
        event.eventType = ZenEvent_Imu;
        event.sensor = this;

        auto& imuData = event.data.imuData;
        imuDataReset(imuData);
        imuData.timestamp = *reinterpret_cast<const uint32_t*>(data) / static_cast<float>(m_samplingRate.load());
        data += sizeof(float);

        const bool lowPrec = getConfigDataFlag(22);

        if (getConfigDataFlag(12))
        {
            for (unsigned idx = 0; idx < 3; ++idx)
                imuData.gRaw[idx] = (180.f / float(M_PI)) * (lowPrec ? parseFloat16(data, 1000.f) : parseFloat32(data));

            const LpMatrix3x3f alignMatrix = m_cache.gyrAlignMatrix;
            const LpVector3f bias = m_cache.gyrBias;

            LpVector3f g;
            convertArrayToLpVector3f(imuData.gRaw, &g);
            matVectMult3(&alignMatrix, &g, &g);
            vectAdd3x1(&bias, &g, &g);
            convertLpVector3fToArray(&g, imuData.g);
        }

        if (getConfigDataFlag(11))
        {
            for (unsigned idx = 0; idx < 3; ++idx)
                imuData.aRaw[idx] = lowPrec ? parseFloat16(data, 1000.f) : parseFloat32(data);

            const LpMatrix3x3f alignMatrix = m_cache.accAlignMatrix;
            const LpVector3f bias = m_cache.accBias;

            LpVector3f a;
            convertArrayToLpVector3f(imuData.aRaw, &a);
            matVectMult3(&alignMatrix, &a, &a);
            vectAdd3x1(&bias, &a, &a);
            convertLpVector3fToArray(&a, imuData.a);
        }

        if (getConfigDataFlag(10))
        {
            for (unsigned idx = 0; idx < 3; ++idx)
                imuData.bRaw[idx] = lowPrec ? parseFloat16(data, 100.f) : parseFloat32(data);

            const LpVector3f hardIronOffset = m_cache.hardIronOffset;
            const LpMatrix3x3f softIronMatrix = m_cache.softIronMatrix;

            LpVector3f b;
            convertArrayToLpVector3f(imuData.bRaw, &b);
            vectSub3x1(&b, &hardIronOffset, &b);
            matVectMult3(&softIronMatrix, &b, &b);
            convertLpVector3fToArray(&b, imuData.b);
        }

        if (getConfigDataFlag(16))
            for (unsigned idx = 0; idx < 3; ++idx)
                imuData.w[idx] = (180.f / float(M_PI)) * (lowPrec ? parseFloat16(data, 1000.f) : parseFloat32(data));

        if (getConfigDataFlag(18))
        {
            for (unsigned idx = 0; idx < 4; ++idx)
                imuData.q[idx] = lowPrec ? parseFloat16(data, 10000.f) : parseFloat32(data);

            LpMatrix3x3f m;
            LpVector4f q;
            convertArrayToLpVector4f(imuData.q, &q);
            quaternionToMatrix(&q, &m);
            convertLpMatrixToArray(&m, imuData.rotationM);
        }

        if (getConfigDataFlag(17))
            for (unsigned idx = 0; idx < 3; ++idx)
                imuData.r[idx] = (180.f / float(M_PI)) * lowPrec ? parseFloat16(data, 10000.f) : parseFloat32(data);

        if (getConfigDataFlag(21))
            for (unsigned idx = 0; idx < 3; ++idx)
                imuData.linAcc[idx] = lowPrec ? parseFloat16(data, 1000.f) : parseFloat32(data);

        if (getConfigDataFlag(9))
            imuData.pressure = lowPrec ? parseFloat16(data, 100.f) : parseFloat32(data);

        if (getConfigDataFlag(19))
            imuData.altitude = lowPrec ? parseFloat16(data, 10.f) : parseFloat32(data);

        if (getConfigDataFlag(13))
            imuData.temperature = lowPrec ? parseFloat16(data, 100.f) : parseFloat32(data);

        if (getConfigDataFlag(26))
            imuData.hm.yHeave = lowPrec ? parseFloat16(data, 1000.f) : parseFloat32(data);

        // [XXX] Do we use gait tracking?

        SensorManager::get().notifyEvent(std::move(event));
        return ZenError_None;
    }

    bool ImuSensor::getConfigDataFlag(unsigned int index)
    {
        return (m_cache.outputDataBitset & (1 << index)) != 0;
    }

    ZenError ImuSensor::setOutputDataFlag(unsigned int index, bool value)
    {
        uint32_t newBitset;
        if (value)
            newBitset = m_cache.outputDataBitset | (1 << index);
        else
            newBitset = m_cache.outputDataBitset & ~(1 << index);

        if (auto error = sendAndWaitForAck(static_cast<DeviceProperty_t>(EDevicePropertyV0::SetTransmitData), reinterpret_cast<unsigned char*>(&newBitset), sizeof(newBitset)))
            return error;

        m_cache.outputDataBitset = newBitset;
        return ZenError_None;
    }

    ZenError ImuSensor::setPrecisionDataFlag(bool value)
    {
        uint32_t newBitset;
        if (value)
            newBitset = m_cache.outputDataBitset | (1 << 22);
        else
            newBitset = m_cache.outputDataBitset & ~(1 << 22);

        uint32_t iValue = value ? 1 : 0;
        if (auto error = sendAndWaitForAck(static_cast<DeviceProperty_t>(EDevicePropertyV0::SetDataMode, false), reinterpret_cast<unsigned char*>(&iValue), sizeof(iValue)))
            return error;

        m_cache.outputDataBitset = newBitset;
        return ZenError_None;
    }
}