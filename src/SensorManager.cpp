#include "SensorManager.h"

#include <optional>
#include <QCoreApplication>

#include "ConnectionNegotiator.h"
#include "Sensor.h"
#include "components/ComponentFactoryManager.h"
#include "io/IoManager.h"
#include "io/can/CanManager.h"
#include "utility/StringView.h"

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
        , m_startedListing(false)
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

        m_startedListing = false;
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

    ZenSensorInitError SensorManager::obtain(const ZenSensorDesc* sensorDesc, IZenSensor** outSensor)
    {
        if (sensorDesc == nullptr)
            return ZenSensorInitError_IsNull;

        if (outSensor == nullptr)
            return ZenSensorInitError_IsNull;

        if (auto* sensor = findSensor(sensorDesc))
        {
            *outSensor = sensor;
            return ZenSensorInitError_None;
        }

        ZenSensorInitError ioError;
        auto ioInterface = IoManager::get().obtain(*sensorDesc, ioError);
        if (!ioInterface)
            return ioError;

        ConnectionNegotiator negotiator(*ioInterface);
        auto agreement = negotiator.negotiate();
        if (!agreement)
                return agreement.error();

        auto sensor = make_sensor(std::move(*agreement), std::move(ioInterface));
        if (!sensor)
            return sensor.error();

        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_sensors.emplace(std::move(*sensor));
        *outSensor = it.first->get();

        return ZenSensorInitError_None;
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

    ZenAsyncStatus SensorManager::listSensorsAsync(ZenSensorDesc** outSensors, size_t* outLength, const char* typeFilter)
    {
        if (outLength == nullptr)
            return ZenAsync_InvalidArgument;

        if (m_startedListing.exchange(true))
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
                m_startedListing = false;
                return error;
            }

            return ZenAsync_Updating;
        }

        m_listingTypeFilter = typeFilter ? std::make_optional<std::string>(typeFilter) : std::nullopt;
        m_listing = true;
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
                if (auto error = IoManager::get().listDevices(m_devices))
                {
                    m_listingError = error;
                }
                else if (m_listingTypeFilter)
                {
                    const auto requiredComponents = util::split(*m_listingTypeFilter, ",");
                    for (auto it = m_devices.cbegin(); it != m_devices.cend();)
                    {
                        IZenSensor* sensor = nullptr;
                        if (ZenSensorInitError_None != obtain(&*it, &sensor))
                        {
                            it = m_devices.erase(it);
                            continue;
                        }

                        bool valid = true;
                        for (const std::string_view& type : requiredComponents)
                        {
                            size_t nComponents;
                            sensor->components(nullptr, &nComponents, type.data());
                            if (nComponents == 0)
                            {
                                valid = false;
                                it = m_devices.erase(it);
                                break;
                            }
                        }

                        release(sensor);
                        if (valid)
                            ++it;
                    }
                }

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

        ComponentFactoryManager::get().initialize();
        IoManager::get().initialize();

        // To guarantee initialization of IoManager
        m_listSensorThread = std::thread(&SensorManager::listSensorLoop, this);

        while (!m_terminate)
        {
            CanManager::get().poll();
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
