//===========================================================================//
//
// Copyright (C) 2020 LP-Research Inc.
//
// This file is part of OpenZen, under the MIT License.
// See https://bitbucket.org/lpresearch/openzen/src/master/LICENSE for details
// SPDX-License-Identifier: MIT
//
//===========================================================================//

#include <gtest/gtest.h>

#include "ZenTypes.h"
#include "InternalTypes.h"
#include "communication/ConnectionNegotiator.h"
#include "utility/StringView.h"

#include "MockbusCommunicator.h"

using namespace zen;

TEST(ConnectionNegotiator, connectLegacySensor) {
    ConnectionNegotiator negotiator;

    MockbusCommunicator mockbus(negotiator,
      { // vector
        // entry 1: pair
        {uint8_t(0), uint8_t(EDevicePropertyV1::GetFirmwareInfo),
          // Legacy sensors will answer with the IMU id (32-bit) integer
          // because idendifier 21 is GET_IMU_ID
          uint8_t(EDevicePropertyV1::GetFirmwareInfo),
          { std::byte(0), std::byte(0), std::byte(0), std::byte(23) }
        },
        {uint8_t(0), uint8_t(EDevicePropertyV0::SetCommandMode),
            uint8_t(EDevicePropertyV0::Ack),
            {}
        }
       }
      );

    auto sensorConfig = negotiator.negotiate(mockbus, 57600);
    ASSERT_TRUE(sensorConfig);
    ASSERT_EQ(0, sensorConfig->version);
    ASSERT_EQ(1, sensorConfig->components.size());
    ASSERT_EQ(0, sensorConfig->components[0].version);
    ASSERT_EQ(g_zenSensorType_Imu, sensorConfig->components[0].id);
}

TEST(ConnectionNegotiator, connectIg1Sensor) {
    ConnectionNegotiator negotiator;

    MockbusCommunicator mockbus(negotiator,
      { // vector
        // entry 1: pair
        {uint8_t(0), uint8_t(EDevicePropertyV1::GetSensorModel),
            uint8_t(EDevicePropertyV1::GetSensorModel),
          { util::stringToBuffer("LPMS-IG1-RS232") }
        },
        // new sensors will return 24-byte char
        {uint8_t(0), uint8_t(EDevicePropertyV1::GetFirmwareInfo),
            uint8_t(EDevicePropertyV1::GetFirmwareInfo),
          { util::stringToBuffer("v-10-100-100-100-100-100") }
        },
        {uint8_t(0), uint8_t(EDevicePropertyV0::SetCommandMode),
            uint8_t(EDevicePropertyV0::Ack),
            {}
        }
      }
      );

    auto sensorConfig = negotiator.negotiate(mockbus, 57600);
    ASSERT_TRUE(sensorConfig);
    ASSERT_EQ(1, sensorConfig->version);
    ASSERT_EQ(1, sensorConfig->components.size());
    ASSERT_EQ(1, sensorConfig->components[0].version);
    ASSERT_EQ(g_zenSensorType_Imu, sensorConfig->components[0].id);
    /* renable this once the GNSS has been implemented
    ASSERT_EQ(1, sensorConfig->components[1].version);
    ASSERT_EQ(g_zenSensorType_Gnss, sensorConfig->components[1].id);
    */
}
