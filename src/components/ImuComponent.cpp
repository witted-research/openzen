//===========================================================================//
//
// Copyright (C) 2020 LP-Research Inc.
//
// This file is part of OpenZen, under the MIT License.
// See https://bitbucket.org/lpresearch/openzen/src/master/LICENSE for details
// SPDX-License-Identifier: MIT
//
//===========================================================================//

#include "components/ImuComponent.h"

#define _USE_MATH_DEFINES
#include <math.h>
#include <string>

#include "SensorManager.h"
#include "ZenTypesHelpers.h"
#include "properties/ImuSensorPropertiesV0.h"
#include "components/SensorParsingUtil.h"

#include <spdlog/spdlog.h>

namespace zen
{
    using sensor_parsing_util::parseFloat16;
    using sensor_parsing_util::parseFloat32;

    ImuComponent::ImuComponent(std::unique_ptr<ISensorProperties> properties, SyncedModbusCommunicator& communicator, unsigned int version) noexcept
        : SensorComponent(std::move(properties))
        , m_cache{}
        , m_communicator(communicator)
        , m_version(version)
    {}

    ZenSensorInitError ImuComponent::init() noexcept
    {
        auto & local_cache = m_cache;

        {
            const auto result = m_properties->getArray(ZenImuProperty_AccAlignment, ZenPropertyType_Float,
                gsl::make_span(reinterpret_cast<std::byte*>(local_cache.borrow()->accAlignMatrix.data), 9));
            if (result.first)
                return ZenSensorInitError_RetrieveFailed;
        }
        m_properties->subscribeToPropertyChanges(ZenImuProperty_AccAlignment,
            [&local_cache](SensorPropertyValue value) {
            const float* data = reinterpret_cast<const float*>(std::get<gsl::span<const std::byte>>(value).data());
            convertArrayToLpMatrix(data, &local_cache.borrow()->accAlignMatrix);
        });
        {
            const auto result = m_properties->getArray(ZenImuProperty_GyrAlignment, ZenPropertyType_Float,
                gsl::make_span(reinterpret_cast<std::byte*>(&local_cache.borrow()->gyrAlignMatrix.data), 9));
            if (result.first)
                return ZenSensorInitError_RetrieveFailed;
        }
        m_properties->subscribeToPropertyChanges(ZenImuProperty_GyrAlignment, [&local_cache](SensorPropertyValue value) {
            const float* data = reinterpret_cast<const float*>(std::get<gsl::span<const std::byte>>(value).data());
            convertArrayToLpMatrix(data, &local_cache.borrow()->gyrAlignMatrix);
        });
        {
            const auto result = m_properties->getArray(ZenImuProperty_MagSoftIronMatrix, ZenPropertyType_Float,
                gsl::make_span(reinterpret_cast<std::byte*>(&local_cache.borrow()->softIronMatrix.data), 9));
            if (result.first)
                return ZenSensorInitError_RetrieveFailed;
        }
        m_properties->subscribeToPropertyChanges(ZenImuProperty_MagSoftIronMatrix,
            [&local_cache](SensorPropertyValue value) {
            const float* data = reinterpret_cast<const float*>(std::get<gsl::span<const std::byte>>(value).data());
            convertArrayToLpMatrix(data, &local_cache.borrow()->softIronMatrix);
        });
        {
            const auto result = m_properties->getArray(ZenImuProperty_AccBias, ZenPropertyType_Float,
                gsl::make_span(reinterpret_cast<std::byte*>(&local_cache.borrow()->accBias.data), 3));
            if (result.first)
                return ZenSensorInitError_RetrieveFailed;

            m_properties->subscribeToPropertyChanges(ZenImuProperty_AccBias,
                [&local_cache](SensorPropertyValue value) {
                const float* data = reinterpret_cast<const float*>(std::get<gsl::span<const std::byte>>(value).data());
                std::copy(data, data + 3, local_cache.borrow()->accBias.data);
            });
        }
        {
            const auto result = m_properties->getArray(ZenImuProperty_GyrBias, ZenPropertyType_Float,
                gsl::make_span(reinterpret_cast<std::byte*>(&local_cache.borrow()->gyrBias.data), 3));
            if (result.first)
                return ZenSensorInitError_RetrieveFailed;

            m_properties->subscribeToPropertyChanges(ZenImuProperty_GyrBias,
                [&local_cache](SensorPropertyValue value) {
                const float* data = reinterpret_cast<const float*>(std::get<gsl::span<const std::byte>>(value).data());
                std::copy(data, data + 3, local_cache.borrow()->gyrBias.data);
            });
        }
        {
            const auto result = m_properties->getArray(ZenImuProperty_MagHardIronOffset, ZenPropertyType_Float,
                gsl::make_span(reinterpret_cast<std::byte*>(&local_cache.borrow()->hardIronOffset.data), 3));
            if (result.first)
                return ZenSensorInitError_RetrieveFailed;

            m_properties->subscribeToPropertyChanges(ZenImuProperty_MagHardIronOffset,
                [&local_cache](SensorPropertyValue value) {
                const float* data = reinterpret_cast<const float*>(std::get<gsl::span<const std::byte>>(value).data());
                std::copy(data, data + 3, local_cache.borrow()->hardIronOffset.data);
            });
        }

        if (m_version == 0)
        {
            // Once setup is done, reset to streaming
            if (ZenError_None != m_properties->setBool(ZenImuProperty_StreamData, true))
                return ZenSensorInitError_RetrieveFailed;
        }

        return ZenSensorInitError_None;
    }

