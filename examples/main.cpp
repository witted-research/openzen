#include "IZenSensor.h"
#include "IZenSensorManager.h"

#include <atomic>
#include <iostream>
#include <string>
#include <thread>

std::atomic_bool g_terminate(false);

void pollLoop(IZenSensorManager* manager)
{
    std::cout << "--- Start polling ---" << std::endl;

    ZenEvent event;
    while (!g_terminate)
    {
        unsigned int i = 0;
        while (manager->waitForNextEvent(&event))
            if (i++ % 100 == 0)
                std::cout << "Event type: " << event.eventType << std::endl;
    }

    std::cout << "--- Finish polling ---" << std::endl;
}

int main(int argc, char *argv[])
{
    ZenError error;
    IZenSensorManager* manager = ZenInit(&error);
    if (error)
        return error;

    std::cout << "Listing devices:" << std::endl;

    ZenSensorDesc* list = nullptr;
    size_t nSensors;
    while (auto status = manager->listSensorsAsync(&list, &nSensors))
    {
        if (status != ZenAsync_Updating)
        {
            ZenShutdown();
            return error;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    for (int i = 0; i < nSensors; ++i)
        std::cout << i << ": " << list[i].name << " (" << list[i].sensorType << "/" << list[i].ioType << ")" << std::endl;

    if (nSensors == 0)
    {
        ZenShutdown();
        return 1;
    }

    unsigned int idx;
    do
    {
        std::cout << "Provide an index within the range 0-" << nSensors - 1 << ":" << std::endl;
        std::cin >> idx;
    } while (idx >= nSensors);

    IZenSensor* sensor = nullptr;
    if (auto error = manager->obtain(&list[idx], &sensor))
    {
        ZenShutdown();
        return error;
    }

    std::thread pollingThread(&pollLoop, manager);

    // Turn streaming off, so we can send commands
    if (auto error = sensor->setBoolDeviceProperty(ZenImuProperty_StreamData, false))
    {
        g_terminate = true;
        ZenShutdown();
        pollingThread.join();
        return error;
    }

    // Get a sensor property
    int32_t time;
    if (auto error = sensor->getInt32DeviceProperty(ZenSensorProperty_TimeOffset, &time))
    {
        g_terminate = true;
        ZenShutdown();
        pollingThread.join();
        return error;
    }

    // Do something based on the sensor property
    if (time)
    {
        if (auto error = sensor->setBoolDeviceProperty(ZenImuProperty_StreamData, true))
        {
            g_terminate = true;
            ZenShutdown();
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
    ZenShutdown();
    pollingThread.join();
    return 0;
}