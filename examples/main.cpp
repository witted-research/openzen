#include "OpenZen.h"

#include <array>
#include <atomic>
#include <iostream>
#include <optional>
#include <string>
#include <thread>

#include <gsl/span>

std::atomic_bool g_terminate(false);

using namespace zen;

namespace
{
    std::optional<ZenSensorComponent> getImuComponent(gsl::span<ZenSensorComponent> components)
    {
        for (const auto& component : components)
            if (component.type() == g_zenSensorType_Imu)
                return component;

        return std::nullopt;
    }
}

void pollLoop(ZenClient* const client)
{
    std::cout << "--- Start polling ---" << std::endl;

    while (!g_terminate)
    {
        unsigned int i = 0;
        while (auto event = client->waitForNextEvent())
            if (i++ % 100 == 0)
                std::cout << "Event type: " << event->eventType << std::endl;
    }

    std::cout << "--- Finish polling ---" << std::endl;
}

int main(int argc, char *argv[])
{
    auto [clientError, client] = make_client();
    if (clientError)
        return clientError;

    std::cout << "Listing IMU devices:" << std::endl;

    const auto descriptions = client.listSensors(g_zenSensorType_Imu);
    if (!descriptions)
        return 1;

    for (const auto& desc : *descriptions)
        std::cout << desc.name << " ("  << desc.ioType << ")" << std::endl;

    if (descriptions->empty())
        return 1;

    unsigned int idx;
    do
    {
        std::cout << "Provide an index within the range 0-" << descriptions->size() - 1 << ":" << std::endl;
        std::cin >> idx;
    } while (idx >= descriptions->size());

    auto[obtainError, sensor] = client.obtainSensor(descriptions->at(idx));
    if (obtainError)
        return obtainError;

    const auto optImu = sensor.getAnyComponentOfType(g_zenSensorType_Imu);
    if (!optImu)
        return ZenError_WrongSensorType;

    auto imu = *optImu;

    std::thread pollingThread(&pollLoop, &client);

    // Get a sensor property
    auto [timeError, time] = sensor.getInt32Property(ZenSensorProperty_TimeOffset);
    if (timeError)
    {
        g_terminate = true;
        client.close();
        pollingThread.join();
        return timeError;
    }

    // Get an array property
    std::array<int32_t, 3> version;
    auto [versionError, versionSize] = sensor.getArrayProperty(ZenSensorProperty_FirmwareVersion, version.data(), version.size());
    if (versionError)
    {
        g_terminate = true;
        client.close();
        pollingThread.join();
        return versionError;
    }

    std::cout << "Firmware version: " << version.at(0) << "." << version.at(1) << "." << version.at(2) << std::endl;

    // Do something based on the sensor property
    if (time)
    {
        if (auto error = imu.setBoolProperty(ZenImuProperty_StreamData, true))
        {
            g_terminate = true;
            client.close();
            pollingThread.join();
            return error;
        }
    }

    std::cout << "Type 'q' to quit" << std::endl;

    std::string line;
    while (std::getline(std::cin, line))
    {
        if (line == "q")
            break;
    }

    g_terminate = true;
    client.close();
    pollingThread.join();
    return 0;
}
