#include "components/ImuIg1Component.h"

#define _USE_MATH_DEFINES
#include <math.h>
#include <string>
#include <iostream>

#include "ZenTypesHelpers.h"
#include "SensorManager.h"
#include "properties/ImuSensorPropertiesV0.h"

namespace zen
{
    namespace
    {
        float parseFloat32(gsl::span<const std::byte>& data) noexcept
        {
            const int32_t temp = ((int32_t(data[3]) * 256 + int32_t(data[2])) * 256 + int32_t(data[1])) * 256 + int32_t(data[0]);
            data = data.subspan(sizeof(int32_t));
            float result;
            std::memcpy(&result, &temp, sizeof(int32_t));
            return result;
        }
    }

    ImuIg1Component::ImuIg1Component(std::unique_ptr<ISensorProperties> properties, SyncedModbusCommunicator& communicator, unsigned int) noexcept
        : SensorComponent(std::move(properties))
        , m_cache{}
        , m_communicator(communicator)
    {}

    nonstd::expected<bool, ZenError> ImuIg1Component::readScalarIfAvailable(ZenProperty_t checkProperty, gsl::span<const std::byte>& data, float * targetArray) const {
        auto enabled = m_properties->getBool(checkProperty);
        if (!enabled)
            return enabled;

        if (*enabled) {
            targetArray[0] = parseFloat32(data);
        }

        return enabled;
    }

    nonstd::expected<bool, ZenError> ImuIg1Component::readVector3IfAvailable(ZenProperty_t checkProperty, gsl::span<const std::byte>& data, float * targetArray) const {
        auto enabled = m_properties->getBool(checkProperty);
        if (!enabled)
            return enabled;

        if (*enabled) {
            for (unsigned idx = 0; idx < 3; ++idx)
                targetArray[idx] = parseFloat32(data);
        }

        return enabled;
    }

    nonstd::expected<bool, ZenError> ImuIg1Component::readVector4IfAvailable(ZenProperty_t checkProperty, gsl::span<const std::byte>& data, float * targetArray) const {
        auto enabled = m_properties->getBool(checkProperty);
        if (!enabled)
            return enabled;

        if (*enabled) {
            for (unsigned idx = 0; idx < 4; ++idx)
                targetArray[idx] = parseFloat32(data);
        }

        return enabled;
    }


