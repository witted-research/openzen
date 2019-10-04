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
                return SET_OR(EDevicePropertyV1::SetStreamFreq);

            case ZenImuProperty_FilterMode:
                return GET_SET(EDevicePropertyV1::GetFilterMode, EDevicePropertyV1::SetFilterMode);

            case ZenImuProperty_OrientationOffsetMode:
                return SET_OR(EDevicePropertyV1::SetOrientationOffsetMode);

            case ZenImuProperty_GyrRange:
                return GET_SET(EDevicePropertyV1::GetGyrRange, EDevicePropertyV1::SetGyrRange);

            case ZenImuProperty_GyrUseAutoCalibration:
                return GET_SET(EDevicePropertyV1::GetEnableGyrAutoCalibration, EDevicePropertyV1::SetEnableGyrAutoCalibration);

            case ZenImuProperty_GyrUseThreshold:
                return GET_SET(EDevicePropertyV1::GetGyrThreshold, EDevicePropertyV1::SetGyrThreshold);

            case ZenImuProperty_MagRange:
                return GET_SET(EDevicePropertyV1::GetMagRange, EDevicePropertyV1::SetMagRange);

            /* CAN bus properties */
            case ZenImuProperty_CanStartId:
                return GET_SET(EDevicePropertyV1::GetCanStartId, EDevicePropertyV1::SetCanStartId);

            case ZenImuProperty_CanBaudrate:
                return GET_SET(EDevicePropertyV1::GetCanBaudRate, EDevicePropertyV1::SetCanBaudRate);

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

        inline std::pair<ZenError, size_t> supportedSamplingRates(gsl::span<int32_t> buffer)
        {
            // this list is directly from the IG1 documentation
            constexpr std::array<int32_t, 7> supported{ 5, 10, 50, 100, 500 };

            if (static_cast<size_t>(buffer.size()) < supported.size())
                return std::make_pair(ZenError_BufferTooSmall, supported.size());

            if (buffer.data() == nullptr)
                return std::make_pair(ZenError_IsNull, supported.size());

            std::copy(supported.cbegin(), supported.cend(), buffer.begin());
            return std::make_pair(ZenError_None, supported.size());
        }

        constexpr uint32_t roundSamplingRate(int32_t value)
        {
            if (value <= 5)
                return 5;
            else if (value <= 10)
                return 10;
            else if (value <= 50)
                return 50;
            else if (value <= 100)
                return 100;
            else
                return 500;
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
            if (value <= 400)
                return 400;
            else if (value <= 1000)
                return 1000;
            else
                return 2000;
        }

        inline std::pair<ZenError, size_t> supportedGyrRanges(gsl::span<int32_t> buffer)
        {
            constexpr std::array<int32_t, 3> supported{ 400, 1000, 2000 };

            if (static_cast<size_t>(buffer.size()) < supported.size())
                return std::make_pair(ZenError_BufferTooSmall, supported.size());

            if (buffer.data() == nullptr)
                return std::make_pair(ZenError_IsNull, supported.size());

            std::copy(supported.cbegin(), supported.cend(), buffer.begin());
            return std::make_pair(ZenError_None, supported.size());
        }

        constexpr uint32_t mapMagRange(int32_t value)
        {
            if (value <= 2)
                return 2;
            else
                return 8;
        }

        inline std::pair<ZenError, size_t> supportedMagRanges(gsl::span<int32_t> buffer)
        {
            constexpr std::array<int32_t, 2> supported{ 2, 8 };

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
