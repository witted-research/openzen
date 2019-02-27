#include "OpenZen.h"

#include <array>
#include <atomic>
#include <condition_variable>
#include <iostream>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <vector>

#include <gsl/span>

std::vector<ZenSensorDesc> g_discoveredSensors;
std::condition_variable g_discoverCv;
std::mutex g_discoverMutex;
std::atomic_bool g_terminate(false);

using namespace zen;

namespace
{
    void addDiscoveredSensor(const ZenEventData_SensorFound& desc)
    {
        std::lock_guard<std::mutex> lock(g_discoverMutex);
        g_discoveredSensors.push_back(desc);
    }

    std::optional<ZenSensorComponent> getImuComponent(gsl::span<ZenSensorComponent> components)
    {
        for (const auto& component : components)
            if (component.type() == g_zenSensorType_Imu)
                return component;

        return std::nullopt;
    }
}

void pollLoop(std::reference_wrapper<ZenClient> client)
{
    while (!g_terminate)
    {
        unsigned int i = 0;
        while (auto event = client.get().waitForNextEvent())
        {
            if (!event->component.handle)
            {
                switch (event->eventType)
                {
                case ZenSensorEvent_SensorFound:
                    addDiscoveredSensor(event->data.sensorFound);
                    break;

                case ZenSensorEvent_SensorListingProgress:
                    if (event->data.sensorListingProgress.progress == 1.0f)
                        g_discoverCv.notify_one();
                    break;
                }
            }
            else
            {
                switch (event->eventType)
                {
                case ZenImuEvent_Sample:
                    if (i++ % 100 == 0)
                        std::cout << "Event type: " << event->eventType << std::endl;
                    break;
                }
            }
        }
    }

    std::cout << "--- Exit polling thread ---" << std::endl;
}

int main(int argc, char *argv[])
{
    auto [clientError, client] = make_client();
    if (clientError)
        return clientError;

    std::thread pollingThread(&pollLoop, std::ref(client));

    std::cout << "Listing sensors:" << std::endl;

    if (auto error = client.listSensorsAsync())
    {
        g_terminate = true;
        client.close();
        pollingThread.join();
        return error;
    }

    std::unique_lock<std::mutex> lock(g_discoverMutex);
    g_discoverCv.wait(lock);

    if (g_discoveredSensors.empty())
    {
        g_terminate = true;
        client.close();
        pollingThread.join();
        return ZenError_Unknown;
    }

    for (const auto& desc : g_discoveredSensors)
        std::cout << desc.name << " ("  << desc.ioType << ")" << std::endl;

    unsigned int idx;
    do
    {
        std::cout << "Provide an index within the range 0-" << g_discoveredSensors.size() - 1 << ":" << std::endl;
        std::cin >> idx;
    } while (idx >= g_discoveredSensors.size());

    auto[obtainError, sensor] = client.obtainSensor(g_discoveredSensors[idx]);
    if (obtainError)
    {
        g_terminate = true;
        client.close();
        pollingThread.join();
        return obtainError;
    }

    const auto optImu = sensor.getAnyComponentOfType(g_zenSensorType_Imu);
    if (!optImu)
    {
        g_terminate = true;
        client.close();
        pollingThread.join();
        return ZenError_WrongSensorType;
    }

    auto imu = *optImu;

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