    ZenSensorInitError ImuIg1Component::init() noexcept
    {
        auto cache = m_cache.borrow();

        // command mode was set by the ImuComponentFactory
        // so we don't need to set it here again.
        // Sampling rate fixed for IG1
        cache->samplingRate = 500;

        //return ZenSensorInitError_None;
/*        m_properties->subscribeToPropertyChanges(ZenImuProperty_SamplingRate, [=](SensorPropertyValue value) {
            m_cache.borrow()->samplingRate = std::get<int32_t>(value);
        });
            {
                const auto result = m_properties->getArray(ZenImuProperty_AccAlignment, ZenPropertyType_Float, gsl::make_span(reinterpret_cast<std::byte*>(cache->accAlignMatrix.data), 9));
                if (result.first)
                    return ZenSensorInitError_RetrieveFailed;
            }*/
/*
            m_properties->subscribeToPropertyChanges(ZenImuProperty_AccAlignment, [=](SensorPropertyValue value) {
                const float* data = reinterpret_cast<const float*>(std::get<gsl::span<const std::byte>>(value).data());
                convertArrayToLpMatrix(data, &cache->accAlignMatrix);
            });
            {
                const auto result = m_properties->getArray(ZenImuProperty_GyrAlignment, ZenPropertyType_Float, gsl::make_span(reinterpret_cast<std::byte*>(cache->gyrAlignMatrix.data), 9));
                if (result.first)
                    return ZenSensorInitError_RetrieveFailed;
            }
            m_properties->subscribeToPropertyChanges(ZenImuProperty_GyrAlignment, [=](SensorPropertyValue value) {
                const float* data = reinterpret_cast<const float*>(std::get<gsl::span<const std::byte>>(value).data());
                convertArrayToLpMatrix(data, &cache->gyrAlignMatrix);
            });
            {
                const auto result = m_properties->getArray(ZenImuProperty_MagSoftIronMatrix, ZenPropertyType_Float, gsl::make_span(reinterpret_cast<std::byte*>(cache->softIronMatrix.data), 9));
                if (result.first)
                    return ZenSensorInitError_RetrieveFailed;
            }
            m_properties->subscribeToPropertyChanges(ZenImuProperty_MagSoftIronMatrix, [=](SensorPropertyValue value) {
                const float* data = reinterpret_cast<const float*>(std::get<gsl::span<const std::byte>>(value).data());
                convertArrayToLpMatrix(data, &cache->softIronMatrix);
            });
            {
                const auto result = m_properties->getArray(ZenImuProperty_AccBias, ZenPropertyType_Float, gsl::make_span(reinterpret_cast<std::byte*>(cache->accBias.data), 3));
                if (result.first)
                    return ZenSensorInitError_RetrieveFailed;

                m_properties->subscribeToPropertyChanges(ZenImuProperty_AccBias, [=](SensorPropertyValue value) {
                    const float* data = reinterpret_cast<const float*>(std::get<gsl::span<const std::byte>>(value).data());
                    std::copy(data, data + 3, cache->accBias.data);
                });
            }
            {
                const auto result = m_properties->getArray(ZenImuProperty_GyrBias, ZenPropertyType_Float, gsl::make_span(reinterpret_cast<std::byte*>(cache->gyrBias.data), 3));
                if (result.first)
                    return ZenSensorInitError_RetrieveFailed;

                m_properties->subscribeToPropertyChanges(ZenImuProperty_GyrBias, [=](SensorPropertyValue value) {
                    const float* data = reinterpret_cast<const float*>(std::get<gsl::span<const std::byte>>(value).data());
                    std::copy(data, data + 3, cache->gyrBias.data);
                });
            }
            {
                const auto result = m_properties->getArray(ZenImuProperty_MagHardIronOffset, ZenPropertyType_Float, gsl::make_span(reinterpret_cast<std::byte*>(cache->hardIronOffset.data), 3));
                if (result.first)
                    return ZenSensorInitError_RetrieveFailed;

                m_properties->subscribeToPropertyChanges(ZenImuProperty_MagHardIronOffset, [=](SensorPropertyValue value) {
                    const float* data = reinterpret_cast<const float*>(std::get<gsl::span<const std::byte>>(value).data());
                    std::copy(data, data + 3, cache->hardIronOffset.data);
                });
            }
        }*/

        // Once setup is done, reset to streaming
        if (ZenError_None != m_properties->setBool(ZenImuProperty_StreamData, true))
            return ZenSensorInitError_RetrieveFailed;

        return ZenSensorInitError_None;
    }

    ZenError ImuIg1Component::processData(uint8_t function, gsl::span<const std::byte> data) noexcept
    {
        const auto property = static_cast<EDevicePropertyV1>(function);
        switch (property)
        {
        case EDevicePropertyV1::GetFilterMode:
        case EDevicePropertyV1::GetAccRange:
        case EDevicePropertyV1::GetGyrRange:
        case EDevicePropertyV1::GetMagRange:
        case EDevicePropertyV1::GetGyrThreshold:
        case EDevicePropertyV1::GetEnableGyrAutoCalibration:
        case EDevicePropertyV1::GetImuTransmitData:
            if (data.size() != sizeof(uint32_t))
                return ZenError_Io_MsgCorrupt;
            return m_communicator.publishResult(function, ZenError_None, *reinterpret_cast<const uint32_t*>(data.data()));
        default:
            return ZenError_Io_UnsupportedFunction;
        }
    }

    nonstd::expected<ZenEventData, ZenError> ImuIg1Component::processEventData(ZenEvent_t eventType, gsl::span<const std::byte> data) noexcept
    {
        switch (eventType)
        {
        case ZenImuEvent_Sample:
            return parseSensorData(data);
            break;

        default:
            return nonstd::make_unexpected(ZenError_UnsupportedEvent);
        }
    }