    ZenError ImuComponent::processData(uint8_t function, gsl::span<const std::byte> data) noexcept
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
        case EDevicePropertyV0::GetUartBaudrate:
        case EDevicePropertyV0::GetCanHeartbeat:
        case EDevicePropertyV0::GetCanConfiguration:
            if (data.size() != sizeof(uint32_t))
                return ZenError_Io_MsgCorrupt;
            return m_communicator.publishResult(function, ZenError_None, *reinterpret_cast<const uint32_t*>(data.data()));

        case EDevicePropertyV0::GetCentricCompensationRate:
        case EDevicePropertyV0::GetFieldRadius:
            if (data.size() != sizeof(float))
                return ZenError_Io_MsgCorrupt;
            return m_communicator.publishResult(function, ZenError_None, *reinterpret_cast<const float*>(data.data()));

        case EDevicePropertyV0::GetAccBias:
        case EDevicePropertyV0::GetGyrBias:
        case EDevicePropertyV0::GetMagBias:
        case EDevicePropertyV0::GetMagReference:
        case EDevicePropertyV0::GetMagHardIronOffset:
            if (data.size() != sizeof(float) * 3)
                return ZenError_Io_MsgCorrupt;
            return m_communicator.publishArray(function, ZenError_None, gsl::make_span(reinterpret_cast<const float*>(data.data()), 3));

        case EDevicePropertyV0::GetAccAlignment:
        case EDevicePropertyV0::GetGyrAlignment:
        case EDevicePropertyV0::GetMagAlignment:
        case EDevicePropertyV0::GetMagSoftIronMatrix:
            // Is this valid? Row-major? Column-major transmission?
            if (data.size() != sizeof(float) * 9)
                return ZenError_Io_MsgCorrupt;
            return m_communicator.publishArray(function, ZenError_None, gsl::make_span(reinterpret_cast<const float*>(data.data()), 9));

        case EDevicePropertyV0::GetCanMapping:
            if (data.size() != sizeof(uint32_t) * 16)
                return ZenError_Io_MsgCorrupt;
            return m_communicator.publishArray(function, ZenError_None, gsl::make_span(reinterpret_cast<const float*>(data.data()), 16));

