#ifndef ZEN_SENSORMANAGER_H_
#define ZEN_SENSORMANAGER_H_

#include <atomic>
#include <mutex>
#include <thread>
#include <type_traits>
#include <set>
#include <unordered_map>

#include "nonstd/expected.hpp"

#include "Sensor.h"
#include "utility/LockingQueue.h"

namespace zen
{
    class SensorManager
    {
    private:
        using key_value_t = std::pair<uintptr_t, std::shared_ptr<Sensor>>;

    public:
        static SensorManager& get();

        SensorManager() noexcept;
        ~SensorManager() noexcept;

        ZenError init() noexcept;
        ZenError deinit() noexcept;

        std::optional<std::shared_ptr<Sensor>> getSensorByToken(uintptr_t token) const noexcept;

        nonstd::expected<key_value_t, ZenSensorInitError> obtain(const ZenSensorDesc& desc) noexcept;

        ZenError release(uintptr_t token) noexcept;

        nonstd::expected<std::vector<ZenSensorDesc>, ZenAsyncStatus> listSensorsAsync(std::optional<std::string_view> typeFilter) noexcept;

        /** Returns true and fills the next event on the queue if there is one, otherwise returns false. */
        std::optional<ZenEvent> pollNextEvent() noexcept;

        /** Returns true and fills the next event on the queue when there is a new one, otherwise returns false upon a call to ZenShutdown() */
        std::optional<ZenEvent> waitForNextEvent() noexcept;

        /** Pushes an event to the event queue */
        void notifyEvent(ZenEvent&& event) noexcept;

    private:
        std::optional<key_value_t> findSensor(const ZenSensorDesc& sensorDesc) noexcept;

        void listSensorLoop();
        void sensorLoop();

        std::vector<ZenSensorDesc> m_devices;

        LockingQueue<ZenEvent> m_eventQueue;
        std::unordered_map<uintptr_t, std::shared_ptr<Sensor>> m_sensors;

        uintptr_t m_nextToken;

        std::thread m_listSensorThread;
        std::thread m_sensorThread;

        std::mutex m_mutex;
        std::atomic_bool m_initialized;

        std::atomic_bool m_startedListing;
        std::atomic_bool m_listing;
        std::atomic_bool m_finished;
        std::atomic_bool m_terminate;
        std::optional<std::string> m_listingTypeFilter;
        ZenError m_listingError;

        bool m_destructing;
    };
}

#endif