    nonstd::expected<ZenEventData, ZenError> ImuIg1Component::parseSensorData(gsl::span<const std::byte> data) const noexcept
    {
        // Any properties that are retrieved here should be cached locally, because it
        // will take too much time to retrieve from the sensor!
        ZenEventData eventData;
        ZenImuData& imuData = eventData.imuData;
        imuDataReset(imuData);

        const auto begin = data.begin();

        const auto size = data.size();
        if (std::distance(begin, data.begin() + sizeof(uint32_t)) > size)
            return nonstd::make_unexpected(ZenError_Io_MsgCorrupt);

        // Internal sampling rate is fixed to 500Hz
        imuData.timestamp = (double)*reinterpret_cast<const uint32_t*>(data.data()) / static_cast<float>(m_cache.borrow()->samplingRate);
        data = data.subspan(sizeof(uint32_t));

        // to store sensor values which are not forwaded to the ImuData class for Ig1
        float unusedValue[3];
        if (auto enabled = readVector3IfAvailable(ZenImuProperty_OutputRawAcc, data, &imuData.aRaw[0])) {}
        else {
            return nonstd::make_unexpected(enabled.error());
        }

        if (auto enabled = readVector3IfAvailable(ZenImuProperty_OutputAccCalibrated, data, &imuData.a[0])) {}
        else {
            return nonstd::make_unexpected(enabled.error());
        }

        if (auto enabled = readVector3IfAvailable(ZenImuProperty_OutputRawGyr0, data, &imuData.gRaw[0])) {}
        else {
            return nonstd::make_unexpected(enabled.error());
        }

        if (auto enabled = readVector3IfAvailable(ZenImuProperty_OutputRawGyr1, data, &unusedValue[0])) {}
        else {
            return nonstd::make_unexpected(enabled.error());
        }

        if (auto enabled = readVector3IfAvailable(ZenImuProperty_OutputGyr0BiasCalib, data, &unusedValue[0])) {}
        else {
            return nonstd::make_unexpected(enabled.error());
        }

        if (auto enabled = readVector3IfAvailable(ZenImuProperty_OutputGyr1BiasCalib, data, &unusedValue[0])) {}
        else {
            return nonstd::make_unexpected(enabled.error());
        }

        // alignment calibration also contains the static calibration correction
        if (auto enabled = readVector3IfAvailable(ZenImuProperty_OutputGyr0AlignCalib, data, &imuData.g[0])) {}
        else {
            return nonstd::make_unexpected(enabled.error());
        }

        if (auto enabled = readVector3IfAvailable(ZenImuProperty_OutputGyr1AlignCalib, data, &unusedValue[0])) {}
        else {
            return nonstd::make_unexpected(enabled.error());
        }

        if (auto enabled = readVector3IfAvailable(ZenImuProperty_OutputRawMag, data, &imuData.bRaw[0])) {}
        else {
            return nonstd::make_unexpected(enabled.error());
        }

        if (auto enabled = readVector3IfAvailable(ZenImuProperty_OutputMagCalib, data, &imuData.b[0])) {}
        else {
            return nonstd::make_unexpected(enabled.error());
        }

        if (auto enabled = readVector3IfAvailable(ZenImuProperty_OutputAngularVel, data, &imuData.w[0])) {}
        else {
            return nonstd::make_unexpected(enabled.error());
        }

        if (auto enabled = readVector4IfAvailable(ZenImuProperty_OutputQuat, data, &imuData.q[0])) {}
        else {
            return nonstd::make_unexpected(enabled.error());
        }

        if (auto enabled = readVector3IfAvailable(ZenImuProperty_OutputEuler, data, &imuData.r[0])) {}
        else {
            return nonstd::make_unexpected(enabled.error());
        }

        if (auto enabled = readVector3IfAvailable(ZenImuProperty_OutputLinearAcc, data, &imuData.linAcc[0])) {}
        else {
            return nonstd::make_unexpected(enabled.error());
        }

        if (auto enabled = readScalarIfAvailable(ZenImuProperty_OutputPressure, data, &imuData.pressure)) {}
        else {
            return nonstd::make_unexpected(enabled.error());
        }

        if (auto enabled = readScalarIfAvailable(ZenImuProperty_OutputAltitude, data, &imuData.altitude)) {}
        else {
            return nonstd::make_unexpected(enabled.error());
        }

        if (auto enabled = readScalarIfAvailable(ZenImuProperty_OutputTemperature, data, &imuData.temperature)) {}
        else {
            return nonstd::make_unexpected(enabled.error());
        }

        return eventData;
    }
}
