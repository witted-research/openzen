#include "properties/LegacyImuProperties.h"

#include <math.h>

#include "SensorProperties.h"
#include "properties/ImuSensorPropertiesV0.h"
#include "utility/Finally.h"

namespace zen
{

    LegacyImuProperties::LegacyImuProperties(SyncedModbusCommunicator& communicator) noexcept
        : m_cache{}
        , m_communicator(communicator)
        , m_streaming(true)
    {}

    ZenError LegacyImuProperties::execute(ZenProperty_t command) noexcept
    {
        const auto commandV0 = imu::v0::mapCommand(command);
        if (imu::v0::supportsExecutingDeviceCommand(commandV0))
        {
            if (auto streaming = getBool(ZenImuProperty_StreamData))
            {
                if (*streaming)
                    if (auto error = setBool(ZenImuProperty_StreamData, false))
                        return error;

                auto guard = finally([&]() {
                    if (*streaming)
                        setBool(ZenImuProperty_StreamData, true);
                });

            }
            else
            {
                return streaming.error();
            }

            return m_communicator.sendAndWaitForAck(0, static_cast<DeviceProperty_t>(commandV0), 0, {});
        }

        return ZenError_UnknownProperty;
    }

    std::pair<ZenError, size_t> LegacyImuProperties::getArray(ZenProperty_t property, ZenPropertyType type, gsl::span<std::byte> buffer) noexcept
    {
        const auto propertyV0 = imu::v0::map(property, true);
        ZenPropertyType expectedType = imu::v0::supportsGettingArrayDeviceProperty(propertyV0);
        if (property == ZenImuProperty_AccSupportedRanges ||
            property == ZenImuProperty_GyrSupportedRanges ||
            property == ZenImuProperty_MagSupportedRanges)
            expectedType = ZenPropertyType_Int32;

        const DeviceProperty_t function = static_cast<DeviceProperty_t>(propertyV0);
        if (expectedType)
        {
            if (type != expectedType)
                return std::make_pair(ZenError_WrongDataType, buffer.size());

            if (auto streaming = getBool(ZenImuProperty_StreamData))
            {
                if (*streaming)
                    if (auto error = setBool(ZenImuProperty_StreamData, false))
                        return std::make_pair(error, buffer.size());

                auto guard = finally([&]() {
                    if (*streaming)
                        setBool(ZenImuProperty_StreamData, true);
                });

                switch (type)
                {
                case ZenPropertyType_Bool:
                    return m_communicator.sendAndWaitForArray(0, function, function, {}, gsl::make_span(reinterpret_cast<bool*>(buffer.data()), buffer.size()));

                case ZenPropertyType_Float:
                    return m_communicator.sendAndWaitForArray(0, function, function, {}, gsl::make_span(reinterpret_cast<float*>(buffer.data()), buffer.size()));

                case ZenPropertyType_Int32:
                    if (property == ZenImuProperty_AccSupportedRanges)
                        return imu::v0::supportedAccRanges(gsl::make_span(reinterpret_cast<int32_t* const>(buffer.data()), buffer.size()));
                    else if (property == ZenImuProperty_GyrSupportedRanges)
                        return imu::v0::supportedGyrRanges(gsl::make_span(reinterpret_cast<int32_t* const>(buffer.data()), buffer.size()));
                    else if (property == ZenImuProperty_MagSupportedRanges)
                        return imu::v0::supportedMagRanges(gsl::make_span(reinterpret_cast<int32_t* const>(buffer.data()), buffer.size()));
                    else
                        return m_communicator.sendAndWaitForArray(0, function, function, {}, gsl::make_span(reinterpret_cast<int32_t*>(buffer.data()), buffer.size()));

                default:
                    return std::make_pair(ZenError_WrongDataType, buffer.size());
                }
            }
            else
            {
                return std::make_pair(streaming.error(), buffer.size());
            }
        }

        return std::make_pair(ZenError_UnknownProperty, buffer.size());
    }

