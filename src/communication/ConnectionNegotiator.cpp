//===========================================================================//
//
// Copyright (C) 2020 LP-Research Inc.
//
// This file is part of OpenZen, under the MIT License.
// See https://bitbucket.org/lpresearch/openzen/src/master/LICENSE for details
// SPDX-License-Identifier: MIT
//
//===========================================================================//

#include "ConnectionNegotiator.h"

#include "spdlog/spdlog.h"

#include "ZenProtocol.h"
#include "communication/Modbus.h"
#include "utility/Finally.h"
#include "utility/StringView.h"

#include "properties/BaseSensorPropertiesV0.h"

#include <gsl/string_span>

#include <chrono>
#include <thread>

namespace zen
{
    ConnectionNegotiator::ConnectionNegotiator() noexcept :
        m_terminated(false)
    {
        // add all supported sensor types and their configurations
        // only support IG1's Imu yet, second gyroscope and GNSS
        // comes later
        m_sensorConfigs.push_back( { {"LPMS-IG1-CAN", "LPMS-IG1-RS232", "LPMS-IG1-RS485"},
        {1,
          { ComponentConfig{1, g_zenSensorType_Imu}
          }
        }
      });

      m_sensorConfigs.push_back( { {"LPMS-IG1P-CAN", "LPMS-IG1P-RS232", "LPMS-IG1P-RS485"},
      {1,
        {
            ComponentConfig{1, g_zenSensorType_Imu},
            ComponentConfig{1, g_zenSensorType_Gnss}
        }
      }
    });

      m_sensorConfigs.push_back( { {"LPMS-BE1"},
      {1,
        {
            ComponentConfig{1, g_zenSensorType_Imu, SpecialOptions_SecondGyroIsPrimary}
        }
      }
    });

      // match every other legacy sensor
      m_sensorConfigs.push_back( { {"*"},
      {0,
        { ComponentConfig{0, g_zenSensorType_Imu}
        }
      }
    });

    }

    nonstd::expected<SensorConfig, ZenSensorInitError> ConnectionNegotiator::negotiate(
      ModbusCommunicator& communicator, unsigned int desiredBaudRate) noexcept
    {
        constexpr const auto IO_TIMEOUT = std::chrono::milliseconds(2000);
        communicator.setBaudRate(desiredBaudRate);

        bool commandModeReply = false;

        // try two times because in some cases, the reply of the first command send to the sensor
        // will not be in the input buffer.
        for (size_t retries = 0; retries < m_connectRetryAttempts; retries++) {
            m_terminated = false;
            spdlog::debug("Attempting to set sensor in command mode for connection negotiaton");
            // wait for some io messages to come in
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
            // disable streaming during connection negotiation, command same for legacy and Ig1
            if (ZenError_None != communicator.send(0, uint8_t(EDevicePropertyV0::SetCommandMode), gsl::span<std::byte>()))
            {
                spdlog::error("Cannot set sensor in command mode");
                return nonstd::make_unexpected(ZenSensorInitError_SendFailed);
            }
            {
                std::unique_lock<std::mutex> lock(m_mutex);
                if (m_cv.wait_for(lock, IO_TIMEOUT, [this]() { return m_terminated; }) == false) {
                    // hit timeout, will retry
                    spdlog::debug("Time out while attempting to set sensor in command mode for connection negotiaton");

                    // reset parser because if the data transmission of the sensor stopped without
                    // sending the full package payload, we might still think we are parsing the payload
                    // while we already get an acknowledgement for our command mode request
                    communicator.resetParser();
                } else {
                    commandModeReply = true;
                    break;
                }
            }
        }

        if (commandModeReply == false) {
            spdlog::error("Time out when setting sensor to command mode before configuration.");
            return nonstd::make_unexpected(ZenSensorInitError_Timeout);
        }

        // will send command 21, which is GET_IMU_ID for legacy sensors. So legacy sensors will return one 32-bit
        // result while its the GET_FIRMWARE_INFO for version 1 sensors, which is a 24-byte long string.
        spdlog::debug("Attempting to query firmware version");
        m_terminated = false;
        if (ZenError_None != communicator.send(0, uint8_t(EDevicePropertyV1::GetFirmwareInfo), gsl::span<std::byte>()))
        {
            // command not supported by sensors except ig1, in this case assume its not an ig1
            //return nonstd::make_unexpected(ZenSensorInitError_SendFailed);
            spdlog::info("IG1 GetSensorModel not supported, assuming its not an IG1, but a legacy device");
        }
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            m_cv.wait_for(lock, IO_TIMEOUT, [this]() { return m_terminated; });
        }

