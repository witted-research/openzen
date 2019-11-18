#include <gtest/gtest.h>

#include "OpenZen.h"


TEST(ZeroMQStreming, sendEvent) {
    // create high-level sensor
    auto client = zen::make_client();
    auto remoteSensor = client.second.obtainSensorByName("ZeroMQ", "tcp://localhost:8899" );

    ASSERT_EQ(ZenError_None, remoteSensor.first);

    client.second.releaseSensor(remoteSensor.second);
}