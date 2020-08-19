//===========================================================================//
//
// Copyright (C) 2020 LP-Research Inc.
//
// This file is part of OpenZen, under the MIT License.
// See https://bitbucket.org/lpresearch/openzen/src/master/LICENSE for details
// SPDX-License-Identifier: MIT
//
//===========================================================================//

#include "components/ImuIg1Component.h"

#define _USE_MATH_DEFINES
#include <math.h>
#include <string>
#include <iostream>

#include "ZenTypesHelpers.h"
#include "SensorManager.h"
#include "properties/ImuSensorPropertiesV0.h"
#include "components/SensorParsingUtil.h"

namespace zen
{
    ImuIg1Component::ImuIg1Component(std::unique_ptr<ISensorProperties> properties, SyncedModbusCommunicator& communicator, unsigned int,
        bool secondGyroIsPrimary) noexcept
        : SensorComponent(std::move(properties))
        , m_communicator(communicator)
        , m_secondGyroIsPrimary(secondGyroIsPrimary)
    {}

    ZenSensorInitError ImuIg1Component::init() noexcept
    {
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
        case EDevicePropertyV1::GetStreamFreq:
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

        // Timestamp needs to be multiplied by 0.002 to convert to seconds
        // according to the IG1 User Manual
        // also output the raw framecount as provided by the sensor
        sensor_parsing_util::parseAndStoreScalar(data, &imuData.frameCount);
        imuData.timestamp = imuData.frameCount * 0.002;

        // to store sensor values which are not forwaded to the ImuData class for Ig1
        float unusedValue[3];
        if (auto enabled = sensor_parsing_util::readVector3IfAvailable(ZenImuProperty_OutputRawAcc,
            m_properties, data, &imuData.aRaw[0])) {}
        else {
            return nonstd::make_unexpected(enabled.error());
        }

        if (auto enabled = sensor_parsing_util::readVector3IfAvailable(ZenImuProperty_OutputAccCalibrated,
            m_properties, data, &imuData.a[0])) {}
        else {
            return nonstd::make_unexpected(enabled.error());
        }

        if (auto enabled = sensor_parsing_util::readVector3IfAvailable(ZenImuProperty_OutputRawGyr0,
            m_properties, data, &imuData.gRaw[0])) {}
        else {
            return nonstd::make_unexpected(enabled.error());
        }

        // LPMS-BE1 writes its only gyro values in the gyr1 field
        float * secondGyroTargetRaw = &unusedValue[0];
        if (m_secondGyroIsPrimary) {
            secondGyroTargetRaw = &imuData.gRaw[0];
        }
        if (auto enabled = sensor_parsing_util::readVector3IfAvailable(ZenImuProperty_OutputRawGyr1,
            m_properties, data, secondGyroTargetRaw)) {}
        else {
            return nonstd::make_unexpected(enabled.error());
        }

        if (auto enabled = sensor_parsing_util::readVector3IfAvailable(ZenImuProperty_OutputGyr0BiasCalib,
            m_properties, data, &unusedValue[0])) {}
        else {
            return nonstd::make_unexpected(enabled.error());
        }

        if (auto enabled = sensor_parsing_util::readVector3IfAvailable(ZenImuProperty_OutputGyr1BiasCalib,
            m_properties, data, &unusedValue[0])) {}
        else {
            return nonstd::make_unexpected(enabled.error());
        }

        // alignment calibration also contains the static calibration correction
        if (auto enabled = sensor_parsing_util::readVector3IfAvailable(ZenImuProperty_OutputGyr0AlignCalib,
            m_properties, data, &imuData.g[0])) {}
        else {
            return nonstd::make_unexpected(enabled.error());
        }

        // LPMS-BE1 writes its only gyro values in the gyr1 field
        float * secondGyroTarget = &unusedValue[0];
        if (m_secondGyroIsPrimary) {
            secondGyroTarget = &imuData.g[0];
        }
        if (auto enabled = sensor_parsing_util::readVector3IfAvailable(ZenImuProperty_OutputGyr1AlignCalib,
            m_properties, data, secondGyroTarget)) {}
        else {
            return nonstd::make_unexpected(enabled.error());
        }

        if (auto enabled = sensor_parsing_util::readVector3IfAvailable(ZenImuProperty_OutputRawMag,
            m_properties, data, &imuData.bRaw[0])) {}
        else {
            return nonstd::make_unexpected(enabled.error());
        }

        if (auto enabled = sensor_parsing_util::readVector3IfAvailable(ZenImuProperty_OutputMagCalib,
            m_properties, data, &imuData.b[0])) {}
        else {
            return nonstd::make_unexpected(enabled.error());
        }

        if (auto enabled = sensor_parsing_util::readVector3IfAvailable(ZenImuProperty_OutputAngularVel,
            m_properties, data, &imuData.w[0])) {}
        else {
            return nonstd::make_unexpected(enabled.error());
        }

        if (auto enabled = sensor_parsing_util::readVector4IfAvailable(ZenImuProperty_OutputQuat,
            m_properties, data, &imuData.q[0])) {}
        else {
            return nonstd::make_unexpected(enabled.error());
        }

        if (auto enabled = sensor_parsing_util::readVector3IfAvailable(ZenImuProperty_OutputEuler,
            m_properties, data, &imuData.r[0])) {}
        else {
            return nonstd::make_unexpected(enabled.error());
        }

        if (auto enabled = sensor_parsing_util::readVector3IfAvailable(ZenImuProperty_OutputLinearAcc,
            m_properties, data, &imuData.linAcc[0])) {}
        else {
            return nonstd::make_unexpected(enabled.error());
        }

        if (auto enabled = sensor_parsing_util::readScalarIfAvailable(ZenImuProperty_OutputPressure,
            m_properties, data, &imuData.pressure)) {}
        else {
            return nonstd::make_unexpected(enabled.error());
        }

        if (auto enabled = sensor_parsing_util::readScalarIfAvailable(ZenImuProperty_OutputAltitude,
            m_properties, data, &imuData.altitude)) {}
        else {
            return nonstd::make_unexpected(enabled.error());
        }

        if (auto enabled = sensor_parsing_util::readScalarIfAvailable(ZenImuProperty_OutputTemperature,
            m_properties, data, &imuData.temperature)) {}
        else {
            return nonstd::make_unexpected(enabled.error());
        }

        return eventData;
    }
}
