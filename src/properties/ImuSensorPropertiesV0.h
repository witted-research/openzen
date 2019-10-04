#ifndef ZEN_PROPERTIES_IMUSENSORPROPERTIESV0_H_
#define ZEN_PROPERTIES_IMUSENSORPROPERTIESV0_H_

#include <array>
#include <cstring>
#include <utility>

#include "InternalTypes.h"
#include "ZenTypes.h"

#define GET_OR(x) isGetter ? (x) : EDevicePropertyV0::Ack
#define SET_OR(x) isGetter ? EDevicePropertyV0::Ack : (x)
#define GET_SET(x, y) isGetter ? (x) : (y)

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

            default:
                return EDevicePropertyV0::Ack;
            }
        }

        constexpr EDevicePropertyV0 map(ZenProperty_t property, bool isGetter)
        {
            switch (property)
            {
            case ZenImuProperty_SamplingRate:
                return SET_OR(EDevicePropertyV0::SetSamplingRate);

            case ZenImuProperty_CentricCompensationRate:
                return GET_SET(EDevicePropertyV0::GetCentricCompensationRate, EDevicePropertyV0::SetCentricCompensationRate);

            case ZenImuProperty_LinearCompensationRate:
                return GET_SET(EDevicePropertyV0::GetLinearCompensationRate, EDevicePropertyV0::SetLinearCompensationRate);

            case ZenImuProperty_FieldRadius:
                return GET_SET(EDevicePropertyV0::GetFieldRadius, EDevicePropertyV0::SetFieldRadius);

            case ZenImuProperty_FilterMode:
                return GET_SET(EDevicePropertyV0::GetFilterMode, EDevicePropertyV0::SetFilterMode);

            case ZenImuProperty_FilterPreset:
                return GET_SET(EDevicePropertyV0::GetFilterPreset, EDevicePropertyV0::SetFilterPreset);

            case ZenImuProperty_OrientationOffsetMode:
                return SET_OR(EDevicePropertyV0::SetOrientationOffsetMode);

            case ZenImuProperty_AccAlignment:
                return GET_SET(EDevicePropertyV0::GetAccAlignment, EDevicePropertyV0::SetAccAlignment);

            case ZenImuProperty_AccBias:
                return GET_SET(EDevicePropertyV0::GetAccBias, EDevicePropertyV0::SetAccBias);

            case ZenImuProperty_AccRange:
                return GET_SET(EDevicePropertyV0::GetAccRange, EDevicePropertyV0::SetAccRange);

            case ZenImuProperty_GyrAlignment:
                return GET_SET(EDevicePropertyV0::GetGyrAlignment, EDevicePropertyV0::SetGyrAlignment);

            case ZenImuProperty_GyrBias:
                return GET_SET(EDevicePropertyV0::GetGyrBias, EDevicePropertyV0::SetGyrBias);

            case ZenImuProperty_GyrRange:
                return GET_SET(EDevicePropertyV0::GetGyrRange, EDevicePropertyV0::SetGyrRange);

            case ZenImuProperty_GyrUseAutoCalibration:
                return SET_OR(EDevicePropertyV0::SetGyrUseAutoCalibration);

            case ZenImuProperty_MagAlignment:
                return GET_SET(EDevicePropertyV0::GetMagAlignment, EDevicePropertyV0::SetMagAlignment);

            case ZenImuProperty_MagBias:
                return GET_SET(EDevicePropertyV0::GetMagBias, EDevicePropertyV0::SetMagBias);

            case ZenImuProperty_MagRange:
                return GET_SET(EDevicePropertyV0::GetMagRange, EDevicePropertyV0::SetMagRange);

            case ZenImuProperty_MagReference:
                return GET_SET(EDevicePropertyV0::GetMagReference, EDevicePropertyV0::SetMagReference);

            case ZenImuProperty_MagHardIronOffset:
                return GET_SET(EDevicePropertyV0::GetMagHardIronOffset, EDevicePropertyV0::SetMagHardIronOffset);

            case ZenImuProperty_MagSoftIronMatrix:
                return GET_SET(EDevicePropertyV0::GetMagSoftIronMatrix, EDevicePropertyV0::SetMagSoftIronMatrix);

            /* CAN bus properties */
            case ZenImuProperty_CanChannelMode:
                return GET_SET(EDevicePropertyV0::GetCanConfiguration, EDevicePropertyV0::SetCanChannelMode);

            case ZenImuProperty_CanPointMode:
                return GET_SET(EDevicePropertyV0::GetCanConfiguration, EDevicePropertyV0::SetCanPointMode);

            case ZenImuProperty_CanStartId:
                return GET_SET(EDevicePropertyV0::GetCanConfiguration, EDevicePropertyV0::SetCanStartId);

            case ZenImuProperty_CanBaudrate:
                return GET_SET(EDevicePropertyV0::GetConfig, EDevicePropertyV0::SetCanBaudrate);

            case ZenImuProperty_CanMapping:
                return GET_SET(EDevicePropertyV0::GetCanMapping, EDevicePropertyV0::SetCanMapping);

            case ZenImuProperty_CanHeartbeat:
                return GET_SET(EDevicePropertyV0::GetCanHeartbeat, EDevicePropertyV0::SetCanHeartbeat);

            /* UART output properties */
            case ZenImuProperty_UartBaudRate:
                return GET_SET(EDevicePropertyV0::GetUartBaudrate, EDevicePropertyV0::SetUartBaudrate);

            case ZenImuProperty_UartFormat:
                return GET_SET(EDevicePropertyV0::GetUartBaudrate, EDevicePropertyV0::SetUartFormat);

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
