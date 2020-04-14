//===========================================================================//
//
// Copyright (C) 2020 LP-Research Inc.
//
// This file is part of OpenZen, under the MIT License.
// See https://bitbucket.org/lpresearch/openzen/src/master/LICENSE for details
// SPDX-License-Identifier: MIT
//
//===========================================================================//

#ifndef ZEN_COMMUNICATION_CONNECTIONNEGOTIATOR_H_
#define ZEN_COMMUNICATION_CONNECTIONNEGOTIATOR_H_

#include <condition_variable>
#include <mutex>
#include <vector>
#include <utility>

#include "nonstd/expected.hpp"

#include "SensorConfig.h"
#include "communication/ModbusCommunicator.h"
#include "io/IoManager.h"

namespace zen
{
    /*
    This class queries the connected sensor and tries to determines which type of sensor is connected 
    and which components it provides. Currently, I implements a special case for the Ig1's two gyros
    and GNSS component.
    */
    class ConnectionNegotiator : public IModbusFrameSubscriber
    {
    public:
        ConnectionNegotiator() noexcept;

        /** Try to determine the appropriate baudrate, and when done negotiate
            the configuration of the sensor. */
        nonstd::expected<SensorConfig, ZenSensorInitError> negotiate(ModbusCommunicator& communicator,
          unsigned int desiredBaudRate) noexcept;

    private:
        ZenError processReceivedData(uint8_t address, uint8_t function,
          gsl::span<const std::byte> data) noexcept override;

    private:
        nonstd::expected<SensorConfig, ZenSensorInitError> loadDeviceConfig() const;

        SensorConfig m_config;
        bool m_terminated;
        std::optional<std::string> m_deviceName;

        std::vector<std::pair<std::vector<std::string>, SensorConfig >> m_sensorConfigs;

        mutable std::condition_variable m_cv;
        mutable std::mutex m_mutex;
        bool m_isLegacy = true;
        const size_t m_connectRetryAttempts = 2;
    };
}

#endif
