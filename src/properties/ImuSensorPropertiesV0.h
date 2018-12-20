#ifndef ZEN_PROPERTIES_IMUSENSORPROPERTIESV0_H_
#define ZEN_PROPERTIES_IMUSENSORPROPERTIESV0_H_

#include <array>
#include <cstring>
#include <optional>

#include "InternalTypes.h"
#include "ZenTypes.h"

#define GET_OR(x) isGetter ? (x) : EDevicePropertyV0::Ack
#define SET_OR(x) isGetter ? EDevicePropertyV0::Ack : (x)
#define GET_SET(x, y) isGetter ? (x) : (y)

namespace zen
{
    namespace imu::v0
    {
        namespace internal
        {
            constexpr std::optional<EDevicePropertyInternal> map(uint8_t function)
            {
                if (function == static_cast<uint8_t>(EDevicePropertyInternal::Config))
                    return EDevicePropertyInternal::Config;

                return {};
            }
        }

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

            case ZenImuProperty_GyrUseThreshold:
                return SET_OR(EDevicePropertyV0::SetGyrUseThreshold);

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

            default:
                return EDevicePropertyV0::Ack;
            }
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

        inline ZenError supportedAccRanges(int32_t* const buffer, size_t& bufferSize)
        {
            constexpr std::array<int32_t, 4> supported{ 2, 4, 8, 16 };

            if (bufferSize < supported.size())
            {
                bufferSize = supported.size();
                return ZenError_BufferTooSmall;
            }

            if (buffer == nullptr)
                return ZenError_IsNull;

            std::copy(supported.cbegin(), supported.cend(), buffer);
            return ZenError_None;
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

        inline ZenError supportedGyrRanges(int32_t* const buffer, size_t& bufferSize)
        {
            constexpr std::array<int32_t, 5> supported{ 125, 245, 500, 1000, 2000 };

            if (bufferSize < supported.size())
            {
                bufferSize = supported.size();
                return ZenError_BufferTooSmall;
            }

            if (buffer == nullptr)
                return ZenError_IsNull;

            std::copy(supported.cbegin(), supported.cend(), buffer);
            return ZenError_None;
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

        inline ZenError supportedMagRanges(int32_t* const buffer, size_t& bufferSize)
        {
            constexpr std::array<int32_t, 4> supported{ 4, 8, 12, 16 };

            if (bufferSize < supported.size())
            {
                bufferSize = supported.size();
                return ZenError_BufferTooSmall;
            }

            if (buffer == nullptr)
                return ZenError_IsNull;

            std::copy(supported.cbegin(), supported.cend(), buffer);
            return ZenError_None;
        }

        constexpr ZenError supportedFilterModes(char* const buffer, size_t& bufferSize)
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

            if (bufferSize < sizeof(json))
            {
                bufferSize = sizeof(json);
                return ZenError_BufferTooSmall;
            }

            if (buffer == nullptr)
                return ZenError_IsNull;

            std::memcpy(buffer, json, sizeof(json));
            return ZenError_None;
        }

        constexpr bool supportsExecutingDeviceCommand(EDevicePropertyV0 command)
        {
            switch (command)
            {
            case EDevicePropertyV0::GetRawSensorData:
            case EDevicePropertyV0::StartGyroCalibration:
            case EDevicePropertyV0::ResetOrientationOffset:
                return true;

            default:
                return false;
            }
        }

        constexpr ZenPropertyType supportsGettingArrayDeviceProperty(EDevicePropertyV0 property)
        {
            switch (property)
            {
            case EDevicePropertyV0::GetAccBias:
            case EDevicePropertyV0::GetGyrBias:
            case EDevicePropertyV0::GetMagBias:
            case EDevicePropertyV0::GetMagHardIronOffset:
            case EDevicePropertyV0::GetMagReference:
                return ZenPropertyType_Float;

            default:
                return ZenPropertyType_Invalid;
            }
        }

        constexpr bool supportsGettingFloatDeviceProperty(EDevicePropertyV0 property)
        {
            switch (property)
            {
            case EDevicePropertyV0::GetCentricCompensationRate:
            case EDevicePropertyV0::GetFieldRadius:
                return true;

            default:
                return false;
            }
        }

        constexpr bool supportsGettingInt32DeviceProperty(EDevicePropertyV0 property)
        {
            switch (property)
            {
            case EDevicePropertyV0::GetAccRange:
            case EDevicePropertyV0::GetGyrRange:
            case EDevicePropertyV0::GetMagRange:
            case EDevicePropertyV0::GetFilterMode:
            case EDevicePropertyV0::GetFilterPreset:
            case EDevicePropertyV0::GetLinearCompensationRate:
                return true;

            default:
                return false;
            }
        }

        constexpr bool supportsGettingMatrix33DeviceProperty(EDevicePropertyV0 property)
        {
            switch (property)
            {
            case EDevicePropertyV0::GetAccAlignment:
            case EDevicePropertyV0::GetGyrAlignment:
            case EDevicePropertyV0::GetMagAlignment:
            case EDevicePropertyV0::GetMagSoftIronMatrix:
                return true;

            default:
                return false;
            }
        }


        constexpr ZenPropertyType supportsSettingArrayDeviceProperty(EDevicePropertyV0 property)
        {
            switch (property)
            {
            case EDevicePropertyV0::SetAccBias:
            case EDevicePropertyV0::SetGyrBias:
            case EDevicePropertyV0::SetMagBias:
            case EDevicePropertyV0::SetMagHardIronOffset:
            case EDevicePropertyV0::SetMagReference:
                return ZenPropertyType_Float;

            default:
                return ZenPropertyType_Invalid;
            }
        }

        constexpr bool supportsSettingFloatDeviceProperty(EDevicePropertyV0 property)
        {
            switch (property)
            {
            case EDevicePropertyV0::SetCentricCompensationRate:
            case EDevicePropertyV0::SetFieldRadius:
                return true;

            default:
                return false;
            }
        }

        constexpr bool supportsSettingInt32DeviceProperty(EDevicePropertyV0 property)
        {
            switch (property)
            {
            case EDevicePropertyV0::SetLinearCompensationRate:
            case EDevicePropertyV0::SetFilterMode:
            case EDevicePropertyV0::SetFilterPreset:
            case EDevicePropertyV0::SetOrientationOffsetMode:
            case EDevicePropertyV0::SetAccRange:
            case EDevicePropertyV0::SetGyrRange:
            case EDevicePropertyV0::SetMagRange:
                return true;

            default:
                return false;
            }
        }

        constexpr bool supportsSettingMatrix33DeviceProperty(EDevicePropertyV0 property)
        {
            switch (property)
            {
            case EDevicePropertyV0::SetAccAlignment:
            case EDevicePropertyV0::SetGyrAlignment:
            case EDevicePropertyV0::SetMagAlignment:
            case EDevicePropertyV0::SetMagSoftIronMatrix:
                return true;

            default:
                return false;
            }
        }
    }
}

#endif