    nonstd::expected<bool, ZenError> LegacyImuProperties::getBool(ZenProperty_t property) noexcept
    {
        if (property == ZenImuProperty_GyrUseThreshold)
            return m_cache.gyrUseThreshold;
        else if (property == ZenImuProperty_StreamData)
            return m_streaming;
        else if (property == ZenImuProperty_OutputLowPrecision)
            return getConfigDataFlag(22);
        else if (property == ZenImuProperty_OutputLinearAcc)
            return getConfigDataFlag(21);
        else if (property == ZenImuProperty_OutputAltitude)
            return getConfigDataFlag(19);
        else if (property == ZenImuProperty_OutputQuat)
            return getConfigDataFlag(18);
        else if (property == ZenImuProperty_OutputEuler)
            return getConfigDataFlag(17);
        else if (property == ZenImuProperty_OutputAngularVel)
            return getConfigDataFlag(16);
        else if (property == ZenImuProperty_OutputHeaveMotion)
            return getConfigDataFlag(14);
        else if (property == ZenImuProperty_OutputTemperature)
            return getConfigDataFlag(13);
        else if (property == ZenImuProperty_OutputRawGyr)
            return getConfigDataFlag(12);
        else if (property == ZenImuProperty_OutputRawAcc)
            return getConfigDataFlag(11);
        else if (property == ZenImuProperty_OutputRawMag)
            return getConfigDataFlag(10);
        else if (property == ZenImuProperty_OutputPressure)
            return getConfigDataFlag(9);

        return nonstd::make_unexpected(ZenError_UnknownProperty);

    }

    nonstd::expected<float, ZenError> LegacyImuProperties::getFloat(ZenProperty_t property) noexcept
    {
        const auto propertyV0 = imu::v0::map(property, true);
        if (propertyV0 == EDevicePropertyV0::GetCentricCompensationRate)
        {
            if (auto streaming = getBool(ZenImuProperty_StreamData))
            {
                if (*streaming)
                    if (auto error = setBool(ZenImuProperty_StreamData, false))
                        return nonstd::make_unexpected(error);

                auto guard = finally([&]() {
                    if (*streaming)
                        setBool(ZenImuProperty_StreamData, true);
                });

                if (auto result = m_communicator.sendAndWaitForResult<uint32_t>(0, static_cast<DeviceProperty_t>(propertyV0), static_cast<ZenProperty_t>(propertyV0), {}))
                    return *result > 0 ? 1.f : 0.f;
                else
                    return nonstd::make_unexpected(result.error());
            }
            else
            {
                return nonstd::make_unexpected(streaming.error());
            }
        }
        else if (propertyV0 == EDevicePropertyV0::GetLinearCompensationRate)
        {
            if (auto streaming = getBool(ZenImuProperty_StreamData))
            {
                if (*streaming)
                    if (auto error = setBool(ZenImuProperty_StreamData, false))
                        return nonstd::make_unexpected(error);

                auto guard = finally([&]() {
                    if (*streaming)
                        setBool(ZenImuProperty_StreamData, true);
                });

                if (auto result = m_communicator.sendAndWaitForResult<uint32_t>(0, static_cast<DeviceProperty_t>(propertyV0), static_cast<ZenProperty_t>(propertyV0), {}))
                    return static_cast<float>(*result);
                else
                    return nonstd::make_unexpected(result.error());
            }
            else
            {
                return nonstd::make_unexpected(streaming.error());
            }

        }
        else if (imu::v0::supportsGettingFloatDeviceProperty(propertyV0))
        {
            const bool streaming = m_streaming;
            if (streaming)
                if (auto error = setBool(ZenImuProperty_StreamData, false))
                    return nonstd::make_unexpected(error);

            auto guard = finally([=]() {
                if (streaming)
                    setBool(ZenImuProperty_StreamData, true);
            });

            return m_communicator.sendAndWaitForResult<float>(0, static_cast<DeviceProperty_t>(propertyV0), static_cast<ZenProperty_t>(propertyV0), {});
        }

        return nonstd::make_unexpected(ZenError_UnknownProperty);
    }

