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
        {uint8_t(0), uint8_t(EDevicePropertyV0::GetDeviceName),
          { util::stringToBuffer("LPMS-CU2") }
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
          { util::stringToBuffer("LPMS-IG1-RS232") }
        }
      }
      );

    auto sensorConfig = negotiator.negotiate(mockbus, 57600);
    ASSERT_TRUE(sensorConfig);
    ASSERT_EQ(1, sensorConfig->version);
    ASSERT_EQ(2, sensorConfig->components.size());
    ASSERT_EQ(1, sensorConfig->components[0].version);
    ASSERT_EQ(g_zenSensorType_ImuIg1, sensorConfig->components[0].id);
    ASSERT_EQ(1, sensorConfig->components[1].version);
    ASSERT_EQ(g_zenSensorType_Gnss, sensorConfig->components[1].id);
}
