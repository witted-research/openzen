//===========================================================================//
//
// Copyright (C) 2020 LP-Research Inc.
//
// This file is part of OpenZen, under the MIT License.
// See https://bitbucket.org/lpresearch/openzen/src/master/LICENSE for details
// SPDX-License-Identifier: MIT
//
//===========================================================================//

#include "Ig1CoreProperties.h"

#include <cstring>

#include "properties/BaseSensorPropertiesV1.h"
#include "utility/Finally.h"

namespace zen
{
    Ig1CoreProperties::Ig1CoreProperties(SyncedModbusCommunicator& communicator, ISensorProperties& imu)
        : m_communicator(communicator)
        , m_imu(imu)
    {}

    ZenError Ig1CoreProperties::execute(ZenProperty_t command) noexcept
    {
        if (isExecutable(command))
        {
            if (auto streaming = m_imu.getBool(ZenImuProperty_StreamData))
            {
                if (*streaming)
                    if (auto error = m_imu.setBool(ZenImuProperty_StreamData, false))
                        return error;

                auto guard = finally([=]() {
                    if (*streaming)
                        m_imu.setBool(ZenImuProperty_StreamData, true);
                });

                const auto function = static_cast<DeviceProperty_t>(base::v1::mapCommand(command));
                return m_communicator.sendAndWaitForAck(0, function, function, {});
            }
            else
            {
                return streaming.error();
            }
        }

        return ZenError_UnknownProperty;
    }