    nonstd::expected<int32_t, ZenError> LegacyImuProperties::getInt32(ZenProperty_t property) noexcept
    {
        const auto propertyV0 = imu::v0::map(property, true);
        if (imu::v0::supportsGettingInt32DeviceProperty(propertyV0))
        {
            if (auto streaming = getBool(ZenImuProperty_StreamData))
            {
                if (*streaming)
                    if (auto error = setBool(ZenImuProperty_StreamData, false))
                        return nonstd::make_unexpected(error);

                auto guard = finally([&]() {
                    if (*streaming)
                        setBool(ZenImuProperty_StreamData, true);
                });

                // Communication protocol only supports uint32_t
                if (auto result = m_communicator.sendAndWaitForResult<uint32_t>(0, static_cast<DeviceProperty_t>(propertyV0), static_cast<ZenProperty_t>(propertyV0), {}))
                    return static_cast<int32_t>(*result);
                else
                    return result.error();
            }
            else
            {
                return streaming.error();
            }
        }

        return nonstd::make_unexpected(ZenError_UnknownProperty);
    }

    nonstd::expected<ZenMatrix3x3f, ZenError> LegacyImuProperties::getMatrix33(ZenProperty_t property) noexcept
    {
        const auto propertyV0 = imu::v0::map(property, true);
        if (imu::v0::supportsGettingMatrix33DeviceProperty(propertyV0))
        {
            if (auto streaming = getBool(ZenImuProperty_StreamData))
            {
                if (*streaming)
                    if (auto error = setBool(ZenImuProperty_StreamData, false))
                        return nonstd::make_unexpected(error);

                auto guard = finally([&]() {
                    if (*streaming)
                        setBool(ZenImuProperty_StreamData, true);
                });

                ZenMatrix3x3f matrix;
                const auto[error, size] = m_communicator.sendAndWaitForArray<float>(0, static_cast<DeviceProperty_t>(propertyV0), static_cast<ZenProperty_t>(propertyV0), {}, gsl::make_span(matrix.data, 9));
                if (error)
                    return nonstd::make_unexpected(error);
                
                return matrix;
            }
            else
            {
                return nonstd::make_unexpected(streaming.error());
            }
        }

        return nonstd::make_unexpected(ZenError_UnknownProperty);
    }

    std::pair<ZenError, size_t> LegacyImuProperties::getString(ZenProperty_t property, gsl::span<char> buffer) noexcept
    {
        if (property == ZenImuProperty_SupportedFilterModes)
            return imu::v0::supportedFilterModes(buffer);

        return std::make_pair(ZenError_UnknownProperty, buffer.size());
    }

    ZenError LegacyImuProperties::setArray(ZenProperty_t property, ZenPropertyType type, gsl::span<const std::byte> buffer) noexcept
    {
        const auto propertyV0 = imu::v0::map(property, false);
        const ZenPropertyType expectedType = imu::v0::supportsSettingArrayDeviceProperty(propertyV0);
        const DeviceProperty_t deviceProperty = static_cast<DeviceProperty_t>(propertyV0);

        if (expectedType)
        {
            if (type != expectedType)
                return ZenError_WrongDataType;

            if (auto streaming = getBool(ZenImuProperty_StreamData))
            {
                if (*streaming)
                    if (auto error = setBool(ZenImuProperty_StreamData, false))
                        return error;

                auto guard = finally([&]() {
                    if (*streaming)
                        setBool(ZenImuProperty_StreamData, true);
                });

            }
            else
            {
                return streaming.error();
            }

            const size_t typeSize = sizeOfPropertyType(type);
            return m_communicator.sendAndWaitForAck(0, deviceProperty, deviceProperty, gsl::make_span(buffer.data(), typeSize * buffer.size()));
        }

        return ZenError_UnknownProperty;
    }

