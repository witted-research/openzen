//===========================================================================//
//
// Copyright (C) 2020 LP-Research Inc.
//
// This file is part of OpenZen, under the MIT License.
// See https://bitbucket.org/lpresearch/openzen/src/master/LICENSE for details
// SPDX-License-Identifier: MIT
//
//===========================================================================//

#include "properties/LegacyImuProperties.h"

#include <math.h>

#include "SensorProperties.h"
#include "properties/ImuSensorPropertiesV0.h"
#include "utility/Finally.h"

#include <spdlog/spdlog.h>

namespace zen
{
    namespace LegacyImu
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

        template <> struct OutputDataFlag<ZenImuProperty_SamplingRate>
        {
            using index = std::integral_constant<unsigned int, 0>;
        };

        template <> struct OutputDataFlag<ZenImuProperty_GyrUseAutoCalibration>
        {
            using index = std::integral_constant<unsigned int, 30>;
        };

        template <ZenProperty_t property>
        constexpr bool getOutputDataFlag(std::atomic_uint32_t& outputDataBitset) noexcept
        {
            return (outputDataBitset & (1 << OutputDataFlag<property>::index::value)) != 0;
        }

        template <ZenProperty_t property>
        constexpr int getOutputDataSamplingRate(std::atomic_uint32_t& outputDataBitset) noexcept
        {
            // read the sampling rate from the first 3 bits, not documentened in public
            // documents at this time
            auto idx = OutputDataFlag<property>::index::value;
            const uint32_t samplingFlag = (outputDataBitset & (1 << (idx + 0))) +
                (outputDataBitset & (1 << (idx + 1))) +
                (outputDataBitset & (1 << (idx + 2)));

            if (samplingFlag == 0) {
                return 5;
            } else if (samplingFlag == 1) {
                return 10;
            } else if (samplingFlag == 2) {
                return 25;
            } else if (samplingFlag == 3) {
                return 50;
            } else if (samplingFlag == 4) {
                return 100;
            } else if (samplingFlag == 5) {
                return 200;
            } else if (samplingFlag == 6) {
                return 400;
            } else if (samplingFlag == 7) {
                return 800;
            } else {
                spdlog::error("Sampling flag {0} in Config Data set not supported", int(samplingFlag));
                return 0;
            }
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
                newBitset = outputDataBitset | (1 << OutputDataFlag<property>::index::value);
            else
                newBitset = outputDataBitset & ~(1 << OutputDataFlag<property>::index::value);

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
        // 0 means no valid sampling rate has been set yet
        m_cache.samplingRate = 0;
    }

    void LegacyImuProperties::setConfigBitset(uint32_t bitset) noexcept {
        m_cache.configBitset = bitset;

        // extract additional configurations, they are read from the
        // config bitset but set via their own command call
        m_cache.gyrAutoCalibration = LegacyImu::getOutputDataFlag< ZenImuProperty_GyrUseAutoCalibration>(m_cache.configBitset);
        m_cache.samplingRate = LegacyImu::getOutputDataSamplingRate<ZenImuProperty_SamplingRate>(m_cache.configBitset);
    }

    ZenError LegacyImuProperties::execute(ZenProperty_t command) noexcept
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

