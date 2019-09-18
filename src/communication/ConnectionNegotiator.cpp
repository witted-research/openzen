#include "ConnectionNegotiator.h"

#include <chrono>
#include <gsl/string_span>

#include "spdlog/spdlog.h"
#include "nlohmann/json.hpp"

#include "ZenProtocol.h"
#include "communication/Modbus.h"
#include "utility/Finally.h"
#include "utility/StringView.h"

#include "properties/BaseSensorPropertiesV0.h"

namespace zen
{
    ConnectionNegotiator::ConnectionNegotiator() noexcept :
        m_terminated(false)
    {
        // add all supported sensor types and their configurations
        // only support IG1's Imu yet, second gyroscope and GNSS
        // comes later
        m_sensorConfigs.push_back( { {"LPMS-IG1-CAN", "LPMS-IG1P-CAN",
        "LPMS-IG1-RS232", "LPMS-IG1P-RS232"},
        {1,
          { ComponentConfig{1, g_zenSensorType_Imu}
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

        spdlog::debug("Requesting device name via Ig1 protocol");

        // try to get the sensor's name
        m_terminated = false;

        // disable streaming during connection negotiation, command same for legacy and Ig1
        if (ZenError_None != communicator.send(0, uint8_t(EDevicePropertyV0::SetCommandMode), gsl::span<std::byte>()))
        {
            // command not supported by sensors except ig1, in this case assume its not an ig1
            //return nonstd::make_unexpected(ZenSensorInitError_SendFailed);
            spdlog::info("Cannot set sensor in command mode");
        }
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            m_cv.wait_for(lock, IO_TIMEOUT, [this]() { return m_terminated; });
        }

        // will send command 20, which is SET_IMU_ID for legacy sensors which should be
        // ignored by the firmware because we don't provide the required parameter
        // The reply will be NACK on this for a legacy sensor
        if (ZenError_None != communicator.send(0, uint8_t(EDevicePropertyV1::GetSensorModel), gsl::span<std::byte>()))
        {
            // command not supported by sensors except ig1, in this case assume its not an ig1
            //return nonstd::make_unexpected(ZenSensorInitError_SendFailed);
            spdlog::info("IG1 GetSensorModel not supported, assuming its not an IG1, but a legacy device");
        }
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            m_cv.wait_for(lock, IO_TIMEOUT, [this]() { return m_terminated; });
        }

        if (m_deviceName) {
            spdlog::debug("Device name from Ig1 protocol: {}", *m_deviceName);
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
            spdlog::debug("Found specific device config for sensor name {} and using it",
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
            spdlog::error("No specific configuration for sensor type {} and no fallback configuration found",
                localDeviceName);
            return nonstd::make_unexpected(ZenSensorInitError_NoConfiguration);
        } else {
            spdlog::debug("Using common device config for sensor name {}",
                localDeviceName);
            return itSensorConfigWildcard->second;
        }
    }

    ZenError ConnectionNegotiator::processReceivedData(uint8_t, uint8_t function, gsl::span<const std::byte> data) noexcept
    {
/*        spdlog::debug("ConnectionNegotiator received data with function {} with data size {}",
            function, data.size());*/

        if ((function == ZenProtocolFunction_Handshake) ||
            (function == uint8_t(EDevicePropertyV1::GetSensorModel)) ||
            (function == uint8_t(EDevicePropertyV0::GetDeviceName))) {
            // fine, thats a package we can use
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

        if ((function == uint8_t(EDevicePropertyV1::GetSensorModel)) ||
            (function == uint8_t(EDevicePropertyV0::GetDeviceName))) {
            auto name = std::string(reinterpret_cast<char const*>(data.data()), data.size());
            // device name can have some trailing zeros
            name = util::right_trim(name);
            spdlog::debug("ConnectionNegotiator received device name {}", name);
            m_deviceName = name;
            return ZenError_None;
        }

        return ZenError_None;
    }
}