    ZenError LegacyImuProperties::setBool(ZenProperty_t property, bool value) noexcept
    {
        if (property == ZenImuProperty_StreamData)
        {
            if (m_streaming != value)
            {
                uint32_t temp;
                const auto propertyV0 = value ? EDevicePropertyV0::SetStreamMode : EDevicePropertyV0::SetCommandMode;
                if (auto error = m_communicator.sendAndWaitForAck(0, static_cast<DeviceProperty_t>(propertyV0), static_cast<ZenProperty_t>(propertyV0), gsl::make_span(reinterpret_cast<const std::byte*>(&temp), sizeof(temp))))
                    return error;

                m_streaming = value;
            }
            return ZenError_None;
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
                if (auto streaming = getBool(ZenImuProperty_StreamData))
                {
                    if (*streaming)
                        if (auto error = setBool(ZenImuProperty_StreamData, false))
                            return error;

                    auto guard = finally([&]() {
                        if (*streaming)
                            setBool(ZenImuProperty_StreamData, true);
                    });

                }
                else
                {
                    return streaming.error();
                }

                uint32_t iValue = value ? 1 : 0;
                if (auto error = m_communicator.sendAndWaitForAck(0, static_cast<DeviceProperty_t>(propertyV0), static_cast<ZenProperty_t>(propertyV0), gsl::span(reinterpret_cast<const std::byte*>(&iValue), sizeof(iValue))))
                    return error;

                m_cache.gyrAutoCalibration = value;
                return ZenError_None;
            }
            else if (propertyV0 == EDevicePropertyV0::SetGyrUseThreshold)
            {
                if (auto streaming = getBool(ZenImuProperty_StreamData))
                {
                    if (*streaming)
                        if (auto error = setBool(ZenImuProperty_StreamData, false))
                            return error;

                    auto guard = finally([&]() {
                        if (*streaming)
                            setBool(ZenImuProperty_StreamData, true);
                    });

                }
                else
                {
                    return streaming.error();
                }

                uint32_t iValue = value ? 1 : 0;
                if (auto error = m_communicator.sendAndWaitForAck(0, static_cast<DeviceProperty_t>(propertyV0), static_cast<ZenProperty_t>(propertyV0), gsl::span(reinterpret_cast<const std::byte*>(&iValue), sizeof(iValue))))
                    return error;

                m_cache.gyrUseThreshold = value;
                return ZenError_None;
            }
        }

