#include "properties/LegacyImuProperties.h"

#include <math.h>

#include "SensorProperties.h"
#include "properties/ImuSensorPropertiesV0.h"
#include "utility/Finally.h"

namespace zen
{
    namespace
    {
        template <ZenProperty_t type>
        struct OutputDataFlag
        {};

        template <> struct OutputDataFlag<ZenImuProperty_OutputLowPrecision>
        {
            using index = std::integral_constant<unsigned int, 22>;
        };

        template <> struct OutputDataFlag<ZenImuProperty_OutputLinearAcc>
        {
            using index = std::integral_constant<unsigned int, 21>;
        };

        template <> struct OutputDataFlag<ZenImuProperty_OutputAltitude>
        {
            using index = std::integral_constant<unsigned int, 19>;
        };

        template <> struct OutputDataFlag<ZenImuProperty_OutputQuat>
        {
            using index = std::integral_constant<unsigned int, 18>;
        };

        template <> struct OutputDataFlag<ZenImuProperty_OutputEuler>
        {
            using index = std::integral_constant<unsigned int, 17>;
        };

        template <> struct OutputDataFlag<ZenImuProperty_OutputAngularVel>
        {
            using index = std::integral_constant<unsigned int, 16>;
        };

        template <> struct OutputDataFlag<ZenImuProperty_OutputHeaveMotion>
        {
            using index = std::integral_constant<unsigned int, 14>;
        };

        template <> struct OutputDataFlag<ZenImuProperty_OutputTemperature>
        {
            using index = std::integral_constant<unsigned int, 13>;
        };

        template <> struct OutputDataFlag<ZenImuProperty_OutputRawGyr>
        {
            using index = std::integral_constant<unsigned int, 12>;
        };

        template <> struct OutputDataFlag<ZenImuProperty_OutputRawAcc>
        {
            using index = std::integral_constant<unsigned int, 11>;
        };

        template <> struct OutputDataFlag<ZenImuProperty_OutputRawMag>
        {
            using index = std::integral_constant<unsigned int, 10>;
        };

        template <> struct OutputDataFlag<ZenImuProperty_OutputPressure>
        {
            using index = std::integral_constant<unsigned int, 9>;
        };

        template <ZenProperty_t property>
        constexpr bool getOutputDataFlag(std::atomic_uint32_t& outputDataBitset) noexcept
        {
            return (outputDataBitset & (1 << OutputDataFlag<property>::index())) != 0;
        }

        template <ZenProperty_t property>
        ZenError setOutputDataFlag(LegacyImuProperties& self, SyncedModbusCommunicator& communicator, std::atomic_uint32_t& outputDataBitset, std::function<void(ZenProperty_t,SensorPropertyValue)> notifyPropertyChange, bool streaming, bool value) noexcept
        {
            if (streaming)
                if (auto error = self.setBool(ZenImuProperty_StreamData, false))
                    return error;

            auto guard = finally([&self, streaming]() {
                if (streaming)
                    self.setBool(ZenImuProperty_StreamData, true);
                });

            uint32_t newBitset;
            if (value)
                newBitset = outputDataBitset | (1 << OutputDataFlag<property>::index());
            else
                newBitset = outputDataBitset & ~(1 << OutputDataFlag<property>::index());

            if (auto error = communicator.sendAndWaitForAck(0, static_cast<DeviceProperty_t>(EDevicePropertyV0::SetTransmitData), static_cast<ZenProperty_t>(EDevicePropertyV0::SetTransmitData), gsl::make_span(reinterpret_cast<const std::byte*>(&newBitset), sizeof(newBitset))))
                return error;

            outputDataBitset = newBitset;
            notifyPropertyChange(property, value);
            return ZenError_None;
        }
    }

    LegacyImuProperties::LegacyImuProperties(SyncedModbusCommunicator& communicator) noexcept
        : m_cache{}
        , m_communicator(communicator)
        , m_streaming(true)
    {
        m_cache.samplingRate = 200;
    }

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

