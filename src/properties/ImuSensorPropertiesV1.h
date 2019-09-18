#ifndef ZEN_PROPERTIES_IMUSENSORPROPERTIESV1_H_
#define ZEN_PROPERTIES_IMUSENSORPROPERTIESV1_H_

#include <array>
#include <cstring>
#include <utility>

#include "InternalTypes.h"
#include "ZenTypes.h"

#define GET_OR(x) isGetter ? (x) : EDevicePropertyV1::Ack
#define SET_OR(x) isGetter ? EDevicePropertyV1::Ack : (x)
#define GET_SET(x, y) isGetter ? (x) : (y)

namespace zen
{
    namespace imu::v1
    {
        constexpr EDevicePropertyV1 mapCommand(ZenProperty_t command)
        {
            switch (command)
            {
            /* case ZenImuProperty_PollSensorData:
                return EDevicePropertyV1::GetRawSensorData; */

            case ZenImuProperty_CalibrateGyro:
                return EDevicePropertyV1::StartGyroCalibration;

            case ZenImuProperty_ResetOrientationOffset:
                return EDevicePropertyV1::ResetOrientationOffset;

            default:
                return EDevicePropertyV1::Ack;
            }
        }

        constexpr EDevicePropertyV1 map(ZenProperty_t property, bool isGetter)
        {
            switch (property)
            {
            case ZenImuProperty_SamplingRate:
                return SET_OR(EDevicePropertyV1::SetSamplingRate);

            case ZenImuProperty_CentricCompensationRate:
                return GET_SET(EDevicePropertyV1::GetCentricCompensationRate, EDevicePropertyV1::SetCentricCompensationRate);

            case ZenImuProperty_LinearCompensationRate:
                return GET_SET(EDevicePropertyV1::GetLinearCompensationRate, EDevicePropertyV1::SetLinearCompensationRate);

            case ZenImuProperty_FieldRadius:
                return GET_SET(EDevicePropertyV1::GetFieldRadius, EDevicePropertyV1::SetFieldRadius);

            case ZenImuProperty_FilterMode:
                return GET_SET(EDevicePropertyV1::GetFilterMode, EDevicePropertyV1::SetFilterMode);

            case ZenImuProperty_FilterPreset:
                return GET_SET(EDevicePropertyV1::GetFilterPreset, EDevicePropertyV1::SetFilterPreset);

            case ZenImuProperty_OrientationOffsetMode:
                return SET_OR(EDevicePropertyV1::SetOrientationOffsetMode);

            case ZenImuProperty_AccBias:
                return GET_SET(EDevicePropertyV1::GetAccBias, EDevicePropertyV1::SetAccBias);

            case ZenImuProperty_GyrAlignment:
                return GET_SET(EDevicePropertyV1::GetGyrAlignment, EDevicePropertyV1::SetGyrAlignment);

            case ZenImuProperty_GyrBias:
                return GET_SET(EDevicePropertyV1::GetGyrBias, EDevicePropertyV1::SetGyrBias);

            case ZenImuProperty_GyrRange:
                return GET_SET(EDevicePropertyV1::GetGyrRange, EDevicePropertyV1::SetGyrRange);

            case ZenImuProperty_GyrUseAutoCalibration:
                return SET_OR(EDevicePropertyV1::SetGyrUseAutoCalibration);

            case ZenImuProperty_GyrUseThreshold:
                return SET_OR(EDevicePropertyV1::SetGyrUseThreshold);

            case ZenImuProperty_MagAlignment:
                return GET_SET(EDevicePropertyV1::GetMagAlignment, EDevicePropertyV1::SetMagAlignment);

            case ZenImuProperty_MagBias:
                return GET_SET(EDevicePropertyV1::GetMagBias, EDevicePropertyV1::SetMagBias);

            case ZenImuProperty_MagRange:
                return GET_SET(EDevicePropertyV1::GetMagRange, EDevicePropertyV1::SetMagRange);

            case ZenImuProperty_MagReference:
                return GET_SET(EDevicePropertyV1::GetMagReference, EDevicePropertyV1::SetMagReference);

            case ZenImuProperty_MagHardIronOffset:
                return GET_SET(EDevicePropertyV1::GetMagHardIronOffset, EDevicePropertyV1::SetMagHardIronOffset);

            case ZenImuProperty_MagSoftIronMatrix:
                return GET_SET(EDevicePropertyV1::GetMagSoftIronMatrix, EDevicePropertyV1::SetMagSoftIronMatrix);

            /* CAN bus properties */
            case ZenImuProperty_CanChannelMode:
                return GET_SET(EDevicePropertyV1::GetCanConfiguration, EDevicePropertyV1::SetCanChannelMode);

            case ZenImuProperty_CanPointMode:
                return GET_SET(EDevicePropertyV1::GetCanConfiguration, EDevicePropertyV1::SetCanPointMode);

            case ZenImuProperty_CanStartId:
                return GET_SET(EDevicePropertyV1::GetCanConfiguration, EDevicePropertyV1::SetCanStartId);

            /* case ZenImuProperty_CanBaudrate:
                return GET_SET(EDevicePropertyV1::GetConfig, EDevicePropertyV1::SetCanBaudrate); */

            case ZenImuProperty_CanMapping:
                return GET_SET(EDevicePropertyV1::GetCanMapping, EDevicePropertyV1::SetCanMapping);

            case ZenImuProperty_CanHeartbeat:
                return GET_SET(EDevicePropertyV1::GetCanHeartbeat, EDevicePropertyV1::SetCanHeartbeat);

            /* UART output properties */
            case ZenImuProperty_UartBaudRate:
                return GET_SET(EDevicePropertyV1::GetUartBaudrate, EDevicePropertyV1::SetUartBaudrate);

            case ZenImuProperty_UartFormat:
                return GET_SET(EDevicePropertyV1::GetUartBaudrate, EDevicePropertyV1::SetUartFormat);

            default:
                return EDevicePropertyV1::Ack;
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
            else
                return 400;
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
