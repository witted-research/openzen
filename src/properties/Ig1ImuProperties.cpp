#include "properties/Ig1ImuProperties.h"

#include <math.h>
#include <iostream>

#include "SensorProperties.h"
#include "properties/ImuSensorPropertiesV1.h"
#include "utility/Finally.h"

namespace zen
{
    namespace
    {
        template <ZenProperty_t type>
        struct OutputDataFlag
        {};

        // change here for the new output flags of IG1
/*      
not supported by Ig1 atm
template <> struct OutputDataFlag<ZenImuProperty_OutputLowPrecision>
        {
            using index = std::integral_constant<unsigned int, 22>;
        };
        */

        template <> struct OutputDataFlag<ZenImuProperty_OutputRawAcc>
        {
            using index = std::integral_constant<unsigned int, 0>;
        };

        template <> struct OutputDataFlag<ZenImuProperty_OutputAccCalibrated>
        {
            using index = std::integral_constant<unsigned int, 1>;
        };

        template <> struct OutputDataFlag<ZenImuProperty_OutputRawGyr0>
        {
            using index = std::integral_constant<unsigned int, 2>;
        };

        template <> struct OutputDataFlag<ZenImuProperty_OutputRawGyr1>
        {
            using index = std::integral_constant<unsigned int, 3>;
        };

        template <> struct OutputDataFlag<ZenImuProperty_OutputGyr0BiasCalib>
        {
            using index = std::integral_constant<unsigned int, 4>;
        };

        template <> struct OutputDataFlag<ZenImuProperty_OutputGyr1BiasCalib>
        {
            using index = std::integral_constant<unsigned int, 5>;
        };

        template <> struct OutputDataFlag<ZenImuProperty_OutputGyr0AlignCalib>
        {
            using index = std::integral_constant<unsigned int, 6>;
        };

        template <> struct OutputDataFlag<ZenImuProperty_OutputGyr1AlignCalib>
        {
            using index = std::integral_constant<unsigned int, 7>;
        };

        template <> struct OutputDataFlag<ZenImuProperty_OutputRawMag>
        {
            using index = std::integral_constant<unsigned int, 8>;
        };

        template <> struct OutputDataFlag<ZenImuProperty_OutputMagCalib>
        {
            using index = std::integral_constant<unsigned int, 9>;
        };

        template <> struct OutputDataFlag<ZenImuProperty_OutputAngularVel>
        {
            using index = std::integral_constant<unsigned int, 10>;
        };

        template <> struct OutputDataFlag<ZenImuProperty_OutputQuat>
        {
            using index = std::integral_constant<unsigned int, 11>;
        };

        template <> struct OutputDataFlag<ZenImuProperty_OutputEuler>
        {
            using index = std::integral_constant<unsigned int, 12>;
        };

        template <> struct OutputDataFlag<ZenImuProperty_OutputLinearAcc>
        {
            using index = std::integral_constant<unsigned int, 13>;
        };

        template <> struct OutputDataFlag<ZenImuProperty_OutputPressure>
        {
            using index = std::integral_constant<unsigned int, 14>;
        };

        template <> struct OutputDataFlag<ZenImuProperty_OutputAltitude>
        {
            using index = std::integral_constant<unsigned int, 15>;
        };

        template <> struct OutputDataFlag<ZenImuProperty_OutputTemperature>
        {
            using index = std::integral_constant<unsigned int, 16>;
        };

        template <ZenProperty_t property>
        constexpr bool getOutputDataFlag(std::atomic_uint32_t& outputDataBitset) noexcept
        {
            return (outputDataBitset & (1 << OutputDataFlag<property>::index::value)) != 0;
        }

        template <ZenProperty_t property>
        ZenError setOutputDataFlag(Ig1ImuProperties& self, SyncedModbusCommunicator& communicator,
            std::atomic_uint32_t& outputDataBitset, std::function<void(ZenProperty_t,SensorPropertyValue)> notifyPropertyChange,
            bool streaming, bool value) noexcept
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
                newBitset = outputDataBitset | (1 << OutputDataFlag<property>::index::value);
            else
                newBitset = outputDataBitset & ~(1 << OutputDataFlag<property>::index::value);

            if (auto error = communicator.sendAndWaitForAck(0, static_cast<DeviceProperty_t>(EDevicePropertyV1::SetImuTransmitData),
                static_cast<ZenProperty_t>(EDevicePropertyV1::SetImuTransmitData), gsl::make_span(reinterpret_cast<const std::byte*>(&newBitset), sizeof(newBitset))))
                return error;