        default:
            return ZenError_Io_UnsupportedFunction;
        }
    }

    nonstd::expected<ZenEventData, ZenError> ImuComponent::processEventData(ZenEventType eventType, gsl::span<const std::byte> data) noexcept
    {
        switch (eventType)
        {
        case ZenEventType_ImuData:
            return parseSensorData(data);

        default:
            return nonstd::make_unexpected(ZenError_UnsupportedEvent);
        }
    }

    nonstd::expected<ZenEventData, ZenError> ImuComponent::parseSensorData(gsl::span<const std::byte> data) const noexcept
    {
        // Any properties that are retrieved here should be cached locally, because it
        // will take too much time to retrieve from the sensor!
        ZenEventData eventData;
        ZenImuData& imuData = eventData.imuData;
        imuDataReset(imuData);

        const auto begin = data.begin();

        const auto size = data.size();
        if (std::distance(begin, data.begin() + sizeof(uint32_t)) > size)
            return nonstd::make_unexpected(ZenError_Io_MsgCorrupt);;

        sensor_parsing_util::parseAndStoreScalar(data, &imuData.frameCount);

        float timestampMultiplier = 0.0f;
        if (const auto samplingRate = m_properties->getInt32(ZenImuProperty_SamplingRate)){
            // When the VR firmware runs with 800 Hz, it also runs with an internal
            // frequency of 800 Hz which means we need to multiply wtih 0.00125 to comput the
            // correct timestamp.
            // therefore, this value is set depending on the IMU variant.
            timestampMultiplier = samplingRate.value() > 400 ? 0.00125f : 0.0025f;
        } else {
            spdlog::error("Cannot query sampling rate to comput timestamp");
            return nonstd::make_unexpected(samplingRate.error());;
        }

        imuData.timestamp = imuData.frameCount * timestampMultiplier;

        if (auto lowPrec = m_properties->getBool(ZenImuProperty_OutputLowPrecision))
        {
            const size_t floatSize = *lowPrec ? sizeof(uint16_t) : sizeof(float);
            if (auto enabled = m_properties->getBool(ZenImuProperty_OutputRawGyr))
            {
                if (*enabled)
                {
                    if (data.size() < (long int)(3 * floatSize)) {
                        spdlog::error("Can't parse gyroscope because data entries missing.");
                        return nonstd::make_unexpected(ZenError_Io_MsgCorrupt);
                    }

                    for (unsigned idx = 0; idx < 3; ++idx)
                        imuData.gRaw[idx] = (180.f / float(M_PI)) * (*lowPrec ? parseFloat16(data, 1000.f) : parseFloat32(data));

                    auto cache = m_cache.borrow();

                    LpVector3f g;
                    convertArrayToLpVector3f(imuData.gRaw, &g);
                    matVectMult3(&cache->gyrAlignMatrix, &g, &g);
                    vectAdd3x1(&cache->gyrBias, &g, &g);
                    convertLpVector3fToArray(&g, imuData.g);
                }
            }
            else
            {
                return nonstd::make_unexpected(enabled.error());;
            }

            if (auto enabled = m_properties->getBool(ZenImuProperty_OutputRawAcc))
            {
                if (*enabled)
                {
                    if (data.size() < (long int)(3 * floatSize)) {
                        spdlog::error("Can't parse acceleration because data entries missing.");
                        return nonstd::make_unexpected(ZenError_Io_MsgCorrupt);
                    }

                    for (unsigned idx = 0; idx < 3; ++idx)
                        imuData.aRaw[idx] = *lowPrec ? parseFloat16(data, 1000.f) : parseFloat32(data);

                    auto cache = m_cache.borrow();

                    LpVector3f a;
                    convertArrayToLpVector3f(imuData.aRaw, &a);
                    matVectMult3(&cache->accAlignMatrix, &a, &a);
                    vectAdd3x1(&cache->accBias, &a, &a);
                    convertLpVector3fToArray(&a, imuData.a);
                }
            }
            else
            {
                return nonstd::make_unexpected(enabled.error());;
            }

            if (auto enabled = m_properties->getBool(ZenImuProperty_OutputRawMag))
            {
                if (*enabled)
                {
                    if (data.size() < (long int)(3 * floatSize)) {
                        spdlog::error("Can't parse magnetometer because data entries missing.");
                        return nonstd::make_unexpected(ZenError_Io_MsgCorrupt);
                    }

                    for (unsigned idx = 0; idx < 3; ++idx)
                        imuData.bRaw[idx] = *lowPrec ? parseFloat16(data, 100.f) : parseFloat32(data);

                    auto cache = m_cache.borrow();

                    LpVector3f b;
                    convertArrayToLpVector3f(imuData.bRaw, &b);
                    vectSub3x1(&b, &cache->hardIronOffset, &b);
                    matVectMult3(&cache->softIronMatrix, &b, &b);
                    convertLpVector3fToArray(&b, imuData.b);
                }
            }
            else
            {
                return nonstd::make_unexpected(enabled.error());;
            }

            // this is the angular velocity which takes into account when an orientation offset was
            // done
            if (auto enabled = m_properties->getBool(ZenImuProperty_OutputAngularVel))
            {
                if (*enabled)
                {
                    if (data.size() < (long int)(3 * floatSize)) {
                        spdlog::error("Can't parse angular velocity because data entries missing.");
                        return nonstd::make_unexpected(ZenError_Io_MsgCorrupt);
                    }

                    for (unsigned idx = 0; idx < 3; ++idx)
                        imuData.w[idx] = (180.f / float(M_PI)) * (*lowPrec ? parseFloat16(data, 1000.f) : parseFloat32(data));
                }
            }
            else
            {
                return nonstd::make_unexpected(enabled.error());;
            }

            if (auto enabled = m_properties->getBool(ZenImuProperty_OutputQuat))
            {
                if (*enabled)
                {
                    if (data.size() < (long int)(4 * floatSize)) {
                        spdlog::error("Can't parse quaternion because data entries missing.");
                        return nonstd::make_unexpected(ZenError_Io_MsgCorrupt);
                    }

                    for (unsigned idx = 0; idx < 4; ++idx)
                        imuData.q[idx] = *lowPrec ? parseFloat16(data, 10000.f) : parseFloat32(data);

                    LpMatrix3x3f m;
                    LpVector4f q;
                    convertArrayToLpVector4f(imuData.q, &q);
                    quaternionToMatrix(&q, &m);
                    convertLpMatrixToArray(&m, imuData.rotationM);
                }
            }
            else
            {
                return nonstd::make_unexpected(enabled.error());;
            }

            if (auto enabled = m_properties->getBool(ZenImuProperty_OutputEuler))
            {
                if (*enabled)
                {
                    if (data.size() < (long int)(3 * floatSize)) {
                        spdlog::error("Can't parse euler angles because data entries missing.");
                        return nonstd::make_unexpected(ZenError_Io_MsgCorrupt);
                    }

                    for (unsigned idx = 0; idx < 3; ++idx)
                        imuData.r[idx] = (180.f / float(M_PI)) * (*lowPrec ? parseFloat16(data, 10000.f) : parseFloat32(data));
                }
            }
            else
            {
                return nonstd::make_unexpected(enabled.error());;
            }

            if (auto enabled = m_properties->getBool(ZenImuProperty_OutputLinearAcc))
            {
                if (*enabled)
                {
                    if (data.size() < (long int)(3 * floatSize)) {
                        spdlog::error("Can't parse linear acceleration because data entries missing.");
                        return nonstd::make_unexpected(ZenError_Io_MsgCorrupt);
                    }

                    for (unsigned idx = 0; idx < 3; ++idx)
                        imuData.linAcc[idx] = *lowPrec ? parseFloat16(data, 1000.f) : parseFloat32(data);
                }
            }
            else
            {
                return nonstd::make_unexpected(enabled.error());;
            }

            if (auto enabled = m_properties->getBool(ZenImuProperty_OutputPressure))
            {
                if (*enabled)
                {
                    if (data.size() < (long int)floatSize) {
                        spdlog::error("Can't parse pressure because data entries missing.");
                        return nonstd::make_unexpected(ZenError_Io_MsgCorrupt);
                    }

                    imuData.pressure = *lowPrec ? parseFloat16(data, 100.f) : parseFloat32(data);
                }
            }
            else
            {
                return nonstd::make_unexpected(enabled.error());;
            }

            if (auto enabled = m_properties->getBool(ZenImuProperty_OutputAltitude))
            {
                if (*enabled)
                {
                    if (data.size() < (long int)floatSize) {
                        spdlog::error("Can't parse altitude because data entries missing.");
                        return nonstd::make_unexpected(ZenError_Io_MsgCorrupt);
                    }

                    imuData.altitude = *lowPrec ? parseFloat16(data, 10.f) : parseFloat32(data);
                }
            }
            else
            {
                return nonstd::make_unexpected(enabled.error());;
            }

            if (auto enabled = m_properties->getBool(ZenImuProperty_OutputTemperature))
            {
                if (*enabled)
                {
                    if (data.size() < (long int)floatSize) {
                        spdlog::error("Can't parse temperature because data entries missing.");
                        return nonstd::make_unexpected(ZenError_Io_MsgCorrupt);
                    }

                    imuData.temperature = *lowPrec ? parseFloat16(data, 100.f) : parseFloat32(data);
                }
            }
            else
            {
                return nonstd::make_unexpected(enabled.error());;
            }

            if (auto enabled = m_properties->getBool(ZenImuProperty_OutputHeaveMotion))
            {
                if (*enabled)
                {
                    if (data.size() < (long int)floatSize) {
                        spdlog::error("Can't parse heave motion because data entries missing.");
                        return nonstd::make_unexpected(ZenError_Io_MsgCorrupt);
                    }

                    imuData.heaveMotion = *lowPrec ? parseFloat16(data, 1000.f) : parseFloat32(data);
                }
            }
            else
            {
                return nonstd::make_unexpected(enabled.error());;
            }

            return eventData;
        }
        else
        {
            return nonstd::make_unexpected(lowPrec.error());
        }
    }
}
