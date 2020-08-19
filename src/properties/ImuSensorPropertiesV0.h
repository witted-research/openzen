//===========================================================================//
//
// Copyright (C) 2020 LP-Research Inc.
//
// This file is part of OpenZen, under the MIT License.
// See https://bitbucket.org/lpresearch/openzen/src/master/LICENSE for details
// SPDX-License-Identifier: MIT
//
//===========================================================================//

#ifndef ZEN_PROPERTIES_IMUSENSORPROPERTIESV0_H_
#define ZEN_PROPERTIES_IMUSENSORPROPERTIESV0_H_

#include <array>
#include <cstring>
#include <utility>

#include "InternalTypes.h"
#include "ZenTypes.h"

namespace zen
{
    namespace imu::v0
    {
        constexpr EDevicePropertyV0 mapCommand(ZenProperty_t command)
        {
            switch (command)
            {
            case ZenImuProperty_PollSensorData:
                return EDevicePropertyV0::GetRawSensorData;

            case ZenImuProperty_CalibrateGyro:
                return EDevicePropertyV0::StartGyroCalibration;

            case ZenImuProperty_ResetOrientationOffset:
                return EDevicePropertyV0::ResetOrientationOffset;

            case ZenImuProperty_StartSensorSync:
                return EDevicePropertyV0::StartSync;

            case ZenImuProperty_StopSensorSync:
                return EDevicePropertyV0::StopSync;

            default:
                return EDevicePropertyV0::Ack;
            }
        }

        constexpr EDevicePropertyV0 map(ZenProperty_t property, bool isGetter)
        {
            const auto set_or = [isGetter](EDevicePropertyV0 prop) {
                return isGetter ? EDevicePropertyV0::Ack : prop;
            };
            const auto get_set = [isGetter] (EDevicePropertyV0 x, EDevicePropertyV0 y) {
                return isGetter ? x : y;
            };

            switch (property)
            {
            case ZenImuProperty_SamplingRate:
                return set_or(EDevicePropertyV0::SetSamplingRate);

            case ZenImuProperty_CentricCompensationRate:
                return get_set(EDevicePropertyV0::GetCentricCompensationRate, EDevicePropertyV0::SetCentricCompensationRate);

            case ZenImuProperty_LinearCompensationRate:
                return get_set(EDevicePropertyV0::GetLinearCompensationRate, EDevicePropertyV0::SetLinearCompensationRate);

            case ZenImuProperty_FieldRadius:
                return get_set(EDevicePropertyV0::GetFieldRadius, EDevicePropertyV0::SetFieldRadius);

            case ZenImuProperty_FilterMode:
                return get_set(EDevicePropertyV0::GetFilterMode, EDevicePropertyV0::SetFilterMode);

            case ZenImuProperty_FilterPreset:
                return get_set(EDevicePropertyV0::GetFilterPreset, EDevicePropertyV0::SetFilterPreset);

            case ZenImuProperty_OrientationOffsetMode:
                return set_or(EDevicePropertyV0::SetOrientationOffsetMode);

            case ZenImuProperty_AccAlignment:
                return get_set(EDevicePropertyV0::GetAccAlignment, EDevicePropertyV0::SetAccAlignment);

            case ZenImuProperty_AccBias:
                return get_set(EDevicePropertyV0::GetAccBias, EDevicePropertyV0::SetAccBias);

            case ZenImuProperty_AccRange:
                return get_set(EDevicePropertyV0::GetAccRange, EDevicePropertyV0::SetAccRange);

            case ZenImuProperty_GyrAlignment:
                return get_set(EDevicePropertyV0::GetGyrAlignment, EDevicePropertyV0::SetGyrAlignment);

            case ZenImuProperty_GyrBias:
                return get_set(EDevicePropertyV0::GetGyrBias, EDevicePropertyV0::SetGyrBias);

            case ZenImuProperty_GyrRange:
                return get_set(EDevicePropertyV0::GetGyrRange, EDevicePropertyV0::SetGyrRange);

            case ZenImuProperty_GyrUseAutoCalibration:
                return set_or(EDevicePropertyV0::SetGyrUseAutoCalibration);

            case ZenImuProperty_MagAlignment:
                return get_set(EDevicePropertyV0::GetMagAlignment, EDevicePropertyV0::SetMagAlignment);

            case ZenImuProperty_MagBias:
                return get_set(EDevicePropertyV0::GetMagBias, EDevicePropertyV0::SetMagBias);

            case ZenImuProperty_MagRange:
                return get_set(EDevicePropertyV0::GetMagRange, EDevicePropertyV0::SetMagRange);

            case ZenImuProperty_MagReference:
                return get_set(EDevicePropertyV0::GetMagReference, EDevicePropertyV0::SetMagReference);

            case ZenImuProperty_MagHardIronOffset:
                return get_set(EDevicePropertyV0::GetMagHardIronOffset, EDevicePropertyV0::SetMagHardIronOffset);

            case ZenImuProperty_MagSoftIronMatrix:
                return get_set(EDevicePropertyV0::GetMagSoftIronMatrix, EDevicePropertyV0::SetMagSoftIronMatrix);

            /* CAN bus properties */
            case ZenImuProperty_CanChannelMode:
                return get_set(EDevicePropertyV0::GetCanConfiguration, EDevicePropertyV0::SetCanChannelMode);

            case ZenImuProperty_CanPointMode:
                return get_set(EDevicePropertyV0::GetCanConfiguration, EDevicePropertyV0::SetCanPointMode);

            case ZenImuProperty_CanStartId:
                return get_set(EDevicePropertyV0::GetCanConfiguration, EDevicePropertyV0::SetCanStartId);

            case ZenImuProperty_CanBaudrate:
                return get_set(EDevicePropertyV0::GetConfig, EDevicePropertyV0::SetCanBaudrate);

            case ZenImuProperty_CanMapping:
                return get_set(EDevicePropertyV0::GetCanMapping, EDevicePropertyV0::SetCanMapping);

            case ZenImuProperty_CanHeartbeat:
                return get_set(EDevicePropertyV0::GetCanHeartbeat, EDevicePropertyV0::SetCanHeartbeat);

            /* UART output properties */
            case ZenImuProperty_UartBaudRate:
                return get_set(EDevicePropertyV0::GetUartBaudrate, EDevicePropertyV0::SetUartBaudrate);

            case ZenImuProperty_UartFormat:
                return get_set(EDevicePropertyV0::GetUartBaudrate, EDevicePropertyV0::SetUartFormat);

            default:
                return EDevicePropertyV0::Ack;
            }
        }