            outputDataBitset = newBitset;
            notifyPropertyChange(property, value);
            return ZenError_None;
        }
    }

    Ig1ImuProperties::Ig1ImuProperties(SyncedModbusCommunicator& communicator) noexcept
        : m_cache{}
        , m_communicator(communicator)
        , m_streaming(true)
    {
        m_cache.samplingRate = 200;
    }

    ZenError Ig1ImuProperties::execute(ZenProperty_t command) noexcept
    {
        if (isExecutable(command))
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

                const auto function = static_cast<DeviceProperty_t>(imu::v1::mapCommand(command));
                return m_communicator.sendAndWaitForAck(0, function, function, {});
            }
            else
            {
                return streaming.error();
            }
        }

        return ZenError_UnknownProperty;
    }

    std::pair<ZenError, size_t> Ig1ImuProperties::getArray(ZenProperty_t property, ZenPropertyType propertyType, gsl::span<std::byte> buffer) noexcept
    {
        if (isArray(property))
        {
            if (propertyType != type(property))
                return std::make_pair(ZenError_WrongDataType, buffer.size());

            if (property == ZenImuProperty_SupportedSamplingRates)
                return imu::v1::supportedSamplingRates(gsl::make_span(reinterpret_cast<int32_t*>(buffer.data()), buffer.size()));
            else if (property == ZenImuProperty_SupportedFilterModes)
                return imu::v1::supportedFilterModes(buffer);
            else if (property == ZenImuProperty_AccSupportedRanges)
                return imu::v1::supportedAccRanges(gsl::make_span(reinterpret_cast<int32_t*>(buffer.data()), buffer.size()));
            else if (property == ZenImuProperty_GyrSupportedRanges)
                return imu::v1::supportedGyrRanges(gsl::make_span(reinterpret_cast<int32_t*>(buffer.data()), buffer.size()));
            else if (property == ZenImuProperty_MagSupportedRanges)
                return imu::v1::supportedMagRanges(gsl::make_span(reinterpret_cast<int32_t*>(buffer.data()), buffer.size()));
            else
            {
                if (auto streaming = getBool(ZenImuProperty_StreamData))
                {
                    if (*streaming)
                        if (auto error = setBool(ZenImuProperty_StreamData, false))
                            return std::make_pair(error, buffer.size());

                    auto guard = finally([&]() {
                        if (*streaming)
                            setBool(ZenImuProperty_StreamData, true);
                    });

                    const auto function = static_cast<DeviceProperty_t>(imu::v1::map(property, true));
                    switch (propertyType)
                    {
                    case ZenPropertyType_Float:
                        return m_communicator.sendAndWaitForArray(0, function, function, {}, gsl::make_span(reinterpret_cast<float*>(buffer.data()), buffer.size()));

                    default:
                        return std::make_pair(ZenError_WrongDataType, buffer.size());
                    }
                }
                else
                {
                    return std::make_pair(streaming.error(), buffer.size());
                }
            }
        }

        return std::make_pair(ZenError_UnknownProperty, buffer.size());
    }

    nonstd::expected<bool, ZenError> Ig1ImuProperties::getBool(ZenProperty_t property) noexcept
    {
        if (property == ZenImuProperty_StreamData)
            return m_streaming;
        // this is a Int32 on the device side but treated as a bool in OpenZen
        // to simplify usage
        else if (property == ZenImuProperty_GyrUseAutoCalibration)
            return getInt32AsBool(property);
        else if (property == ZenImuProperty_GyrUseThreshold)
            return getInt32AsBool(property);
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
        else if (property == ZenImuProperty_OutputTemperature)
            return getOutputDataFlag<ZenImuProperty_OutputTemperature>(m_cache.outputDataBitset);
        
        // Gyr 0
        else if (property == ZenImuProperty_OutputRawGyr0)
            return getOutputDataFlag<ZenImuProperty_OutputRawGyr0>(m_cache.outputDataBitset);
        else if (property == ZenImuProperty_OutputGyr0AlignCalib)
            return getOutputDataFlag<ZenImuProperty_OutputGyr0AlignCalib>(m_cache.outputDataBitset);
        else if (property == ZenImuProperty_OutputGyr0BiasCalib)
            return getOutputDataFlag<ZenImuProperty_OutputGyr0BiasCalib>(m_cache.outputDataBitset);

        // Gyr 1
        else if (property == ZenImuProperty_OutputRawGyr1)
            return getOutputDataFlag<ZenImuProperty_OutputRawGyr1>(m_cache.outputDataBitset);
        else if (property == ZenImuProperty_OutputGyr1AlignCalib)
            return getOutputDataFlag<ZenImuProperty_OutputGyr1AlignCalib>(m_cache.outputDataBitset);
        else if (property == ZenImuProperty_OutputGyr1BiasCalib)
            return getOutputDataFlag<ZenImuProperty_OutputGyr1BiasCalib>(m_cache.outputDataBitset);

        // calibrated acceleration
        else if (property == ZenImuProperty_OutputAccCalibrated)
            return getOutputDataFlag<ZenImuProperty_OutputAccCalibrated>(m_cache.outputDataBitset);
        
        else if (property == ZenImuProperty_OutputRawAcc)
            return getOutputDataFlag<ZenImuProperty_OutputRawAcc>(m_cache.outputDataBitset);
        else if (property == ZenImuProperty_OutputRawMag)
            return getOutputDataFlag<ZenImuProperty_OutputRawMag>(m_cache.outputDataBitset);
        else if (property == ZenImuProperty_OutputMagCalib)
            return getOutputDataFlag<ZenImuProperty_OutputMagCalib>(m_cache.outputDataBitset);

        else if (property == ZenImuProperty_OutputPressure)
            return getOutputDataFlag<ZenImuProperty_OutputPressure>(m_cache.outputDataBitset);

        return nonstd::make_unexpected(ZenError_UnknownProperty);

    }

    nonstd::expected<float, ZenError> Ig1ImuProperties::getFloat(ZenProperty_t property) noexcept
    {
        if (!isArray(property) && type(property) == ZenPropertyType_Float)
        {
            const bool streaming = m_streaming;
            if (streaming)
                if (auto error = setBool(ZenImuProperty_StreamData, false))
                    return nonstd::make_unexpected(error);

            auto guard = finally([=]() {
                if (streaming)
                    setBool(ZenImuProperty_StreamData, true);
            });

            const auto function = static_cast<DeviceProperty_t>(imu::v1::map(property, true));
            return m_communicator.sendAndWaitForResult<float>(0, function, function, {});
        }

        return nonstd::make_unexpected(ZenError_UnknownProperty);
    }

    nonstd::expected<int32_t, ZenError> Ig1ImuProperties::getInt32(ZenProperty_t property) noexcept
    {
        if (!isArray(property) && type(property) == ZenPropertyType_Int32)
        {
            if (property == ZenImuProperty_SamplingRate)
            {
                return m_cache.samplingRate;
            }
            else
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
                    const auto function = static_cast<DeviceProperty_t>(imu::v1::map(property, true));
                    if (auto result = m_communicator.sendAndWaitForResult<uint32_t>(0, function, function, {}))
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

    ZenError Ig1ImuProperties::setArray(ZenProperty_t property, ZenPropertyType propertyType, gsl::span<const std::byte> buffer) noexcept
    {
        if (!isConstant(property) && isArray(property))
        {
            if (type(property) != propertyType)
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

                const auto function = static_cast<DeviceProperty_t>(imu::v1::map(property, false));
                if (auto error = m_communicator.sendAndWaitForAck(0, function, function, gsl::make_span(buffer.data(), sizeOfPropertyType(propertyType) * buffer.size())))
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

    ZenError Ig1ImuProperties::setBool(ZenProperty_t property, bool value) noexcept
    {
        if (property == ZenImuProperty_StreamData)
        {
            if (m_streaming != value)
            {
                const auto propertyV0 = value ? EDevicePropertyV1::GotoStreamMode : EDevicePropertyV1::GotoCommandMode;
                if (auto error = m_communicator.sendAndWaitForAck(0, static_cast<DeviceProperty_t>(propertyV0),
                    static_cast<ZenProperty_t>(propertyV0),
                    gsl::span<std::byte>()
                )) {
                    return error;
                }
                m_streaming = value;
                notifyPropertyChange(property, value);
            }
            return ZenError_None;
        }
        else if (property == ZenImuProperty_GyrUseAutoCalibration)
            return setInt32AsBool(ZenImuProperty_GyrUseAutoCalibration, value);
        else if (property == ZenImuProperty_GyrUseThreshold)
            return setInt32AsBool(ZenImuProperty_GyrUseThreshold, value);
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
        else if (property == ZenImuProperty_OutputTemperature)
            return setOutputDataFlag<ZenImuProperty_OutputTemperature>(*this, m_communicator, m_cache.outputDataBitset, [=](ZenProperty_t property, SensorPropertyValue value) { notifyPropertyChange(property, value); }, m_streaming, value);
        else if (property == ZenImuProperty_OutputRawGyr)
            return setOutputDataFlag<ZenImuProperty_OutputRawAcc>(*this, m_communicator, m_cache.outputDataBitset, [=](ZenProperty_t property, SensorPropertyValue value) { notifyPropertyChange(property, value); }, m_streaming, value);
        else if (property == ZenImuProperty_OutputRawMag)
            return setOutputDataFlag<ZenImuProperty_OutputRawMag>(*this, m_communicator, m_cache.outputDataBitset, [=](ZenProperty_t property, SensorPropertyValue value) { notifyPropertyChange(property, value); }, m_streaming, value);
        else if (property == ZenImuProperty_OutputMagCalib)
            return setOutputDataFlag<ZenImuProperty_OutputMagCalib>(*this, m_communicator, m_cache.outputDataBitset, [=](ZenProperty_t property, SensorPropertyValue value) { notifyPropertyChange(property, value); }, m_streaming, value);
        else if (property == ZenImuProperty_OutputPressure)
            return setOutputDataFlag<ZenImuProperty_OutputPressure>(*this, m_communicator, m_cache.outputDataBitset, [=](ZenProperty_t property, SensorPropertyValue value) { notifyPropertyChange(property, value); }, m_streaming, value);

        // Gyros 
        else if (property == ZenImuProperty_OutputRawGyr0)
            return setOutputDataFlag<ZenImuProperty_OutputRawGyr0>(*this, m_communicator, m_cache.outputDataBitset, [=](ZenProperty_t property, SensorPropertyValue value) { notifyPropertyChange(property, value); }, m_streaming, value);
        else if (property == ZenImuProperty_OutputGyr0AlignCalib)
            return setOutputDataFlag<ZenImuProperty_OutputGyr0AlignCalib>(*this, m_communicator, m_cache.outputDataBitset, [=](ZenProperty_t property, SensorPropertyValue value) { notifyPropertyChange(property, value); }, m_streaming, value);
        else if (property == ZenImuProperty_OutputGyr0BiasCalib)
            return setOutputDataFlag<ZenImuProperty_OutputGyr0BiasCalib>(*this, m_communicator, m_cache.outputDataBitset, [=](ZenProperty_t property, SensorPropertyValue value) { notifyPropertyChange(property, value); }, m_streaming, value);

        else if (property == ZenImuProperty_OutputRawGyr1)
            return setOutputDataFlag<ZenImuProperty_OutputRawGyr1>(*this, m_communicator, m_cache.outputDataBitset, [=](ZenProperty_t property, SensorPropertyValue value) { notifyPropertyChange(property, value); }, m_streaming, value);
        else if (property == ZenImuProperty_OutputGyr1AlignCalib)
            return setOutputDataFlag<ZenImuProperty_OutputGyr1AlignCalib>(*this, m_communicator, m_cache.outputDataBitset, [=](ZenProperty_t property, SensorPropertyValue value) { notifyPropertyChange(property, value); }, m_streaming, value);
        else if (property == ZenImuProperty_OutputGyr1BiasCalib)
            return setOutputDataFlag<ZenImuProperty_OutputGyr1BiasCalib>(*this, m_communicator, m_cache.outputDataBitset, [=](ZenProperty_t property, SensorPropertyValue value) { notifyPropertyChange(property, value); }, m_streaming, value);
        else if (property == ZenImuProperty_OutputAccCalibrated)
            return setOutputDataFlag<ZenImuProperty_OutputAccCalibrated>(*this, m_communicator, m_cache.outputDataBitset, [=](ZenProperty_t property, SensorPropertyValue value) { notifyPropertyChange(property, value); }, m_streaming, value);

        return ZenError_UnknownProperty;
    }

    ZenError Ig1ImuProperties::setFloat(ZenProperty_t property, float value) noexcept
    {
        if (!isConstant(property) && !isArray(property) && type(property) == ZenPropertyType_Float)
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

                const auto function = static_cast<DeviceProperty_t>(imu::v1::map(property, false));
                if (auto error = m_communicator.sendAndWaitForAck(0, function, function, gsl::make_span(reinterpret_cast<const std::byte*>(&value), sizeof(value))))
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

    ZenError Ig1ImuProperties::setInt32(ZenProperty_t property, int32_t value) noexcept
    {
        if (!isConstant(property) && !isArray(property) && type(property) == ZenPropertyType_Int32)
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
                    uiValue = imu::v1::roundSamplingRate(value);
                if (property == ZenImuProperty_AccRange)
                    uiValue = imu::v1::mapAccRange(value);
                else if (property == ZenImuProperty_GyrRange)
                    uiValue = imu::v1::mapGyrRange(value);
                else if (property == ZenImuProperty_MagRange)
                    uiValue = imu::v1::mapMagRange(value);
                else
                    uiValue = static_cast<uint32_t>(value);

                const auto function = static_cast<DeviceProperty_t>(imu::v1::map(property, false));
                if (auto error = m_communicator.sendAndWaitForAck(0, function, function, gsl::make_span(reinterpret_cast<const std::byte*>(&uiValue), sizeof(uiValue))))
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

    bool Ig1ImuProperties::isArray(ZenProperty_t property) const noexcept
    {
        switch (property)
        {
        case ZenImuProperty_SupportedSamplingRates:
        case ZenImuProperty_SupportedFilterModes:
        case ZenImuProperty_AccAlignment:
        case ZenImuProperty_AccBias:
        case ZenImuProperty_AccSupportedRanges:
        case ZenImuProperty_GyrAlignment:
        case ZenImuProperty_GyrBias:
        case ZenImuProperty_GyrSupportedRanges:
        case ZenImuProperty_MagAlignment:
        case ZenImuProperty_MagBias:
        case ZenImuProperty_MagSupportedRanges:
        case ZenImuProperty_MagReference:
        case ZenImuProperty_MagHardIronOffset:
        case ZenImuProperty_MagSoftIronMatrix:
            return true;

        default:
            return false;
        }
    }

    bool Ig1ImuProperties::isConstant(ZenProperty_t property) const noexcept
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

    bool Ig1ImuProperties::isExecutable(ZenProperty_t property) const noexcept
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

    ZenPropertyType Ig1ImuProperties::type(ZenProperty_t property) const noexcept
    {
        switch (property)
        {
        case ZenImuProperty_SupportedFilterModes:
            return ZenPropertyType_Byte;

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
        case ZenImuProperty_AccAlignment:
        case ZenImuProperty_AccBias:
        case ZenImuProperty_GyrAlignment:
        case ZenImuProperty_GyrBias:
        case ZenImuProperty_MagAlignment:
        case ZenImuProperty_MagBias:
        case ZenImuProperty_MagReference:
        case ZenImuProperty_MagHardIronOffset:
        case ZenImuProperty_MagSoftIronMatrix:
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

        default:
            return ZenPropertyType_Invalid;
        }
    }

    nonstd::expected<bool, ZenError> Ig1ImuProperties::getInt32AsBool(ZenProperty_t property) {
        if (auto streaming = getBool(ZenImuProperty_StreamData))
        {
            if (*streaming)
                if (auto error = setBool(ZenImuProperty_StreamData, false))
                    return error;

            auto guard = finally([&]() {
                if (*streaming)
                    setBool(ZenImuProperty_StreamData, true);
                });

            const auto function = static_cast<DeviceProperty_t>(imu::v1::map(property, true));
            if (auto result = m_communicator.sendAndWaitForResult<uint32_t>(0, function, function, {}))
                return static_cast<int32_t>(*result) > 0;
            else
                return result.error();
        }
        else
        {
            return streaming.error();
        }
    }

    ZenError Ig1ImuProperties::setInt32AsBool(ZenProperty_t property, bool value) {
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
            const auto function = static_cast<DeviceProperty_t>(imu::v1::map(property, false));
            if (auto error = m_communicator.sendAndWaitForAck(0, function, function,
                gsl::span(reinterpret_cast<const std::byte*>(&iValue), sizeof(iValue))))
                return error;

            notifyPropertyChange(property, value);
            return ZenError_None;
        }
        else
        {
            return streaming.error();
        }
    }

}