                return m_communicator.sendAndWaitForAck(0, static_cast<DeviceProperty_t>(commandV0), 0, {});
            }
            else
            {
                return streaming.error();
            }
        }

        return ZenError_UnknownProperty;
    }

    std::pair<ZenError, size_t> LegacyImuProperties::getArray(ZenProperty_t property, ZenPropertyType type, gsl::span<std::byte> buffer) noexcept
    {
        const auto propertyV0 = imu::v0::map(property, true);
        ZenPropertyType expectedType = imu::v0::supportsGettingArrayDeviceProperty(propertyV0);
        if (property == ZenImuProperty_SupportedSamplingRates ||
            property == ZenImuProperty_AccSupportedRanges ||
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
                    if (property == ZenImuProperty_SupportedSamplingRates)
                        return imu::v0::supportedSamplingRates(gsl::make_span(reinterpret_cast<int32_t* const>(buffer.data()), buffer.size()));
                    else if (property == ZenImuProperty_AccSupportedRanges)
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
        if (property == ZenImuProperty_GyrUseAutoCalibration)
            return m_cache.gyrAutoCalibration;
        else if (property == ZenImuProperty_GyrUseThreshold)
            return m_cache.gyrUseThreshold;
        else if (property == ZenImuProperty_StreamData)
            return m_streaming;
        else if (property == ZenImuProperty_OutputLowPrecision)
            return getOutputDataFlag<ZenImuProperty_OutputLowPrecision>(m_cache.outputDataBitset);
        else if (property == ZenImuProperty_OutputLinearAcc)
            return getOutputDataFlag<ZenImuProperty_OutputLinearAcc>(m_cache.outputDataBitset);
        else if (property == ZenImuProperty_OutputAltitude)
            return getOutputDataFlag<ZenImuProperty_OutputAltitude>(m_cache.outputDataBitset);
        else if (property == ZenImuProperty_OutputQuat)
            return getOutputDataFlag<ZenImuProperty_OutputQuat>(m_cache.outputDataBitset);
        else if (property == ZenImuProperty_OutputEuler)
            return getOutputDataFlag<ZenImuProperty_OutputEuler>(m_cache.outputDataBitset);
        else if (property == ZenImuProperty_OutputAngularVel)
            return getOutputDataFlag<ZenImuProperty_OutputAngularVel>(m_cache.outputDataBitset);
        else if (property == ZenImuProperty_OutputHeaveMotion)
            return getOutputDataFlag<ZenImuProperty_OutputHeaveMotion>(m_cache.outputDataBitset);
        else if (property == ZenImuProperty_OutputTemperature)
            return getOutputDataFlag<ZenImuProperty_OutputTemperature>(m_cache.outputDataBitset);
        else if (property == ZenImuProperty_OutputRawGyr)
            return getOutputDataFlag<ZenImuProperty_OutputRawGyr>(m_cache.outputDataBitset);
        else if (property == ZenImuProperty_OutputRawAcc)
            return getOutputDataFlag<ZenImuProperty_OutputRawAcc>(m_cache.outputDataBitset);
        else if (property == ZenImuProperty_OutputRawMag)
            return getOutputDataFlag<ZenImuProperty_OutputRawMag>(m_cache.outputDataBitset);
        else if (property == ZenImuProperty_OutputPressure)
            return getOutputDataFlag<ZenImuProperty_OutputPressure>(m_cache.outputDataBitset);

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
        if (property == ZenImuProperty_SamplingRate)
        {
            return m_cache.samplingRate;
        }
        else
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

                const size_t typeSize = sizeOfPropertyType(type);
                if (auto error = m_communicator.sendAndWaitForAck(0, deviceProperty, deviceProperty, gsl::make_span(buffer.data(), typeSize * buffer.size())))
                    return error;

                notifyPropertyChange(property, buffer);
                return ZenError_None;
            }
            else
            {
                return streaming.error();
            }
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
                notifyPropertyChange(property, value);
            }
            return ZenError_None;
        }
        else if (property == ZenImuProperty_OutputLowPrecision)
            return setPrecisionDataFlag(value);
        else if (property == ZenImuProperty_OutputLinearAcc)
            return setOutputDataFlag<ZenImuProperty_OutputLinearAcc>(*this, m_communicator, m_cache.outputDataBitset, [=](ZenProperty_t property, SensorPropertyValue value) { notifyPropertyChange(property, value); }, m_streaming, value);
        else if (property == ZenImuProperty_OutputAltitude)
            return setOutputDataFlag<ZenImuProperty_OutputAltitude>(*this, m_communicator, m_cache.outputDataBitset, [=](ZenProperty_t property, SensorPropertyValue value) { notifyPropertyChange(property, value); }, m_streaming, value);
        else if (property == ZenImuProperty_OutputQuat)
            return setOutputDataFlag<ZenImuProperty_OutputQuat>(*this, m_communicator, m_cache.outputDataBitset, [=](ZenProperty_t property, SensorPropertyValue value) { notifyPropertyChange(property, value); }, m_streaming, value);
        else if (property == ZenImuProperty_OutputEuler)
            return setOutputDataFlag<ZenImuProperty_OutputEuler>(*this, m_communicator, m_cache.outputDataBitset, [=](ZenProperty_t property, SensorPropertyValue value) { notifyPropertyChange(property, value); }, m_streaming, value);
        else if (property == ZenImuProperty_OutputAngularVel)
            return setOutputDataFlag<ZenImuProperty_OutputAngularVel>(*this, m_communicator, m_cache.outputDataBitset, [=](ZenProperty_t property, SensorPropertyValue value) { notifyPropertyChange(property, value); }, m_streaming, value);
        else if (property == ZenImuProperty_OutputHeaveMotion)
            return setOutputDataFlag<ZenImuProperty_OutputHeaveMotion>(*this, m_communicator, m_cache.outputDataBitset, [=](ZenProperty_t property, SensorPropertyValue value) { notifyPropertyChange(property, value); }, m_streaming, value);
        else if (property == ZenImuProperty_OutputTemperature)
            return setOutputDataFlag<ZenImuProperty_OutputTemperature>(*this, m_communicator, m_cache.outputDataBitset, [=](ZenProperty_t property, SensorPropertyValue value) { notifyPropertyChange(property, value); }, m_streaming, value);
        else if (property == ZenImuProperty_OutputRawGyr)
            return setOutputDataFlag<ZenImuProperty_OutputRawGyr>(*this, m_communicator, m_cache.outputDataBitset, [=](ZenProperty_t property, SensorPropertyValue value) { notifyPropertyChange(property, value); }, m_streaming, value);
        else if (property == ZenImuProperty_OutputRawAcc)
            return setOutputDataFlag<ZenImuProperty_OutputRawAcc>(*this, m_communicator, m_cache.outputDataBitset, [=](ZenProperty_t property, SensorPropertyValue value) { notifyPropertyChange(property, value); }, m_streaming, value);
        else if (property == ZenImuProperty_OutputRawMag)
            return setOutputDataFlag<ZenImuProperty_OutputRawMag>(*this, m_communicator, m_cache.outputDataBitset, [=](ZenProperty_t property, SensorPropertyValue value) { notifyPropertyChange(property, value); }, m_streaming, value);
        else if (property == ZenImuProperty_OutputPressure)
            return setOutputDataFlag<ZenImuProperty_OutputPressure>(*this, m_communicator, m_cache.outputDataBitset, [=](ZenProperty_t property, SensorPropertyValue value) { notifyPropertyChange(property, value); }, m_streaming, value);
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

                    uint32_t iValue = value ? 1 : 0;
                    if (auto error = m_communicator.sendAndWaitForAck(0, static_cast<DeviceProperty_t>(propertyV0), static_cast<ZenProperty_t>(propertyV0), gsl::span(reinterpret_cast<const std::byte*>(&iValue), sizeof(iValue))))
                        return error;

                    m_cache.gyrAutoCalibration = value;
                    notifyPropertyChange(property, value);
                    return ZenError_None;
                }
                else
                {
                    return streaming.error();
                }
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

                    uint32_t iValue = value ? 1 : 0;
                    if (auto error = m_communicator.sendAndWaitForAck(0, static_cast<DeviceProperty_t>(propertyV0), static_cast<ZenProperty_t>(propertyV0), gsl::span(reinterpret_cast<const std::byte*>(&iValue), sizeof(iValue))))
                        return error;

                    m_cache.gyrUseThreshold = value;
                    notifyPropertyChange(property, value);
                    return ZenError_None;
                }
                else
                {
                    return streaming.error();
                }
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

                constexpr float eps = std::numeric_limits<float>::epsilon();
                uint32_t iValue = (-eps <= value && value <= eps) ? 0 : 1; // Account for imprecision of float
                if (auto error = m_communicator.sendAndWaitForAck(0, static_cast<DeviceProperty_t>(propertyV0), static_cast<ZenProperty_t>(propertyV0), gsl::make_span(reinterpret_cast<const std::byte*>(&iValue), sizeof(iValue))))
                    return error;

                notifyPropertyChange(property, value);
                return ZenError_None;
            }
            else
            {
                return streaming.error();
            }
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

                uint32_t iValue = lroundf(value);
                if (auto error = m_communicator.sendAndWaitForAck(0, static_cast<DeviceProperty_t>(propertyV0), static_cast<ZenProperty_t>(propertyV0), gsl::make_span(reinterpret_cast<const std::byte*>(&iValue), sizeof(iValue))))
                    return error;

                notifyPropertyChange(property, value);
                return ZenError_None;
            }
            else
            {
                return streaming.error();
            }
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

                if (auto error = m_communicator.sendAndWaitForAck(0, static_cast<DeviceProperty_t>(propertyV0), static_cast<ZenProperty_t>(propertyV0), gsl::make_span(reinterpret_cast<const std::byte*>(&value), sizeof(value))))
                    return error;

                notifyPropertyChange(property, value);
                return ZenError_None;
            }
            else
            {
                return streaming.error();
            }
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

                // Communication protocol only supports uint32_t
                uint32_t uiValue;
                if (property == ZenImuProperty_SamplingRate)
                    uiValue = imu::v0::roundSamplingRate(value);
                if (property == ZenImuProperty_AccRange)
                    uiValue = imu::v0::mapAccRange(value);
                else if (property == ZenImuProperty_GyrRange)
                    uiValue = imu::v0::mapGyrRange(value);
                else if (property == ZenImuProperty_MagRange)
                    uiValue = imu::v0::mapMagRange(value);
                else
                    uiValue = static_cast<uint32_t>(value);

                if (auto error = m_communicator.sendAndWaitForAck(0, static_cast<DeviceProperty_t>(propertyV0), static_cast<ZenProperty_t>(propertyV0), gsl::make_span(reinterpret_cast<const std::byte*>(&uiValue), sizeof(uiValue))))
                    return error;

                if (property == ZenImuProperty_SamplingRate)
                    m_cache.samplingRate = uiValue;

                notifyPropertyChange(property, value);
                return ZenError_None;
            }
            else
            {
                return streaming.error();
            }
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

                if (auto error = m_communicator.sendAndWaitForAck(0, static_cast<DeviceProperty_t>(propertyV0), static_cast<ZenProperty_t>(propertyV0), gsl::make_span(reinterpret_cast<const std::byte*>(value.data), 9 * sizeof(float))))
                    return error;

                notifyPropertyChange(property, gsl::make_span(reinterpret_cast<const std::byte*>(&value.data[0]), 9));
                return ZenError_None;
            }
            else
            {
                return streaming.error();
            }
        }

        return ZenError_UnknownProperty;
    }

    bool LegacyImuProperties::isArray(ZenProperty_t property) const noexcept
    {
        switch (property)
        {
        case ZenImuProperty_SupportedSamplingRates:
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
        case ZenImuProperty_SupportedSamplingRates:
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

        case ZenImuProperty_SamplingRate:
        case ZenImuProperty_SupportedSamplingRates:
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
        notifyPropertyChange(ZenImuProperty_OutputLowPrecision, value);
        return ZenError_None;
    }
}