        m_terminated = false;
        if (!m_isLegacy) {
            if (ZenError_None != communicator.send(0, uint8_t(EDevicePropertyV1::GetSensorModel), gsl::span<std::byte>()))
            {
                // command not supported by sensors except ig1, in this case assume its not an ig1
                spdlog::error("Cannot load sensor model from IG1");
                return nonstd::make_unexpected(ZenSensorInitError_SendFailed);
            }
            {
                std::unique_lock<std::mutex> lock(m_mutex);
                m_cv.wait_for(lock, IO_TIMEOUT, [this]() { return m_terminated; });
            }
        }

        if (m_deviceName) {
            spdlog::debug("Device name from Ig1 protocol: {0}", *m_deviceName);
        }

        return loadDeviceConfig();
    }

    nonstd::expected<SensorConfig, ZenSensorInitError> ConnectionNegotiator::loadDeviceConfig() const {
        const std::string localDeviceName = [this]() { if (m_deviceName)
            return *m_deviceName;
        else
            return std::string(""); }();

        // got a name, try to find it in our device config
        auto itSensorConfig = std::find_if(m_sensorConfigs.begin(), m_sensorConfigs.end(),
            [localDeviceName](auto const& config) {
                const auto names = config.first;
                // in any of the names ?
                if (std::find_if(names.begin(), names.end(), [localDeviceName](auto const& name) {
                    return name == localDeviceName;
                } ) != names.end()) {
                    return true;
                } else {
                    return false;
                }
            });

        if (itSensorConfig != m_sensorConfigs.end()) {
            spdlog::debug("Found specific device config for sensor name {0} and using it",
                localDeviceName);
            return itSensorConfig->second;
        }

        // find the wildcard one
        auto itSensorConfigWildcard = std::find_if(m_sensorConfigs.begin(), m_sensorConfigs.end(),
            [](auto const& config) {
                const auto names = config.first;
                if (names.size() > 0) {
                    return names[0] == "*";
                }
                return false;
            });

        if (itSensorConfigWildcard == m_sensorConfigs.end()) {
            spdlog::error("No specific configuration for sensor type {0} and no fallback configuration found",
                localDeviceName);
            return nonstd::make_unexpected(ZenSensorInitError_NoConfiguration);
        } else {
            spdlog::debug("Using common device config for sensor name {0}",
                localDeviceName);
            return itSensorConfigWildcard->second;
        }
    }

    ZenError ConnectionNegotiator::processReceivedData(uint8_t, uint8_t function, gsl::span<const std::byte> data) noexcept
    {
        if ((function == ZenProtocolFunction_Handshake) ||
            (function == uint8_t(EDevicePropertyV1::Ack)) ||
            (function == uint8_t(EDevicePropertyV1::GetFirmwareInfo)) ||
            (function == uint8_t(EDevicePropertyV1::GetSensorModel))) {
            // fine, thats a package we can use during the connection negotiation
        }
        else {
            // don't give an error on unexpected packages to be more tolerant if the
            // sensor was still streaming some data.
            return ZenError_None;
        }

        auto guard = finally([this]() {
            std::unique_lock<std::mutex> lock(m_mutex);
            m_terminated = true;
            lock.unlock();

            m_cv.notify_one();
        });

        if (function == uint8_t(EDevicePropertyV1::GetFirmwareInfo)) {
            // legacy sensor, providing just a 32-bit integer
            spdlog::debug("ConnectionNegotiator received data size {0} when loading the firmware version", data.size());
            if (data.size() == 4) {
                m_isLegacy = true;
                spdlog::debug("ConnectionNegotiator received 32-bit from legacy sensor");
            }
            else {
                auto firmwareInfo = std::string(reinterpret_cast<char const*>(data.data()), data.size());
                spdlog::debug("ConnectionNegotiator loaded firmware Info from Ig1 sensor {0}", firmwareInfo);
                m_isLegacy = false;
            }
        }

        if (function == uint8_t(EDevicePropertyV1::GetSensorModel)) {
            auto name = std::string(reinterpret_cast<char const*>(data.data()), data.size());
            // device name can have some trailing zeros
            name = util::right_trim(name);
            spdlog::debug("ConnectionNegotiator received sensor model {0}", name);
            m_deviceName = name;
            return ZenError_None;
        }

        return ZenError_None;
    }
}