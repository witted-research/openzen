#ifndef ZEN_SENSORMANAGER_H_
#define ZEN_SENSORMANAGER_H_

#include <atomic>
#include <condition_variable>
#include <map>
#include <mutex>
#include <set>
#include <thread>
#include <type_traits>

#include "nonstd/expected.hpp"

#include "Sensor.h"
#include "SensorClient.h"

namespace zen
{
    class SensorManager
    {
    public:
        static SensorManager& get();

        SensorManager() noexcept;
        ~SensorManager() noexcept;

        nonstd::expected<std::shared_ptr<Sensor>, ZenSensorInitError> obtain(const ZenSensorDesc& desc) noexcept;

        void subscribeToSensor(std::shared_ptr<Sensor> sensor, SensorClient& client) noexcept;

        void subscribeToSensorDiscovery(SensorClient& client) noexcept;

        void unsubscribeFromSensor(SensorClient& client, std::shared_ptr<Sensor> sensor) noexcept;

        /** Pushes an event to subscribed clients */
        void notifyEvent(const ZenEvent& event) noexcept;

    private:
        void sensorDiscoveryLoop() noexcept;
        void sensorLoop();

        std::map<std::shared_ptr<Sensor>, std::set<std::reference_wrapper<SensorClient>, SensorClientCmp>, SensorCmp> m_sensorSubscribers;
        std::set<std::reference_wrapper<SensorClient>, SensorClientCmp> m_discoverySubscribers;

        std::vector<ZenSensorDesc> m_devices;

        std::thread m_sensorThread;
        std::thread m_sensorDiscoveryThread;

        std::condition_variable m_discoveryCv;

        std::mutex m_sensorsMutex;
        std::mutex m_discoveryMutex;

        uintptr_t m_nextToken;
        bool m_discovering;
        
        std::atomic_bool m_terminate;
    };
}

#endif
