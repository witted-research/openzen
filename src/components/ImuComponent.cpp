#include "components/IMUComponent.h"

#define _USE_MATH_DEFINES
#include <math.h>
#include <string>

#include "SensorManager.h"
#include "SensorProperties.h"
#include "ImuHelpers.h"
#include "properties/ImuSensorPropertiesV0.h"
#include "properties/ImuPropertyRulesV1.h"
#include "properties/LegacyImuProperties.h"

namespace zen
{
    namespace
    {
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

        std::unique_ptr<IZenSensorProperties> make_properties(uint8_t id, unsigned int version, AsyncIoInterface& ioInterface)
        {
            switch (version)
            {
            case 1:
                return std::make_unique<SensorProperties<ImuPropertyRulesV1>>(id, ioInterface);

            default:
                return nullptr;
            }
        }
    }

    ImuComponent::ImuComponent(uint8_t id, unsigned int version, Sensor& base, AsyncIoInterface& ioInterface)
        : m_cache{}
        , m_base(base)
        , m_ioInterface(ioInterface)
        , m_initialized(false)
        , m_version(version)
        , m_id(id)
    {}

    ZenError ImuComponent::init()
    {
        // Legacy sensors require a "Command Mode" to active for accessing properties
        // as well a configuration bitset to determine which data to output
        if (m_version == 0)
        {
            auto properties = std::make_unique<LegacyImuProperties>(m_ioInterface, *this);

            // Initialize to non-streaming
            if (auto error = properties->setBool(ZenImuProperty_StreamData, false))
                return error;

            uint32_t newBitset;
            if (auto error = m_ioInterface.sendAndWaitForResult(0, static_cast<DeviceProperty_t>(EDevicePropertyInternal::Config), static_cast<ZenProperty_t>(EDevicePropertyInternal::Config), {}, newBitset))
                return error;

            properties->setOutputDataBitset(newBitset);
            m_properties = std::move(properties);
        }
        else
        {
            m_properties = make_properties(m_id, m_version, m_ioInterface);
        }

        {
            auto cache = m_cache.borrow();

            ZenMatrix3x3f matrix;
            if (auto error = m_properties->getMatrix33(ZenImuProperty_AccAlignment, &matrix))
                return error;

            convertArrayToLpMatrix(matrix.data, &cache->accAlignMatrix);
            if (auto error = m_properties->getMatrix33(ZenImuProperty_GyrAlignment, &matrix))
                return error;

            convertArrayToLpMatrix(matrix.data, &cache->gyrAlignMatrix);
            if (auto error = m_properties->getMatrix33(ZenImuProperty_MagSoftIronMatrix, &matrix))
                return error;

            convertArrayToLpMatrix(matrix.data, &cache->softIronMatrix);

            size_t size = 3 * sizeof(float);
            if (auto error = m_properties->getArray(ZenImuProperty_AccBias, ZenPropertyType_Float, cache->accBias.data, &size))
                return error;

            size = 3 * sizeof(float);
            if (auto error = m_properties->getArray(ZenImuProperty_GyrBias, ZenPropertyType_Float, cache->gyrBias.data, &size))
                return error;

            size = 3 * sizeof(float);
            if (auto error = m_properties->getArray(ZenImuProperty_MagHardIronOffset, ZenPropertyType_Float, cache->hardIronOffset.data, &size))
                return error;
        }

        if (m_version == 0)
            m_initialized = true;

        return ZenError_None;
    }

    ZenError ImuComponent::processData(uint8_t function, const unsigned char* data, size_t length)
    {
        if (auto optInternal = imu::v0::internal::map(function))
        {
            switch (*optInternal)
            {
            case EDevicePropertyInternal::Config:
                if (length != sizeof(uint32_t))
                    return ZenError_Io_MsgCorrupt;
                return m_ioInterface.publishResult(function, ZenError_None, *reinterpret_cast<const uint32_t*>(data));

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
                return m_ioInterface.publishResult(function, ZenError_None, *reinterpret_cast<const uint32_t*>(data));

            case EDevicePropertyV0::GetCentricCompensationRate:
            case EDevicePropertyV0::GetFieldRadius:
                if (length != sizeof(float))
                    return ZenError_Io_MsgCorrupt;
                return m_ioInterface.publishResult(function, ZenError_None, *reinterpret_cast<const float*>(data));

            case EDevicePropertyV0::GetRawSensorData:
                return processSensorData(data, length);

            case EDevicePropertyV0::GetAccBias:
            case EDevicePropertyV0::GetGyrBias:
            case EDevicePropertyV0::GetMagBias:
            case EDevicePropertyV0::GetMagReference:
            case EDevicePropertyV0::GetMagHardIronOffset:
                if (length != sizeof(float) * 3)
                    return ZenError_Io_MsgCorrupt;
                return m_ioInterface.publishArray(function, ZenError_None, reinterpret_cast<const float*>(data), 3);

            case EDevicePropertyV0::GetAccAlignment:
            case EDevicePropertyV0::GetGyrAlignment:
            case EDevicePropertyV0::GetMagAlignment:
            case EDevicePropertyV0::GetMagSoftIronMatrix:
                // Is this valid? Row-major? Column-major transmission?
                if (length != sizeof(float) * 9)
                    return ZenError_Io_MsgCorrupt;
                return m_ioInterface.publishArray(function, ZenError_None, reinterpret_cast<const float*>(data), 9);

            default:
                return ZenError_Io_UnsupportedFunction;
            }
        }
    }