        return ZenError_UnknownProperty;
    }

    ZenError LegacyImuProperties::setFloat(ZenProperty_t property, float value) noexcept
    {
        const auto propertyV0 = imu::v0::map(property, false);
        if (propertyV0 == EDevicePropertyV0::SetCentricCompensationRate)
        {
            if (auto streaming = getBool(ZenImuProperty_StreamData))
            {
                if (*streaming)
                    if (auto error = setBool(ZenImuProperty_StreamData, false))
                        return error;

                auto guard = finally([&]() {
                    if (*streaming)
                        setBool(ZenImuProperty_StreamData, true);
                });

            }
            else
            {
                return streaming.error();
            }

            constexpr float eps = std::numeric_limits<float>::epsilon();
            uint32_t iValue = (-eps <= value && value <= eps) ? 0 : 1; // Account for imprecision of float
            return m_communicator.sendAndWaitForAck(0, static_cast<DeviceProperty_t>(propertyV0), static_cast<ZenProperty_t>(propertyV0), gsl::make_span(reinterpret_cast<const std::byte*>(&iValue), sizeof(iValue)));
        }
        else if (propertyV0 == EDevicePropertyV0::SetLinearCompensationRate)
        {
            if (auto streaming = getBool(ZenImuProperty_StreamData))
            {
                if (*streaming)
                    if (auto error = setBool(ZenImuProperty_StreamData, false))
                        return error;

                auto guard = finally([&]() {
                    if (*streaming)
                        setBool(ZenImuProperty_StreamData, true);
                });

            }
            else
            {
                return streaming.error();
            }

            uint32_t iValue = lroundf(value);
            return m_communicator.sendAndWaitForAck(0, static_cast<DeviceProperty_t>(propertyV0), static_cast<ZenProperty_t>(propertyV0), gsl::make_span(reinterpret_cast<const std::byte*>(&iValue), sizeof(iValue)));
        }
        else if (imu::v0::supportsSettingFloatDeviceProperty(propertyV0))
        {
            if (auto streaming = getBool(ZenImuProperty_StreamData))
            {
                if (*streaming)
                    if (auto error = setBool(ZenImuProperty_StreamData, false))
                        return error;

                auto guard = finally([&]() {
                    if (*streaming)
                        setBool(ZenImuProperty_StreamData, true);
                });

            }
            else
            {
                return streaming.error();
            }

            return m_communicator.sendAndWaitForAck(0, static_cast<DeviceProperty_t>(propertyV0), static_cast<ZenProperty_t>(propertyV0), gsl::make_span(reinterpret_cast<const std::byte*>(&value), sizeof(value)));
        }

        return ZenError_UnknownProperty;
    }

    ZenError LegacyImuProperties::setInt32(ZenProperty_t property, int32_t value) noexcept
    {
        const auto propertyV0 = imu::v0::map(property, false);
        if (imu::v0::supportsSettingInt32DeviceProperty(propertyV0))
        {
            if (auto streaming = getBool(ZenImuProperty_StreamData))
            {
                if (*streaming)
                    if (auto error = setBool(ZenImuProperty_StreamData, false))
                        return error;

                auto guard = finally([&]() {
                    if (*streaming)
                        setBool(ZenImuProperty_StreamData, true);
                });

            }
            else
            {
                return streaming.error();
            }

            // Communication protocol only supports uint32_t
            uint32_t uiValue;
            if (property == ZenImuProperty_AccRange)
                uiValue = imu::v0::mapAccRange(value);
            else if (property == ZenImuProperty_GyrRange)
                uiValue = imu::v0::mapGyrRange(value);
            else if (property == ZenImuProperty_MagRange)
                uiValue = imu::v0::mapMagRange(value);
            else
                uiValue = static_cast<uint32_t>(value);

            return m_communicator.sendAndWaitForAck(0, static_cast<DeviceProperty_t>(propertyV0), static_cast<ZenProperty_t>(propertyV0), gsl::make_span(reinterpret_cast<const std::byte*>(&uiValue), sizeof(uiValue)));
        }

        return ZenError_UnknownProperty;
    }

    ZenError LegacyImuProperties::setMatrix33(ZenProperty_t property, const ZenMatrix3x3f& value) noexcept
    {
        const auto propertyV0 = imu::v0::map(property, false);
        if (imu::v0::supportsSettingMatrix33DeviceProperty(propertyV0))
        {
            if (auto streaming = getBool(ZenImuProperty_StreamData))
            {
                if (*streaming)
                    if (auto error = setBool(ZenImuProperty_StreamData, false))
                        return error;

                auto guard = finally([&]() {
                    if (*streaming)
                        setBool(ZenImuProperty_StreamData, true);
                });

            }
            else
            {
                return streaming.error();
            }

            return m_communicator.sendAndWaitForAck(0, static_cast<DeviceProperty_t>(propertyV0), static_cast<ZenProperty_t>(propertyV0), gsl::make_span(reinterpret_cast<const std::byte*>(value.data), 9 * sizeof(float)));
        }

        return ZenError_UnknownProperty;
    }

    bool LegacyImuProperties::isArray(ZenProperty_t property) const noexcept
    {
        switch (property)
        {
        case ZenImuProperty_SupportedFilterModes:
        case ZenImuProperty_AccBias:
        case ZenImuProperty_AccSupportedRanges:
        case ZenImuProperty_GyrBias:
        case ZenImuProperty_GyrSupportedRanges:
        case ZenImuProperty_MagBias:
        case ZenImuProperty_MagSupportedRanges:
        case ZenImuProperty_MagReference:
        case ZenImuProperty_MagHardIronOffset:
            return true;

        default:
            return false;
        }
    }

    bool LegacyImuProperties::isConstant(ZenProperty_t property) const noexcept
    {
        switch (property)
        {
        case ZenImuProperty_SupportedFilterModes:
        case ZenImuProperty_AccSupportedRanges:
        case ZenImuProperty_GyrSupportedRanges:
        case ZenImuProperty_MagSupportedRanges:
            return true;

        default:
            return false;
        }
    }

    bool LegacyImuProperties::isExecutable(ZenProperty_t property) const noexcept
    {
        switch (property)
        {
        case ZenImuProperty_PollSensorData:
        case ZenImuProperty_CalibrateGyro:
        case ZenImuProperty_ResetOrientationOffset:
            return true;

        default:
            return false;
        }
    }

    ZenPropertyType LegacyImuProperties::type(ZenProperty_t property) const noexcept
    {
        switch (property)
        {
        case ZenImuProperty_StreamData:
        case ZenImuProperty_GyrUseAutoCalibration:
        case ZenImuProperty_GyrUseThreshold:
        case ZenImuProperty_OutputLowPrecision:
        case ZenImuProperty_OutputRawAcc:
        case ZenImuProperty_OutputRawGyr:
        case ZenImuProperty_OutputRawMag:
        case ZenImuProperty_OutputEuler:
        case ZenImuProperty_OutputQuat:
        case ZenImuProperty_OutputAngularVel:
        case ZenImuProperty_OutputLinearAcc:
        case ZenImuProperty_OutputHeaveMotion:
        case ZenImuProperty_OutputAltitude:
        case ZenImuProperty_OutputPressure:
        case ZenImuProperty_OutputTemperature:
            return ZenPropertyType_Bool;

        case ZenImuProperty_CentricCompensationRate:
        case ZenImuProperty_LinearCompensationRate:
        case ZenImuProperty_FieldRadius:
        case ZenImuProperty_AccBias:
        case ZenImuProperty_GyrBias:
        case ZenImuProperty_MagBias:
        case ZenImuProperty_MagReference:
        case ZenImuProperty_MagHardIronOffset:
            return ZenPropertyType_Float;

        case ZenImuProperty_FilterMode:
        case ZenImuProperty_FilterPreset:
        case ZenImuProperty_OrientationOffsetMode:
        case ZenImuProperty_AccRange:
        case ZenImuProperty_AccSupportedRanges:
        case ZenImuProperty_GyrRange:
        case ZenImuProperty_GyrSupportedRanges:
        case ZenImuProperty_MagRange:
        case ZenImuProperty_MagSupportedRanges:
            return ZenPropertyType_Int32;

        case ZenImuProperty_AccAlignment:
        case ZenImuProperty_GyrAlignment:
        case ZenImuProperty_MagAlignment:
        case ZenImuProperty_MagSoftIronMatrix:
            return ZenPropertyType_Matrix;

        case ZenImuProperty_SupportedFilterModes:
            return ZenPropertyType_String;

        default:
            return ZenPropertyType_Invalid;
        }
    }

    bool LegacyImuProperties::getConfigDataFlag(unsigned int index) noexcept
    {
        return (m_cache.outputDataBitset & (1 << index)) != 0;
    }

    ZenError LegacyImuProperties::setOutputDataFlag(unsigned int index, bool value) noexcept
    {
        const bool streaming = m_streaming;
        if (streaming)
            if (auto error = setBool(ZenImuProperty_StreamData, false))
                return error;

        auto guard = finally([=]() {
            if (streaming)
                setBool(ZenImuProperty_StreamData, true);
        });

        uint32_t newBitset;
        if (value)
            newBitset = m_cache.outputDataBitset | (1 << index);
        else
            newBitset = m_cache.outputDataBitset & ~(1 << index);

        if (auto error = m_communicator.sendAndWaitForAck(0, static_cast<DeviceProperty_t>(EDevicePropertyV0::SetTransmitData), static_cast<ZenProperty_t>(EDevicePropertyV0::SetTransmitData), gsl::make_span(reinterpret_cast<const std::byte*>(&newBitset), sizeof(newBitset))))
            return error;

        m_cache.outputDataBitset = newBitset;
        return ZenError_None;
    }

    ZenError LegacyImuProperties::setPrecisionDataFlag(bool value) noexcept
    {
        const bool streaming = m_streaming;
        if (streaming)
            if (auto error = setBool(ZenImuProperty_StreamData, false))
                return error;

        auto guard = finally([=]() {
            if (streaming)
                setBool(ZenImuProperty_StreamData, true);
        });

        uint32_t newBitset;
        if (value)
            newBitset = m_cache.outputDataBitset | (1 << 22);
        else
            newBitset = m_cache.outputDataBitset & ~(1 << 22);

        const uint32_t iValue = value ? 1 : 0;
        if (auto error = m_communicator.sendAndWaitForAck(0, static_cast<DeviceProperty_t>(EDevicePropertyV0::SetDataMode), static_cast<ZenProperty_t>(EDevicePropertyV0::SetDataMode), gsl::make_span(reinterpret_cast<const std::byte*>(&iValue), sizeof(iValue))))
            return error;

        m_cache.outputDataBitset = newBitset;
        return ZenError_None;
    }
}
