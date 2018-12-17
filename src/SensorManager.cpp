#include "SensorManager.h"

#include <QCoreApplication>

#include "Sensor.h"
#include "io/IoManager.h"
#include "io/can/CanManager.h"

ZEN_API IZenSensorManager* ZenInit(ZenError* outError)
{
    auto& sensorManager = zen::SensorManager::get();
    if (auto error = sensorManager.init())
    {
        *outError = error;
        return nullptr;
    }

    *outError = ZenError_None;
    return &sensorManager;
}

ZEN_API ZenError ZenShutdown()
{
    if (auto error = zen::SensorManager::get().deinit())
        return error;

    return ZenError_None;
}

namespace zen
{
    SensorManager& SensorManager::get()
    {
        static SensorManager singleton;
        return singleton;
    }

    SensorManager::SensorManager()
        : m_initialized(false) 
        , m_listing(false)
        , m_finished(false)
        , m_terminate(false)
        , m_destructing(false)
    {}

    SensorManager::~SensorManager()
    {
        m_destructing = true;
        m_terminate = true;

        if (m_sensorThread.joinable())
            m_sensorThread.join();
    }

    ZenError SensorManager::init()
    {
        if (m_initialized)
            return ZenError_AlreadyInitialized;

        m_listing = false;
        m_finished = false;

        m_terminate = false;
        m_sensorThread = std::thread(&SensorManager::sensorLoop, this);

        m_initialized = true;
        return ZenError_None;
    }

    ZenError SensorManager::deinit()
    {
        if (!m_initialized)
            return ZenError_NotInitialized;

        m_terminate = true;
        m_sensorThread.join();
        m_eventQueue.clear();

        m_initialized = false;
        return ZenError_None;
    }

    ZenError SensorManager::obtain(const ZenSensorDesc* sensorDesc, IZenSensor** outSensor)
    {
        if (sensorDesc == nullptr)
            return ZenError_IsNull;

        if (outSensor == nullptr)
            return ZenError_IsNull;

        if (sensorDesc->sensorType >= ZenSensor_Max)
            return ZenError_WrongSensorType;

        if (auto* sensor = findSensor(sensorDesc))
        {
            *outSensor = sensor;
            return ZenError_None;
        }

        ZenError error;
        auto ioInterface = IoManager::get().obtain(*sensorDesc, error);
        if (!ioInterface)
            return error;

        auto sensor = std::make_unique<Sensor>(std::move(ioInterface));
        Sensor* sensorPtr = sensor.get();
        // Add sensor, to ensure it is being polled
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_sensors.emplace(std::move(sensor));
        }

        if (auto error = sensorPtr->init())
        {
            release(sensorPtr);
            return error;
        }

        *outSensor = sensorPtr;
        return ZenError_None;
    }

    ZenError SensorManager::release(IZenSensor* sensor)
    {
        if (sensor == nullptr)
            return ZenError_IsNull;

        std::lock_guard<std::mutex> lock(m_mutex);

        auto it = m_sensors.find(sensor);
        if (it != m_sensors.end())
            m_sensors.erase(it);

        return ZenError_None;
    }

    bool SensorManager::pollNextEvent(ZenEvent* outEvent)
    {
        if (auto optEvent = m_eventQueue.tryToPop())
        {
            *outEvent = *optEvent;
            return true;
        }

        return false;
    }

    bool SensorManager::waitForNextEvent(ZenEvent* outEvent)
    {
        if (auto optEvent = m_eventQueue.waitToPop())
        {
            *outEvent = *optEvent;
            return true;
        }

        return false;
    }

    ZenAsyncStatus SensorManager::listSensorsAsync(ZenSensorDesc** outSensors, size_t* outLength)
    {
        if (outLength == nullptr)
            return ZenAsync_InvalidArgument;

        if (m_listing.exchange(true))
        {
            if (m_finished.exchange(false))
            {
                if (!m_devices.empty())
                {
                    ZenSensorDesc* sensors = new ZenSensorDesc[m_devices.size()];
                    std::copy(m_devices.cbegin(), m_devices.cend(), sensors);

                    *outSensors = sensors;
                }

                *outLength = m_devices.size();
                m_devices.clear();

                const auto error = m_listingError ? ZenAsync_Failed : ZenAsync_Finished;
                m_listing = false;
                return error;
            }

            return ZenAsync_Updating;
        }

        return ZenAsync_Updating;
    }

    IZenSensor* SensorManager::findSensor(const ZenSensorDesc* sensorDesc)
    {
        for (const auto& sensor : m_sensors)
            if (sensor->equals(sensorDesc))
                return sensor.get();

        return nullptr;
    }

    void SensorManager::listSensorLoop()
    {
        while (!m_terminate)
        {
            if (m_listing && !m_finished)
            {
                m_listingError = IoManager::get().listDevices(m_devices);
                m_finished = true;
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }

    void SensorManager::sensorLoop()
    {
        // Necessary for QBluetooth
        std::unique_ptr<QCoreApplication> app;

        if (QCoreApplication::instance() == nullptr)
        {
            int argv = 0;
            char* argc[]{ nullptr };
            app = std::make_unique<QCoreApplication>(argv, argc);
        }

        IoManager::get().initialize();

        // To guarantee initialization of IoManager
        m_listSensorThread = std::thread(&SensorManager::listSensorLoop, this);

        while (!m_terminate)
        {
            {
                std::lock_guard<std::mutex> lock(m_mutex);

                for (const auto& sensor : m_sensors)
                    sensor->poll();

                CanManager::get().poll();
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }

        m_listSensorThread.join();
        m_sensors.clear();

        IoManager::get().deinitialize();

        // We cannot destruct the QCoreApplication if the main thread no longer exists, which is the case upon static deinitialization
        if (m_destructing)
            app.release();
    }
}