    ZenError ImuComponent::processSensorData(const unsigned char* data, size_t length)
    {
        // Legacy check to guarantee we are not receiving streaming data before initialized
        if (m_version == 0 && !m_initialized)
            return ZenError_None;

        // Any properties that are retrieved here should be cached locally, because it
        // will take too much time to retrieve from the sensor!
        ZenEvent event{0};
        event.eventType = ZenEvent_Imu;
        event.sensor = this;

        auto& imuData = event.data.imuData;
        imuDataReset(imuData);
        imuData.timestamp = *reinterpret_cast<const uint32_t*>(data) / static_cast<float>(m_base.samplingRate());
        data += sizeof(float);

        bool enabled;
        if (auto error = m_properties->getBool(ZenImuProperty_OutputLowPrecision, &enabled))
            return error;

        const bool lowPrec = enabled;

        if (m_properties->getBool(ZenImuProperty_OutputRawGyr, &enabled) == ZenError_None && enabled)
        {
            for (unsigned idx = 0; idx < 3; ++idx)
                imuData.gRaw[idx] = (180.f / float(M_PI)) * (lowPrec ? parseFloat16(data, 1000.f) : parseFloat32(data));

            auto cache = m_cache.borrow();

            LpVector3f g;
            convertArrayToLpVector3f(imuData.gRaw, &g);
            matVectMult3(&cache->gyrAlignMatrix, &g, &g);
            vectAdd3x1(&cache->gyrBias, &g, &g);
            convertLpVector3fToArray(&g, imuData.g);
        }

        if (m_properties->getBool(ZenImuProperty_OutputRawAcc, &enabled) == ZenError_None && enabled)
        {
            for (unsigned idx = 0; idx < 3; ++idx)
                imuData.aRaw[idx] = lowPrec ? parseFloat16(data, 1000.f) : parseFloat32(data);

            auto cache = m_cache.borrow();

            LpVector3f a;
            convertArrayToLpVector3f(imuData.aRaw, &a);
            matVectMult3(&cache->accAlignMatrix, &a, &a);
            vectAdd3x1(&cache->accBias, &a, &a);
            convertLpVector3fToArray(&a, imuData.a);
        }

        if (m_properties->getBool(ZenImuProperty_OutputRawMag, &enabled) == ZenError_None && enabled)
        {
            for (unsigned idx = 0; idx < 3; ++idx)
                imuData.bRaw[idx] = lowPrec ? parseFloat16(data, 100.f) : parseFloat32(data);

            auto cache = m_cache.borrow();

            LpVector3f b;
            convertArrayToLpVector3f(imuData.bRaw, &b);
            vectSub3x1(&b, &cache->hardIronOffset, &b);
            matVectMult3(&cache->softIronMatrix, &b, &b);
            convertLpVector3fToArray(&b, imuData.b);
        }

        if (m_properties->getBool(ZenImuProperty_OutputAngularVel, &enabled) == ZenError_None && enabled)
            for (unsigned idx = 0; idx < 3; ++idx)
                imuData.w[idx] = (180.f / float(M_PI)) * (lowPrec ? parseFloat16(data, 1000.f) : parseFloat32(data));

        if (m_properties->getBool(ZenImuProperty_OutputQuat, &enabled) == ZenError_None && enabled)
        {
            for (unsigned idx = 0; idx < 4; ++idx)
                imuData.q[idx] = lowPrec ? parseFloat16(data, 10000.f) : parseFloat32(data);

            LpMatrix3x3f m;
            LpVector4f q;
            convertArrayToLpVector4f(imuData.q, &q);
            quaternionToMatrix(&q, &m);
            convertLpMatrixToArray(&m, imuData.rotationM);
        }

        if (m_properties->getBool(ZenImuProperty_OutputEuler, &enabled) == ZenError_None && enabled)
            for (unsigned idx = 0; idx < 3; ++idx)
                imuData.r[idx] = (180.f / float(M_PI)) * lowPrec ? parseFloat16(data, 10000.f) : parseFloat32(data);

        if (m_properties->getBool(ZenImuProperty_OutputLinearAcc, &enabled) == ZenError_None && enabled)
            for (unsigned idx = 0; idx < 3; ++idx)
                imuData.linAcc[idx] = lowPrec ? parseFloat16(data, 1000.f) : parseFloat32(data);

        if (m_properties->getBool(ZenImuProperty_OutputPressure, &enabled) == ZenError_None && enabled)
            imuData.pressure = lowPrec ? parseFloat16(data, 100.f) : parseFloat32(data);

        if (m_properties->getBool(ZenImuProperty_OutputAltitude, &enabled) == ZenError_None && enabled)
            imuData.altitude = lowPrec ? parseFloat16(data, 10.f) : parseFloat32(data);

        if (m_properties->getBool(ZenImuProperty_OutputTemperature, &enabled) == ZenError_None && enabled)
            imuData.temperature = lowPrec ? parseFloat16(data, 100.f) : parseFloat32(data);

        if (m_properties->getBool(ZenImuProperty_OutputHeaveMotion, &enabled) == ZenError_None && enabled)
            imuData.hm.yHeave = lowPrec ? parseFloat16(data, 1000.f) : parseFloat32(data);

        SensorManager::get().notifyEvent(std::move(event));
        return ZenError_None;
    }
}