                const auto function = static_cast<DeviceProperty_t>(imu::v0::mapCommand(command));
                return m_communicator.sendAndWaitForAck(0, function, function, {});
            }
            else
            {
                return streaming.error();
            }
        }

        return ZenError_UnknownProperty;
    }

    std::pair<ZenError, size_t> LegacyImuProperties::getArray(ZenProperty_t property, ZenPropertyType propertyType, gsl::span<std::byte> buffer) noexcept
    {
        if (!isArray(property))
            return std::make_pair(ZenError_UnknownProperty, buffer.size());

        if (propertyType != type(property))
            return std::make_pair(ZenError_WrongDataType, buffer.size());

        // the size of the buffer needs to multiplied by the data type size
        // to arrive at the byte count in the returned buffer
        if (property == ZenImuProperty_SupportedSamplingRates) {
            auto [err, item_count] = imu::v0::supportedSamplingRates(gsl::make_span(reinterpret_cast<int32_t*>(buffer.data()), buffer.size()));
            return {err, item_count * sizeof(int32_t)};
        }
        else if (property == ZenImuProperty_SupportedFilterModes) {
            return imu::v0::supportedFilterModes(buffer);
        }
        else if (property == ZenImuProperty_AccSupportedRanges) {
            auto [err, item_count] = imu::v0::supportedAccRanges(gsl::make_span(reinterpret_cast<int32_t*>(buffer.data()), buffer.size()));
            return {err, item_count * sizeof(int32_t)};
        }
        else if (property == ZenImuProperty_GyrSupportedRanges) {
            auto [err, item_count] =  imu::v0::supportedGyrRanges(gsl::make_span(reinterpret_cast<int32_t*>(buffer.data()), buffer.size()));
            return {err, item_count * sizeof(int32_t)};
        }
        else if (property == ZenImuProperty_MagSupportedRanges) {
            auto [err, item_count] =  imu::v0::supportedMagRanges(gsl::make_span(reinterpret_cast<int32_t*>(buffer.data()), buffer.size()));
            return {err, item_count * sizeof(int32_t)};
        }

        auto streaming = getBool(ZenImuProperty_StreamData);
        if (!streaming)
            return std::make_pair(streaming.error(), buffer.size());

        if (*streaming)
            if (auto error = setBool(ZenImuProperty_StreamData, false))
                return std::make_pair(error, buffer.size());

        auto guard = finally([&]() {
            if (*streaming)
                setBool(ZenImuProperty_StreamData, true);
            });

        const auto function = static_cast<DeviceProperty_t>(imu::v0::map(property, true));
        if (propertyType == ZenPropertyType_Float)
            return m_communicator.sendAndWaitForArray(0, function, function, {}, gsl::make_span(reinterpret_cast<float*>(buffer.data()), buffer.size()));
        else if (propertyType == ZenPropertyType_Int32)
            return m_communicator.sendAndWaitForArray(0, function, function, {}, gsl::make_span(reinterpret_cast<int32_t*>(buffer.data()), buffer.size()));

        return std::make_pair(ZenError_WrongDataType, buffer.size());
    }

    nonstd::expected<bool, ZenError> LegacyImuProperties::getBool(ZenProperty_t property) noexcept
    {
        using LegacyImu::getOutputDataFlag;
        if (property == ZenImuProperty_StreamData)
            return m_streaming;
        else if (property == ZenImuProperty_GyrUseAutoCalibration)
            return m_cache.gyrAutoCalibration;
        else if (property == ZenImuProperty_OutputLowPrecision)
            return getOutputDataFlag<ZenImuProperty_OutputLowPrecision>(m_cache.configBitset);
        else if (property == ZenImuProperty_OutputLinearAcc)
            return getOutputDataFlag<ZenImuProperty_OutputLinearAcc>(m_cache.configBitset);
        else if (property == ZenImuProperty_OutputAltitude)
            return getOutputDataFlag<ZenImuProperty_OutputAltitude>(m_cache.configBitset);
        else if (property == ZenImuProperty_OutputQuat)
            return getOutputDataFlag<ZenImuProperty_OutputQuat>(m_cache.configBitset);
        else if (property == ZenImuProperty_OutputEuler)
            return getOutputDataFlag<ZenImuProperty_OutputEuler>(m_cache.configBitset);
        else if (property == ZenImuProperty_OutputAngularVel)
            return getOutputDataFlag<ZenImuProperty_OutputAngularVel>(m_cache.configBitset);
        else if (property == ZenImuProperty_OutputHeaveMotion)
            return getOutputDataFlag<ZenImuProperty_OutputHeaveMotion>(m_cache.configBitset);
        else if (property == ZenImuProperty_OutputTemperature)
            return getOutputDataFlag<ZenImuProperty_OutputTemperature>(m_cache.configBitset);
        else if (property == ZenImuProperty_OutputRawGyr)
            return getOutputDataFlag<ZenImuProperty_OutputRawGyr>(m_cache.configBitset);
        else if (property == ZenImuProperty_OutputRawAcc)
            return getOutputDataFlag<ZenImuProperty_OutputRawAcc>(m_cache.configBitset);
        else if (property == ZenImuProperty_OutputRawMag)
            return getOutputDataFlag<ZenImuProperty_OutputRawMag>(m_cache.configBitset);
        else if (property == ZenImuProperty_OutputPressure)
            return getOutputDataFlag<ZenImuProperty_OutputPressure>(m_cache.configBitset);

        return nonstd::make_unexpected(ZenError_UnknownProperty);

    }

    constexpr float mapMagCovarFromImu(uint32_t value)
    {
        if (value <= 0x00000000)
            return 0;
        else if (value <= 0x00000001)
            return 1e1;
        else if (value <= 0x00000002)
            return 5e1;
        else
            return 1e2;
    }

    constexpr float mapLinAccCompFromImu(uint32_t value)
    {
        if (value <= 0x00000000)
            return 0;
        else if (value <= 0x00000001)
            return 1e2;
        else if (value <= 0x00000002)
            return 1e3;
        else if (value <= 0x00000003)
            return 1e4;
        else
            return 1e5;
    }

    constexpr float mapCentricAccCompFromImu(uint32_t value)
    {
        if (value <= 0x00000000)
            return 0;
        else
            return 1e2;
    }

    constexpr float mapCanHeartbeatFromImu(uint32_t value)
    {
        if (value <= 0x00000000)
            return 0.5;
        else if (value <= 0x00000001)
            return 1.0;
        else if (value <= 0x00000002)
            return 2.0;
        else if (value <= 0x00000003)
            return 3.0;
        else
            return 10.0;
    }

    nonstd::expected<float, ZenError> LegacyImuProperties::getFloat(ZenProperty_t property) noexcept
    {
        if (isArray(property) || type(property) != ZenPropertyType_Float)
            return nonstd::make_unexpected(ZenError_UnknownProperty);

        auto streaming = getBool(ZenImuProperty_StreamData);
        if (!streaming)
            return nonstd::make_unexpected(streaming.error());

        if (*streaming)
            if (auto error = setBool(ZenImuProperty_StreamData, false))
                return nonstd::make_unexpected(error);

        auto guard = finally([&]() {
            if (*streaming)
                setBool(ZenImuProperty_StreamData, true);
            });

        if (property == ZenImuProperty_FilterPreset) // Note: property name is misleading, this sets the magnetometer covariance
        {
            const auto function = static_cast<DeviceProperty_t>(EDevicePropertyV0::GetFilterPreset);
            if (auto result = m_communicator.sendAndWaitForResult<uint32_t>(0, function, function, {}))
                return mapMagCovarFromImu(*result);
            else
                return nonstd::make_unexpected(result.error());
        }
        if (property == ZenImuProperty_LinearCompensationRate)
        {
            const auto function = static_cast<DeviceProperty_t>(EDevicePropertyV0::GetLinearCompensationRate);
            if (auto result = m_communicator.sendAndWaitForResult<uint32_t>(0, function, function, {}))
                return mapLinAccCompFromImu(*result);
            else
                return nonstd::make_unexpected(result.error());
        }
        else if (property == ZenImuProperty_CentricCompensationRate)
        {
            const auto function = static_cast<DeviceProperty_t>(EDevicePropertyV0::GetCentricCompensationRate);
            if (auto result = m_communicator.sendAndWaitForResult<uint32_t>(0, function, function, {}))
                return mapCentricAccCompFromImu(*result);
            else
                return nonstd::make_unexpected(result.error());
        }
        else if (property == ZenImuProperty_CanHeartbeat)
        {
            const auto function = static_cast<DeviceProperty_t>(EDevicePropertyV0::GetCanHeartbeat);
            if (auto result = m_communicator.sendAndWaitForResult<uint32_t>(0, function, function, {}))
                return mapCanHeartbeatFromImu(*result);
            else
                return nonstd::make_unexpected(result.error());
        }
        else
        {
            const auto function = static_cast<DeviceProperty_t>(imu::v0::map(property, true));
            return m_communicator.sendAndWaitForResult<float>(0, function, function, {});
        }
    }

    constexpr int32_t mapUartBaudrateFromImu(uint32_t value)
    {
        auto i = value & 0x000000ff;
        if (i <= 0)
            return 19200;
        else if (i <= 1)
            return 38400;
        else if (i <= 2)
            return 57600;
        else if (i <= 3)
            return 115200;
        else if (i <= 4)
            return 230400;
        else if (i <= 5)
            return 256000;
        else if (i <= 6)
            return 460800;
        else
            return 921600;
    }

    constexpr int32_t mapUartFormatFromImu(uint32_t value)
    {
        return static_cast<int32_t>(((value & 0xff00) >> 8) - 1);
    }

    constexpr int32_t mapCanBaudrateFromImu(uint32_t value)
    {
        auto i = value & 0x00000038;
        if (i <= 0x00000000)
            return 10000;
        else if (i <= 0x00000008)
            return 20000;
        else if (i <= 0x00000010)
            return 50000;
        else if (i <= 0x00000018)
            return 125000;
        else if (i <= 0x00000020)
            return 250000;
        else if (i <= 0x00000028)
            return 500000;
        else if (i <= 0x00000030)
            return 800000;
        else
            return 1000000;
    }

    /*constexpr */int32_t mapCanChannelModeFromImu(uint32_t value)
    {
        if ((value & 0x1) > 0)
            return 1; // sequential
        else
            return 0; // CANopen
    }

    constexpr int32_t mapCanPointModeFromImu(uint32_t value)
    {
        if ((value & 0x2) > 0)
            return 1; // fixed point
        else
            return 0; // floating point
    }

    constexpr int32_t mapCanStartIdFromImu(uint32_t value)
    {
        return (value & 0xffff0000) >> 16;
    }

    nonstd::expected<int32_t, ZenError> LegacyImuProperties::getInt32(ZenProperty_t property) noexcept
    {
        if (isArray(property) || type(property) != ZenPropertyType_Int32)
            return nonstd::make_unexpected(ZenError_UnknownProperty);

        // can't query sampling rate but output what we have in the cache
        if (property == ZenImuProperty_SamplingRate){
            return m_cache.samplingRate;
        }

        auto streaming = getBool(ZenImuProperty_StreamData);
        if (!streaming)
            return nonstd::make_unexpected(streaming.error());

        if (*streaming)
            if (auto error = setBool(ZenImuProperty_StreamData, false))
                return nonstd::make_unexpected(error);

        auto guard = finally([&]() {
            if (*streaming)
                setBool(ZenImuProperty_StreamData, true);
            });

        // Communication protocol only supports uint32_t
        const auto function = static_cast<DeviceProperty_t>(imu::v0::map(property, true));

        auto result = m_communicator.sendAndWaitForResult<int32_t>(0, function, function, {});
        if (!result)
            return result;

        if (property == ZenImuProperty_UartBaudRate)
            return mapUartBaudrateFromImu(*result);
        else if (property == ZenImuProperty_UartFormat)
            return mapUartFormatFromImu(*result);
        else if (property == ZenImuProperty_CanBaudrate)
            return mapCanBaudrateFromImu(*result);
        else if (property == ZenImuProperty_CanChannelMode)
            return mapCanChannelModeFromImu(*result);
        else if (property == ZenImuProperty_CanPointMode)
            return mapCanPointModeFromImu(*result);
        else if (property == ZenImuProperty_CanStartId)
            return mapCanStartIdFromImu(*result);

        return static_cast<int32_t>(*result);
    }

    ZenError LegacyImuProperties::setArray(ZenProperty_t property, ZenPropertyType propertyType, gsl::span<const std::byte> buffer) noexcept
    {
        if (isConstant(property) || !isArray(property))
            return ZenError_UnknownProperty;


        if (type(property) != propertyType)
            return ZenError_WrongDataType;

        auto streaming = getBool(ZenImuProperty_StreamData);
        if (!streaming)
            return streaming.error();

        if (*streaming)
            if (auto error = setBool(ZenImuProperty_StreamData, false))
                return error;

        auto guard = finally([&]() {
            if (*streaming)
                setBool(ZenImuProperty_StreamData, true);
            });

        const auto function = static_cast<DeviceProperty_t>(imu::v0::map(property, false));
        if (auto error = m_communicator.sendAndWaitForAck(0, function, function, gsl::make_span(buffer.data(), sizeOfPropertyType(propertyType) * buffer.size())))
            return error;

        notifyPropertyChange(property, buffer);
        return ZenError_None;
    }

    ZenError LegacyImuProperties::setBool(ZenProperty_t property, bool value) noexcept
    {
        using LegacyImu::setOutputDataFlag;
        if (property == ZenImuProperty_StreamData)
        {
            if (m_streaming != value)
            {
                const auto propertyV0 = value ? EDevicePropertyV0::SetStreamMode : EDevicePropertyV0::SetCommandMode;
                if (auto error = m_communicator.sendAndWaitForAck(0, static_cast<DeviceProperty_t>(propertyV0),
                    static_cast<ZenProperty_t>(propertyV0), {}))
                    return error;

                m_streaming = value;
                notifyPropertyChange(property, value);
            }
            return ZenError_None;
        }
        else if (property == ZenImuProperty_GyrUseAutoCalibration)
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
                const auto function = static_cast<DeviceProperty_t>(EDevicePropertyV0::SetGyrUseAutoCalibration);
                if (auto error = m_communicator.sendAndWaitForAck(0, function, function,
                    gsl::span(reinterpret_cast<const std::byte*>(&iValue), sizeof(iValue))))
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
        else if (property == ZenImuProperty_OutputLowPrecision)
            return setPrecisionDataFlag(value);
        else if (property == ZenImuProperty_OutputLinearAcc)
            return setOutputDataFlag<ZenImuProperty_OutputLinearAcc>(*this, m_communicator, m_cache.configBitset, [=](ZenProperty_t property, SensorPropertyValue value) { notifyPropertyChange(property, value); }, m_streaming, value);
        else if (property == ZenImuProperty_OutputAltitude)
            return setOutputDataFlag<ZenImuProperty_OutputAltitude>(*this, m_communicator, m_cache.configBitset, [=](ZenProperty_t property, SensorPropertyValue value) { notifyPropertyChange(property, value); }, m_streaming, value);
        else if (property == ZenImuProperty_OutputQuat)
            return setOutputDataFlag<ZenImuProperty_OutputQuat>(*this, m_communicator, m_cache.configBitset, [=](ZenProperty_t property, SensorPropertyValue value) { notifyPropertyChange(property, value); }, m_streaming, value);
        else if (property == ZenImuProperty_OutputEuler)
            return setOutputDataFlag<ZenImuProperty_OutputEuler>(*this, m_communicator, m_cache.configBitset, [=](ZenProperty_t property, SensorPropertyValue value) { notifyPropertyChange(property, value); }, m_streaming, value);
        else if (property == ZenImuProperty_OutputAngularVel)
            return setOutputDataFlag<ZenImuProperty_OutputAngularVel>(*this, m_communicator, m_cache.configBitset, [=](ZenProperty_t property, SensorPropertyValue value) { notifyPropertyChange(property, value); }, m_streaming, value);
        else if (property == ZenImuProperty_OutputHeaveMotion)
            return setOutputDataFlag<ZenImuProperty_OutputHeaveMotion>(*this, m_communicator, m_cache.configBitset, [=](ZenProperty_t property, SensorPropertyValue value) { notifyPropertyChange(property, value); }, m_streaming, value);
        else if (property == ZenImuProperty_OutputTemperature)
            return setOutputDataFlag<ZenImuProperty_OutputTemperature>(*this, m_communicator, m_cache.configBitset, [=](ZenProperty_t property, SensorPropertyValue value) { notifyPropertyChange(property, value); }, m_streaming, value);
        else if (property == ZenImuProperty_OutputRawGyr)
            return setOutputDataFlag<ZenImuProperty_OutputRawGyr>(*this, m_communicator, m_cache.configBitset, [=](ZenProperty_t property, SensorPropertyValue value) { notifyPropertyChange(property, value); }, m_streaming, value);
        else if (property == ZenImuProperty_OutputRawAcc)
            return setOutputDataFlag<ZenImuProperty_OutputRawAcc>(*this, m_communicator, m_cache.configBitset, [=](ZenProperty_t property, SensorPropertyValue value) { notifyPropertyChange(property, value); }, m_streaming, value);
        else if (property == ZenImuProperty_OutputRawMag)
            return setOutputDataFlag<ZenImuProperty_OutputRawMag>(*this, m_communicator, m_cache.configBitset, [=](ZenProperty_t property, SensorPropertyValue value) { notifyPropertyChange(property, value); }, m_streaming, value);
        else if (property == ZenImuProperty_OutputPressure)
            return setOutputDataFlag<ZenImuProperty_OutputPressure>(*this, m_communicator, m_cache.configBitset, [=](ZenProperty_t property, SensorPropertyValue value) { notifyPropertyChange(property, value); }, m_streaming, value);

        return ZenError_UnknownProperty;
    }

    constexpr uint32_t mapMagCovarToImu(float value)
    {
        if (value < 1e1)
            return 0x00000000;
        else if (value < 5e1)
            return 0x00000001;
        else if (value < 1e2)
            return 0x00000002;
        else
            return 0x00000003;
    }

    constexpr uint32_t mapLinAccCompToImu(float value)
    {
        if (value < 1e2)
            return 0x00000000;
        else if (value < 1e3)
            return 0x00000001;
        else if (value < 1e4)
            return 0x00000002;
        else if (value < 1e5)
            return 0x00000003;
        else
            return 0x00000004;
    }

    constexpr uint32_t mapCentricAccCompToImu(float value)
    {
        if (value < 1e2)
            return 0x00000000;
        else
            return 0x00000001;
    }

    constexpr uint32_t mapCanHeartbeatToImu(float value)
    {
        if (value <= 0.5)
            return 0x00000000;
        else if (value <= 1.0)
            return 0x00000001;
        else if (value <= 2.0)
            return 0x00000002;
        else if (value <= 5.0)
            return 0x00000003;
        else
            return 0x00000004;
    }

    ZenError LegacyImuProperties::setFloat(ZenProperty_t property, float value) noexcept
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

                if (property == ZenImuProperty_CentricCompensationRate)
                {
                    const uint32_t iValue = mapCentricAccCompToImu(value);
                    const auto function = static_cast<DeviceProperty_t>(EDevicePropertyV0::SetCentricCompensationRate);
                    if (auto error = m_communicator.sendAndWaitForAck(0, function, function, gsl::make_span(reinterpret_cast<const std::byte*>(&iValue), sizeof(iValue))))
                        return error;

                    notifyPropertyChange(property, value);
                    return ZenError_None;
                }
                else if (property == ZenImuProperty_LinearCompensationRate)
                {
                    uint32_t iValue = mapLinAccCompToImu(value);
                    const auto function = static_cast<DeviceProperty_t>(EDevicePropertyV0::SetLinearCompensationRate);
                    if (auto error = m_communicator.sendAndWaitForAck(0, function, function, gsl::make_span(reinterpret_cast<const std::byte*>(&iValue), sizeof(iValue))))
                        return error;

                    notifyPropertyChange(property, value);
                    return ZenError_None;
                }
                else if (property == ZenImuProperty_FilterPreset) // Note: property name is misleading, this sets the magnetometer covariance
                {
                    uint32_t iValue = mapMagCovarToImu(value);
                    const auto function = static_cast<DeviceProperty_t>(EDevicePropertyV0::SetFilterPreset);
                    if (auto error = m_communicator.sendAndWaitForAck(0, function, function, gsl::make_span(reinterpret_cast<const std::byte*>(&iValue), sizeof(iValue))))
                        return error;

                    notifyPropertyChange(property, value);
                    return ZenError_None;
                }
                else if (property == ZenImuProperty_CanHeartbeat)
                {
                    const auto function = static_cast<DeviceProperty_t>(imu::v0::map(property, false));
                    uint32_t iValue = mapCanHeartbeatToImu(value);
                    if (auto error = m_communicator.sendAndWaitForAck(0, function, function, gsl::make_span(reinterpret_cast<const std::byte*>(&iValue), sizeof(value))))
                        return error;

                    notifyPropertyChange(property, value);
                    return ZenError_None;
                }
                else
                {
                    const auto function = static_cast<DeviceProperty_t>(imu::v0::map(property, false));
                    if (auto error = m_communicator.sendAndWaitForAck(0, function, function, gsl::make_span(reinterpret_cast<const std::byte*>(&value), sizeof(value))))
                        return error;

                    notifyPropertyChange(property, value);
                    return ZenError_None;
                }
            }
            else
            {
                return streaming.error();
            }
        }

        return ZenError_UnknownProperty;
    }

    constexpr uint32_t mapUartBaudrateToImu(int32_t value)
    {
        if (value <= 19200)
            return 0;
        else if (value <= 38400)
            return 1;
        else if (value <= 57600)
            return 2;
        else if (value <= 115200)
            return 3;
        else if (value <= 230400)
            return 4;
        else if (value <= 256000)
            return 5;
        else if (value <= 460800)
            return 6;
        else
            return 7;
    }

    constexpr uint32_t mapUartFormatToImu(int32_t value)
    {
        if (value <= 0)
            return 0x00000100; // LP-BUS
        else
            return 0x00000200; // ASCII
    }

    constexpr uint32_t mapCanBaudrateToImu(int32_t value)
    {
        if (value <= 10000)
            return 10;
        else if (value <= 20000)
            return 20;
        else if (value <= 30000)
            return 30;
        else if (value <= 50000)
            return 50;
        else if (value <= 125000)
            return 125;
        else if (value <= 250000)
            return 250;
        else if (value <= 500000)
            return 500;
        else if (value <= 800000)
            return 800;
        else
            return 1000;
    }

    /* constexpr */uint32_t mapCanChannelModeToImu(int32_t value)
    {
        printf("map to imu: %d\n", value);

        if (value <= 0)
            return 0x00000000; // CANopen
        else
            return 0x00000001; // sequential
    }

    constexpr uint32_t mapCanPointModeToImu(int32_t value)
    {
        if (value <= 0)
            return 0x00000000; // floating point
        else
            return 0x00000002; // fixed point
    }

    constexpr uint32_t mapCanStartIdToImu(int32_t value)
    {
        return (uint32_t)value & 0xffff;
    }

    ZenError LegacyImuProperties::setInt32(ZenProperty_t property, int32_t value) noexcept
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
                    uiValue = imu::v0::roundSamplingRate(value);
                if (property == ZenImuProperty_AccRange)
                    uiValue = imu::v0::mapAccRange(value);
                else if (property == ZenImuProperty_GyrRange)
                    uiValue = imu::v0::mapGyrRange(value);
                else if (property == ZenImuProperty_MagRange)
                    uiValue = imu::v0::mapMagRange(value);
                else if (property == ZenImuProperty_UartFormat)
                    uiValue = mapUartFormatToImu(value);
                else if (property == ZenImuProperty_UartBaudRate)
                    uiValue = mapUartBaudrateToImu(value);
                else if (property == ZenImuProperty_CanBaudrate)
                    uiValue = mapCanBaudrateToImu(value);
                else if (property == ZenImuProperty_CanChannelMode)
                    uiValue = mapCanChannelModeToImu(value);
                else if (property == ZenImuProperty_CanPointMode)
                    uiValue = mapCanPointModeToImu(value);
                else if (property == ZenImuProperty_CanStartId)
                    uiValue = mapCanStartIdToImu(value);
                else
                    uiValue = static_cast<uint32_t>(value);

                const auto function = static_cast<DeviceProperty_t>(imu::v0::map(property, false));
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

    bool LegacyImuProperties::isArray(ZenProperty_t property) const noexcept
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
        case ZenImuProperty_CanMapping:
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
        case ZenImuProperty_StartSensorSync:
        case ZenImuProperty_StopSensorSync:
            return true;

        default:
            return false;
        }
    }

    ZenPropertyType LegacyImuProperties::type(ZenProperty_t property) const noexcept
    {
        switch (property)
        {
        case ZenImuProperty_SupportedFilterModes:
            return ZenPropertyType_Byte;

        case ZenImuProperty_StreamData:
        case ZenImuProperty_GyrUseAutoCalibration:
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
        case ZenImuProperty_FilterPreset: // Note: property name is misleading, this sets the magnetometer covariance
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
        case ZenImuProperty_CanHeartbeat:
            return ZenPropertyType_Float;

        case ZenImuProperty_SamplingRate:
        case ZenImuProperty_SupportedSamplingRates:
        case ZenImuProperty_FilterMode:
        case ZenImuProperty_OrientationOffsetMode:
        case ZenImuProperty_AccRange:
        case ZenImuProperty_AccSupportedRanges:
        case ZenImuProperty_GyrRange:
        case ZenImuProperty_GyrSupportedRanges:
        case ZenImuProperty_MagRange:
        case ZenImuProperty_MagSupportedRanges:

        case ZenImuProperty_CanChannelMode:
        case ZenImuProperty_CanPointMode:
        case ZenImuProperty_CanStartId:
        case ZenImuProperty_CanBaudrate:
        case ZenImuProperty_CanMapping:
        case ZenImuProperty_UartBaudRate:
        case ZenImuProperty_UartFormat:
            return ZenPropertyType_Int32;

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
            newBitset = m_cache.configBitset | (1 << 22);
        else
            newBitset = m_cache.configBitset & ~(1 << 22);

        const uint32_t iValue = value ? 1 : 0;
        if (auto error = m_communicator.sendAndWaitForAck(0, static_cast<DeviceProperty_t>(EDevicePropertyV0::SetDataMode), static_cast<ZenProperty_t>(EDevicePropertyV0::SetDataMode), gsl::make_span(reinterpret_cast<const std::byte*>(&iValue), sizeof(iValue))))
            return error;

        m_cache.configBitset = newBitset;
        notifyPropertyChange(ZenImuProperty_OutputLowPrecision, value);
        return ZenError_None;
    }
}
