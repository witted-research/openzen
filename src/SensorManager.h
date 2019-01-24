#ifndef ZEN_SENSORMANAGER_H_
#define ZEN_SENSORMANAGER_H_

#include <atomic>
#include <mutex>
#include <thread>
#include <type_traits>
#include <set>

#include "IZenSensorManager.h"
#include "Sensor.h"
#include "utility/LockingQueue.h"

namespace zen
{
    template <typename T>
    struct PointerCmp
    {
        using is_transparent = std::true_type;

        struct UniversalPtr
        {
            T* ptr;

            UniversalPtr() = default;
            UniversalPtr(const UniversalPtr&) = default;

            template <typename U>
            UniversalPtr(U* p) : ptr(p)
            { static_assert(std::is_base_of<T, U>()); }

            template <typename U, class... Args>
            UniversalPtr(const std::unique_ptr<U, Args...>& p) : ptr(p.get())
            { static_assert(std::is_base_of<T, U>()); }

            template <typename U>
            UniversalPtr(const std::shared_ptr<U>& p) : ptr(p.get())
            { static_assert(std::is_base_of<T, U>()); }

            bool operator<(UniversalPtr rhs) const { return std::less<T*>()(ptr, rhs.ptr); }
        };

        bool operator()(const UniversalPtr&& lhs, const UniversalPtr&& rhs) const { return lhs < rhs; }
    };

    class SensorManager : public IZenSensorManager
    {
    public:
        static SensorManager& get();

        SensorManager();
        ~SensorManager();

        ZenError init();
        ZenError deinit();

        ZenError obtain(const ZenSensorDesc* sensorDesc, IZenSensor** outSensor) override;

        ZenError release(IZenSensor* sensor) override;

        /** Returns true and fills the next event on the queue if there is one, otherwise returns false. */
        bool pollNextEvent(ZenEvent* outEvent) override;

        /** Returns true and fills the next event on the queue when there is a new one, otherwise returns false upon a call to ZenShutdown() */
        bool waitForNextEvent(ZenEvent* outEvent) override;

        ZenAsyncStatus listSensorsAsync(ZenSensorDesc** outSensors, size_t* outLength, const char* typeFilter) override;

        std::pair<ZenError, Sensor*> makeSensor(std::unique_ptr<BaseIoInterface> ioInterface);

        /** Pushes an event to the event queue */
        void notifyEvent(ZenEvent&& event) { m_eventQueue.push(std::move(event)); }

    private:
        IZenSensor* findSensor(const ZenSensorDesc* sensorDesc);

        void listSensorLoop();
        void sensorLoop();

        LockingQueue<ZenEvent> m_eventQueue;
        std::set<std::unique_ptr<Sensor>, PointerCmp<IZenSensor>> m_sensors;

        std::vector<ZenSensorDesc> m_devices;

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
