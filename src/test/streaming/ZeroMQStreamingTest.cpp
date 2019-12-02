#include <gtest/gtest.h>

#include "OpenZen.h"
#include "streaming/StreamingProtocol.h"
#include "streaming/ZenTypesSerialization.h"

#include <cereal/cereal.hpp>
#include <cereal/archives/binary.hpp>
#include <sstream>

TEST(ZeroMQStreming, acquireAndRelease) {
    // create high-level sensor
    auto client = zen::make_client();
    auto remoteSensor = client.second.obtainSensorByName("ZeroMQ", "tcp://localhost:8899" );

    ASSERT_EQ(ZenError_None, remoteSensor.first);
    client.second.releaseSensor(remoteSensor.second);
}

TEST(ZeroMQStreming, acquireInvalidHostAndProtocol) {
    // create high-level sensor with non-existing host
    {
        auto client = zen::make_client();
        auto remoteSensor = client.second.obtainSensorByName("ZeroMQ", "tcp://non_existent_host:8899");

        ASSERT_EQ(ZenError_None, remoteSensor.first);
    }

    // unsupported protocoll
    {
        auto client = zen::make_client();
        auto remoteSensor = client.second.obtainSensorByName("ZeroMQ", "abc://non_existent_host:8899");

        ASSERT_EQ(ZenSensorInitError_ConnectFailed, remoteSensor.first);
    }

    // totally wrong URL
    {
        auto client = zen::make_client();
        auto remoteSensor = client.second.obtainSensorByName("ZeroMQ", "!?-/:){}!-:");

        ASSERT_EQ(ZenSensorInitError_ConnectFailed, remoteSensor.first);
    }
}


TEST(ZeroMQStreming, wrongPublishUrl) {
    // create high-level sensor
    const std::string publishUrl = "!?-/:){}!-:";

    // create a test sensor to publish
    auto localClient = zen::make_client();
    auto localTestSensor = localClient.second.obtainSensorByName("TestSensor", "").second;

    auto publishResult = localTestSensor.publishEvents(publishUrl);

}

// todo cleanup and test

TEST(ZeroMQStreaming, sendAndReceive) {
    // create high-level sensor
    const std::string obtainUrl = "tcp://localhost:8899";
    const std::string publishUrl = "tcp://*:8899";
    auto remoteClient = zen::make_client();
    auto remoteSensor = remoteClient.second.obtainSensorByName("ZeroMQ", obtainUrl).second;

    // create a test sensor to publish
    auto localClient = zen::make_client();
    auto localTestSensor = localClient.second.obtainSensorByName("TestSensor", "").second;

    localTestSensor.publishEvents(publishUrl);

    // wait for some events to arrive
    std::this_thread::sleep_for(std::chrono::seconds(1));
    // check if something arrived at the remote sensor via network
    auto sensorData = remoteClient.second.pollNextEvent();
    ASSERT_TRUE(sensorData.has_value());
}

TEST(ZeroMQStreaming, parseImuMessage) {
    std::stringstream buffer;

    zen::Serialization::ZenEventImuSerialization imuData;

    imuData.sensor = 3;
    imuData.component = 4;
    imuData.data.a[0] = 23.0f;
    imuData.data.w[0] = 5.0f;

    {
        cereal::BinaryOutputArchive imu_archive(buffer);
        imu_archive(imuData);
    }
    // header has 4 bytes
    std::vector<std::byte> completeBuffer;
    completeBuffer.push_back(std::byte(0));
    completeBuffer.push_back(std::byte(0));
    completeBuffer.push_back(std::byte(0));
    completeBuffer.push_back(std::byte(zen::Streaming::StreamingMessageType_ZenEventImu));

    auto sBuffer = buffer.str();

    for (auto s : sBuffer) {
        completeBuffer.push_back(std::byte(s));
    }

    zmq::message_t msg(completeBuffer.data(), completeBuffer.size());

    auto unpackedMessage = zen::Streaming::fromZmqMessage(msg);
    ASSERT_TRUE(unpackedMessage.has_value());
    ASSERT_EQ(unpackedMessage->type, zen::Streaming::StreamingMessageType_ZenEventImu);
    ASSERT_EQ(unpackedMessage->payload.imuData.data.a[0], 23.0f);
    ASSERT_EQ(unpackedMessage->payload.imuData.data.w[0], 5.0f);
    ASSERT_EQ(unpackedMessage->payload.imuData.sensor, 3);
    ASSERT_EQ(unpackedMessage->payload.imuData.component, 4);
}


TEST(ZeroMQStreaming, parseGnssMessage) {
    std::stringstream buffer;

    zen::Serialization::ZenEventGnssSerialization gnssData;

    gnssData.sensor = 3;
    gnssData.component = 4;
    gnssData.data.longitude = 23.0f;
    gnssData.data.latitude = 5.0f;

    {
        cereal::BinaryOutputArchive imu_archive(buffer);
        imu_archive(gnssData);
    }
    // header has 4 bytes
    std::vector<std::byte> completeBuffer;
    completeBuffer.push_back(std::byte(0));
    completeBuffer.push_back(std::byte(0));
    completeBuffer.push_back(std::byte(0));
    completeBuffer.push_back(std::byte(zen::Streaming::StreamingMessageType_ZenEventGnss));

    auto sBuffer = buffer.str();

    for (auto s : sBuffer) {
        completeBuffer.push_back(std::byte(s));
    }

    zmq::message_t msg(completeBuffer.data(), completeBuffer.size());

    auto unpackedMessage = zen::Streaming::fromZmqMessage(msg);
    ASSERT_TRUE(unpackedMessage.has_value());
    ASSERT_EQ(unpackedMessage->type, zen::Streaming::StreamingMessageType_ZenEventGnss);
    ASSERT_EQ(unpackedMessage->payload.gnssData.data.longitude, 23.0f);
    ASSERT_EQ(unpackedMessage->payload.gnssData.data.latitude, 5.0f);
    ASSERT_EQ(unpackedMessage->payload.gnssData.sensor, 3);
    ASSERT_EQ(unpackedMessage->payload.gnssData.component, 4);
}
