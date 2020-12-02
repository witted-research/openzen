//===========================================================================//
//
// Copyright (C) 2020 LP-Research Inc.
//
// This file is part of OpenZen, under the MIT License.
// See https://bitbucket.org/lpresearch/openzen/src/master/LICENSE for details
// SPDX-License-Identifier: MIT
//
//===========================================================================//

#include "OpenZen.h"

#include <array>
#include <atomic>
#include <condition_variable>
#include <iostream>
#include <mutex>
#include <limits>
#include <string>
#include <thread>
#include <vector>
std::vector<ZenSensorDesc> g_discoveredSensors;
std::condition_variable g_discoverCv;
std::mutex g_discoverMutex;
std::atomic_bool g_terminate(false);

std::atomic<uintptr_t> g_imuHandle;
std::atomic<uintptr_t> g_gnssHandle;

using namespace zen;

void pollLoop(std::reference_wrapper<ZenClient> client)
{
    while (!g_terminate)
    {
        unsigned int i = 0;
        while (true)
        {
            const auto pair = client.get().waitForNextEvent();
            const bool success = pair.first;
            auto& event = pair.second;
            if (!success)
                break;
            if (( g_imuHandle > 0) && (event.component.handle == g_imuHandle))
            {
                switch (event.eventType)
                {
                case ZenEventType_ImuData:
                    if (i++ % 10 == 0) {
                        std::cout << " > Yaw: = " << event.data.imuData.r[2] << std::endl;
                    }
                    break;
                }
            }
        }
    }
}

int main(int argc, char *argv[])
{
    if ((argc > 1) && (std::string(argv[1]) == "debug")) {
        std::cout << "Debug output enabled" << std::endl;
        ZenSetLogLevel(ZenLogLevel_Debug);
    } else {
        ZenSetLogLevel(ZenLogLevel_Info);
    }

    g_imuHandle = 0;
    g_gnssHandle = 0;

    auto clientPair = make_client();
    auto& clientError = clientPair.first;
    auto& client = clientPair.second;

    if (clientError)
        return clientError;

    std::thread pollingThread(&pollLoop, std::ref(client));

    auto sensorPair = client.obtainSensorByName("LinuxDevice", "0001");
    auto& obtainError = sensorPair.first;
    auto& sensor = sensorPair.second;
    if (obtainError)
    {
        g_terminate = true;
        client.close();
        pollingThread.join();
        return obtainError;
    }

    auto imuPair = sensor.getAnyComponentOfType(g_zenSensorType_Imu);
    auto& hasImu = imuPair.first;
    auto imu = imuPair.second;

    if (!hasImu)
    {
        g_terminate = true;
        client.close();
        pollingThread.join();
        return ZenError_WrongSensorType;
    }

    // store the handle to the IMU to identify data coming from the imu
    // in our data processing thread
    g_imuHandle = imu.component().handle;

    // Get a string property
    auto sensorModelPair = sensor.getStringProperty(ZenSensorProperty_SensorModel);
    auto & sensorModelError = sensorModelPair.first;
    auto & sensorModelName = sensorModelPair.second;
    if (sensorModelError) {
        g_terminate = true;
        client.close();
        pollingThread.join();
        return sensorModelError;
    }
    std::cout << "Sensor Model: " << sensorModelName << std::endl;

    // Enable sensor streaming
    if (auto error = imu.setBoolProperty(ZenImuProperty_StreamData, true))
    {
        g_terminate = true;
        client.close();
        pollingThread.join();
        return error;
    }

    while (!g_terminate)
    {

    }
    client.close();
    pollingThread.join();
    return 0;
}
