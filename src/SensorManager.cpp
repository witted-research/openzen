#include "SensorManager.h"

#include <optional>
#include <QCoreApplication>

#include "Sensor.h"
#include "communication/ConnectionNegotiator.h"
#include "components/ComponentFactoryManager.h"
#include "io/IoManager.h"
#include "io/can/CanManager.h"
#include "utility/StringView.h"

namespace zen
{
    namespace
    {
        bool hasComponentOfType(const std::vector<std::unique_ptr<SensorComponent>>& components, std::string_view type)
        {
            for (const auto& component : components)
                if (component->type() == type)
                    return true;

            return false;
        }
    }

    SensorManager& SensorManager::get()
    {
        static SensorManager singleton;
        return singleton;
    }

    SensorManager::SensorManager() noexcept
        : m_initialized(false) 
        , m_startedListing(false)
        , m_listing(false)
        , m_finished(false)
        , m_terminate(false)
        , m_destructing(false)
        , m_nextToken(1)
    {}

    SensorManager::~SensorManager() noexcept
    {
        m_destructing = true;
        m_terminate = true;

        if (m_sensorThread.joinable())
            m_sensorThread.join();

        if (m_listSensorThread.joinable())
            m_listSensorThread.join();
    }

    ZenError SensorManager::init() noexcept
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

    ZenError SensorManager::deinit() noexcept
    {
        if (!m_initialized)
            return ZenError_NotInitialized;

        m_terminate = true;
        m_sensorThread.join();
        m_eventQueue.clear();

        m_initialized = false;
        return ZenError_None;
    }

    std::optional<std::shared_ptr<Sensor>> SensorManager::getSensorByToken(uintptr_t token) const noexcept
    {
        auto it = m_sensors.find(token);
        if (it == m_sensors.cend())
            return std::nullopt;

        return it->second;
    }

    nonstd::expected<SensorManager::key_value_t, ZenSensorInitError> SensorManager::obtain(const ZenSensorDesc& desc) noexcept
    {
        if (auto sensor = findSensor(desc))
            return std::move(*sensor);

        auto ioSystem = IoManager::get().getIoSystem(desc.ioType);
        if (!ioSystem)
            return nonstd::make_unexpected(ZenSensorInitError_UnsupportedIoType);

        ConnectionNegotiator negotiator;
        auto communicator = std::make_unique<ModbusCommunicator>(negotiator, std::make_unique<modbus::RTUFrameFactory>(), std::make_unique<modbus::RTUFrameParser>());

        if (auto ioInterface = ioSystem->get().obtain(desc, *communicator.get()))
            communicator->init(std::move(*ioInterface));
        else
            return nonstd::make_unexpected(ioInterface.error());

        auto agreement = negotiator.negotiate(*communicator.get());
        if (!agreement)
            return nonstd::make_unexpected(agreement.error());

        std::lock_guard<std::mutex> lock(m_mutex);
        const auto token = m_nextToken++;
        auto sensor = make_sensor(std::move(*agreement), std::move(communicator), token);
        if (!sensor)
            return nonstd::make_unexpected(sensor.error());

        auto it = m_sensors.emplace(token, std::move(*sensor));
        return *it.first;
    }

    ZenError SensorManager::release(uintptr_t token) noexcept
    {
        std::lock_guard<std::mutex> lock(m_mutex);

        auto it = m_sensors.find(token);
        if (it != m_sensors.end())
            m_sensors.erase(it);

        return ZenError_None;
    }

    nonstd::expected<std::vector<ZenSensorDesc>, ZenAsyncStatus> SensorManager::listSensorsAsync(std::optional<std::string_view> typeFilter) noexcept
    {
        if (m_startedListing.exchange(true))
        {
            if (m_finished.exchange(false))
            {
                const auto expected = m_listingError
                    ? nonstd::make_unexpected(ZenAsync_Failed)
                    : nonstd::expected<std::vector<ZenSensorDesc>, ZenAsyncStatus>(std::move(m_devices));

                m_listing = false;
                m_startedListing = false;
                return std::move(expected);
            }

            return nonstd::make_unexpected(ZenAsync_Updating);
        }

        m_listingTypeFilter = typeFilter;
        m_listing = true;
        return nonstd::make_unexpected(ZenAsync_Updating);
    }

    std::optional<ZenEvent> SensorManager::pollNextEvent() noexcept
    {
        return m_eventQueue.tryToPop();
    }

    std::optional<ZenEvent> SensorManager::waitForNextEvent() noexcept
    {
        return m_eventQueue.waitToPop();
    }

    void SensorManager::notifyEvent(ZenEvent&& event) noexcept
    {
        m_eventQueue.push(std::move(event));
    }

    std::optional<SensorManager::key_value_t> SensorManager::findSensor(const ZenSensorDesc& sensorDesc) noexcept
    {
        for (const auto& sensor : m_sensors)
            if (sensor.second->equals(sensorDesc))
                return sensor;

        return std::nullopt;
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
                        auto sensor = obtain(*it);
                        if (!sensor)
                        {
                            it = m_devices.erase(it);
                            continue;
                        }

                        const auto& components = sensor->second->components();

                        bool valid = true;
                        for (const std::string_view& type : requiredComponents)
                        {
                            if (!hasComponentOfType(components, type))
                            {
                                it = m_devices.erase(it);
                                valid = false;
                                break;
                            }
                        }

                        if (valid)
                            ++it;

                        release(sensor->first);
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