    std::pair<ZenError, size_t> Ig1CoreProperties::getArray(ZenProperty_t property, ZenPropertyType propertyType, gsl::span<std::byte> buffer) noexcept
    {
        if (isArray(property))
        {
            if (propertyType != type(property))
                return std::make_pair(ZenError_WrongDataType, buffer.size());

            if (property == ZenSensorProperty_SupportedBaudRates)
            {
                return supportedBaudRates(buffer);
            }
            else
            {
                if (auto streaming = m_imu.getBool(ZenImuProperty_StreamData))
                {
                    if (*streaming)
                        if (auto error = m_imu.setBool(ZenImuProperty_StreamData, false))
                            return std::make_pair(error, buffer.size());

                    auto guard = finally([&]() {
                        if (*streaming)
                            m_imu.setBool(ZenImuProperty_StreamData, true);
                    });

                    const DeviceProperty_t function = static_cast<DeviceProperty_t>(base::v1::map(property, true));
                    // [LEGACY] The core sensor only supports integer properties
                    switch (propertyType)
                    {
                    case ZenPropertyType_Int32:
                    {
                        // As the communication protocol only supports uint32_t for getting arrays, we need to cast all values to guarantee the correct sign
                        // This only applies to Sensor, as IMUComponent has no integer array properties
                        uint32_t* uiBuffer = reinterpret_cast<uint32_t*>(buffer.data());
                        const auto result = m_communicator.sendAndWaitForArray(0, function, function, {}, gsl::make_span(uiBuffer, buffer.size()));
                        if (result.first)
                            return result;

                        int32_t * iBuffer = reinterpret_cast<int32_t*>(buffer.data());
                        for (size_t idx = 0; idx < result.second; ++idx)
                            iBuffer[idx] = static_cast<int32_t>(uiBuffer[idx]);

                        // Some properties need to be reversed
                        const bool reverse = property == ZenSensorProperty_FirmwareVersion;
                        if (reverse)
                            std::reverse(iBuffer, iBuffer + result.second);

                        return std::make_pair(ZenError_None, result.second);
                    }
                    case ZenPropertyType_Byte:
                    {
                        std::byte* uiBuffer = reinterpret_cast<std::byte*>(buffer.data());
                        const auto result = m_communicator.sendAndWaitForArray(0, function, function, {}, gsl::make_span(uiBuffer, buffer.size()));
                        if (result.first)
                            return result;

                        return std::make_pair(ZenError_None, result.second);

                        // return std::make_pair(ZenError_WrongDataType, buffer.size());
                    }
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

    nonstd::expected<bool, ZenError> Ig1CoreProperties::getBool(ZenProperty_t property) noexcept
    {
        // The base sensor has no boolean properties that can be retrieved, so no need to add backwards compatibility
        if (type(property) == ZenPropertyType_Bool)
        {
            if (auto streaming = m_imu.getBool(ZenImuProperty_StreamData))
            {
                if (*streaming)
                    if (auto error = m_imu.setBool(ZenImuProperty_StreamData, false))
                        return nonstd::make_unexpected(error);

                auto guard = finally([&]() {
                    if (*streaming)
                        m_imu.setBool(ZenImuProperty_StreamData, true);
                });

                const auto function = static_cast<uint8_t>(EDevicePropertyV0::GetBatteryCharging);
                if (auto result = m_communicator.sendAndWaitForResult<uint32_t>(0, function, function, {}))
                    return *result != 0;
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

    nonstd::expected<float, ZenError> Ig1CoreProperties::getFloat(ZenProperty_t property) noexcept
    {
        if (!isArray(property) && type(property) == ZenPropertyType_Float)
        {
            if (auto streaming = m_imu.getBool(ZenImuProperty_StreamData))
            {
                if (*streaming)
                    if (auto error = m_imu.setBool(ZenImuProperty_StreamData, false))
                        return nonstd::make_unexpected(error);

                auto guard = finally([&]() {
                    if (*streaming)
                        m_imu.setBool(ZenImuProperty_StreamData, true);
                });

                const auto function = static_cast<DeviceProperty_t>(base::v1::map(property, true));
                return m_communicator.sendAndWaitForResult<float>(0, function, function, {});
            }
            else
            {
                return nonstd::make_unexpected(streaming.error());
            }
        }

        return nonstd::make_unexpected(ZenError_UnknownProperty);
    }

    nonstd::expected<int32_t, ZenError> Ig1CoreProperties::getInt32(ZenProperty_t property) noexcept
    {
        if (property == ZenSensorProperty_BaudRate)
            return m_communicator.baudRate();
        else
        {
            if (property == ZenSensorProperty_TimeOffset)
            {
                if (auto streaming = m_imu.getBool(ZenImuProperty_StreamData))
                {
                    if (*streaming)
                        if (auto error = m_imu.setBool(ZenImuProperty_StreamData, false))
                            return nonstd::make_unexpected(error);

                    auto guard = finally([&]() {
                        if (*streaming)
                            m_imu.setBool(ZenImuProperty_StreamData, true);
                    });

                    // Legacy communication protocol only supports uint32_t
                    const auto function = static_cast<DeviceProperty_t>(base::v1::map(property, true));
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

        return ZenError_UnknownProperty;
    }

    ZenError Ig1CoreProperties::setInt32(ZenProperty_t property, int32_t value) noexcept
    {
        if (!isConstant(property) && !isArray(property) && type(property) == ZenPropertyType_Int32)
        {
            if (property == ZenSensorProperty_BaudRate)
                return m_communicator.setBaudRate(value);
            else
            {
                if (auto streaming = m_imu.getBool(ZenImuProperty_StreamData))
                {
                    if (*streaming)
                        if (auto error = m_imu.setBool(ZenImuProperty_StreamData, false))
                            return error;

                    auto guard = finally([&]() {
                        if (*streaming)
                            m_imu.setBool(ZenImuProperty_StreamData, true);
                    });

                    // Legacy communication protocol only supports uint32_t
                    const uint32_t uiValue = static_cast<uint32_t>(value);

                    const auto function = static_cast<DeviceProperty_t>(base::v1::map(property, false));
                    if (auto error = m_communicator.sendAndWaitForAck(0, function, function, gsl::make_span(reinterpret_cast<const std::byte*>(&uiValue), sizeof(uiValue))))
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

        return ZenError_UnknownProperty;
    }

    bool Ig1CoreProperties::isArray(ZenProperty_t property) const noexcept
    {
        switch (property)
        {
        case ZenSensorProperty_DeviceName:
        case ZenSensorProperty_FirmwareInfo:
        case ZenSensorProperty_FirmwareVersion:
        case ZenSensorProperty_SerialNumber:
        case ZenSensorProperty_SupportedBaudRates:
        case ZenSensorProperty_SensorModel:
            return true;

        default:
            return false;
        }
    }

    bool Ig1CoreProperties::isConstant(ZenProperty_t property) const noexcept
    {
        switch (property)
        {
        case ZenSensorProperty_DeviceName:
        case ZenSensorProperty_FirmwareInfo:
        case ZenSensorProperty_FirmwareVersion:
        case ZenSensorProperty_SerialNumber:
        case ZenSensorProperty_SupportedBaudRates:
        case ZenSensorProperty_BatteryLevel:
        case ZenSensorProperty_BatteryVoltage:
            return true;

        default:
            return false;
        }
    }

    bool Ig1CoreProperties::isExecutable(ZenProperty_t property) const noexcept
    {
        switch (property)
        {
        case ZenSensorProperty_RestoreFactorySettings:
        case ZenSensorProperty_StoreSettingsInFlash:
            return true;

        default:
            return false;
        }
    }

    ZenPropertyType Ig1CoreProperties::type(ZenProperty_t property) const noexcept
    {
        switch (property)
        {
        case ZenSensorProperty_DeviceName:
        case ZenSensorProperty_FirmwareInfo:
        case ZenSensorProperty_SerialNumber:
        case ZenSensorProperty_SensorModel:
            return ZenPropertyType_Byte;

        case ZenSensorProperty_BatteryCharging:
            return ZenPropertyType_Bool;

        case ZenSensorProperty_BatteryLevel:
        case ZenSensorProperty_BatteryVoltage:
            return ZenPropertyType_Float;

        case ZenSensorProperty_FirmwareVersion:
        case ZenSensorProperty_BaudRate:
        case ZenSensorProperty_SupportedBaudRates:
        case ZenSensorProperty_DataMode:
        case ZenSensorProperty_TimeOffset:
            return ZenPropertyType_Int32;

        default:
            return ZenPropertyType_Invalid;
        }
    }

    std::pair<ZenError, size_t> Ig1CoreProperties::supportedBaudRates(gsl::span<std::byte> buffer) const noexcept
    {
        if (auto baudRates = m_communicator.supportedBaudRates())
        {
            if (static_cast<size_t>(buffer.size()) < baudRates->size())
                return std::make_pair(ZenError_BufferTooSmall, baudRates->size());

            if (buffer.data() == nullptr)
                return std::make_pair(ZenError_IsNull, baudRates->size());

            std::memcpy(buffer.data(), baudRates->data(), baudRates->size() * sizeof(int32_t));
            return std::make_pair(ZenError_None, baudRates->size());
        }
        else
        {
            return std::make_pair(baudRates.error(), buffer.size());
        }
    }
}