        constexpr uint32_t roundSamplingRate(int32_t value)
        {
            if (value <= 5)
                return 5;
            else if (value <= 10)
                return 10;
            else if (value <= 25)
                return 25;
            else if (value <= 50)
                return 50;
            else if (value <= 100)
                return 100;
            else if (value <= 200)
                return 200;
            else if (value <= 400)
                return 400;
            else
                return 800;
        }

        inline std::pair<ZenError, size_t> supportedSamplingRates(gsl::span<int32_t> buffer)
        {
            constexpr std::array<int32_t, 7> supported{ 5, 10, 25, 50, 100, 200, 400 };

            if (static_cast<size_t>(buffer.size()) < supported.size())
                return std::make_pair(ZenError_BufferTooSmall, supported.size());

            if (buffer.data() == nullptr)
                return std::make_pair(ZenError_IsNull, supported.size());

            std::copy(supported.cbegin(), supported.cend(), buffer.begin());
            return std::make_pair(ZenError_None, supported.size());
        }

        constexpr uint32_t mapAccRange(int32_t value)
        {
            if (value <= 2)
                return 2;
            else if (value <= 4)
                return 4;
            else if (value <= 8)
                return 8;
            else
                return 16;
        }

        inline std::pair<ZenError, size_t> supportedAccRanges(gsl::span<int32_t> buffer)
        {
            constexpr std::array<int32_t, 4> supported{ 2, 4, 8, 16 };

            if (static_cast<size_t>(buffer.size()) < supported.size())
                return std::make_pair(ZenError_BufferTooSmall, supported.size());

            if (buffer.data() == nullptr)
                return std::make_pair(ZenError_IsNull, supported.size());

            std::copy(supported.cbegin(), supported.cend(), buffer.begin());
            return std::make_pair(ZenError_None, supported.size());
        }

        constexpr uint32_t mapGyrRange(int32_t value)
        {
            if (value <= 125)
                return 125;
            else if (value <= 245)
                return 245;
            else if (value <= 500)
                return 500;
            else if (value <= 1000)
                return 1000;
            else
                return 2000;
        }

        inline std::pair<ZenError, size_t> supportedGyrRanges(gsl::span<int32_t> buffer)
        {
            constexpr std::array<int32_t, 5> supported{ 125, 245, 500, 1000, 2000 };

            if (static_cast<size_t>(buffer.size()) < supported.size())
                return std::make_pair(ZenError_BufferTooSmall, supported.size());

            if (buffer.data() == nullptr)
                return std::make_pair(ZenError_IsNull, supported.size());

            std::copy(supported.cbegin(), supported.cend(), buffer.begin());
            return std::make_pair(ZenError_None, supported.size());
        }

        constexpr uint32_t mapMagRange(int32_t value)
        {
            if (value <= 4)
                return 4;
            else if (value <= 8)
                return 8;
            else if (value <= 12)
                return 12;
            else
                return 16;
        }

        inline std::pair<ZenError, size_t> supportedMagRanges(gsl::span<int32_t> buffer)
        {
            constexpr std::array<int32_t, 4> supported{ 4, 8, 12, 16 };

            if (static_cast<size_t>(buffer.size()) < supported.size())
                return std::make_pair(ZenError_BufferTooSmall, supported.size());

            if (buffer.data() == nullptr)
                return std::make_pair(ZenError_IsNull, supported.size());

            std::copy(supported.cbegin(), supported.cend(), buffer.begin());
            return std::make_pair(ZenError_None, supported.size());
        }

        constexpr std::pair<ZenError, size_t> supportedFilterModes(gsl::span<std::byte> buffer)
        {
            const char json[]{
                "{\n"
                "    \"config\": [\n"
                "        {\n"
                "            \"key\": \"Gyroscope filter\","
                "                \"value\" : 0"
                "        },"
                "        {"
                "            \"key\": \"Gyroscope & accelerometer filter\","
                "            \"value\" : 1"
                "        },"
                "        {"
                "            \"key\": \"Gyroscope, accelerometer & magnetometer filter\","
                "            \"value\" : 2"
                "        },"
                "        {"
                "            \"key\": \"Madgwick gyroscope & accelerometer filter\","
                "            \"value\" : 3"
                "        },"
                "        {"
                "            \"key\": \"Madgwick gyroscope, accelerometer & magnetometer filter\","
                "            \"value\" : 4"
                "        }"
                "    ]\n"
                "}" };

            if (static_cast<size_t>(buffer.size()) < sizeof(json))
                return std::make_pair(ZenError_BufferTooSmall, sizeof(json));

            if (buffer.data() == nullptr)
                return std::make_pair(ZenError_IsNull, sizeof(json));

            std::memcpy(buffer.data(), json, sizeof(json));
            return std::make_pair(ZenError_None, sizeof(json));
        }
    }
}

#endif
