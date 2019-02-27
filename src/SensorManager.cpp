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
        void notifyProgress(std::set<std::reference_wrapper<SensorClient>, SensorClientCmp>& subscribers, float progress)
        {
            ZenEvent event{};
            event.eventType = ZenSensorEvent_SensorListingProgress;
            event.data.sensorListingProgress.progress = progress;

            for (auto& subscriber : subscribers)
                subscriber.get().notifyEvent(event);
        }
    }

    SensorManager& SensorManager::get()
    {
        static SensorManager singleton;
        return singleton;
    }

    SensorManager::SensorManager() noexcept
        : m_sensorThread(&SensorManager::sensorLoop, this)
        , m_nextToken(1)
        , m_discovering(false)
        , m_terminate(false)
    {}

    SensorManager::~SensorManager() noexcept
    {
        m_terminate = true;

        if (m_sensorThread.joinable())
            m_sensorThread.join();

        if (m_sensorDiscoveryThread.joinable())
            m_sensorDiscoveryThread.join();
    }

    nonstd::expected<std::shared_ptr<Sensor>, ZenSensorInitError> SensorManager::obtain(const ZenSensorDesc& desc) noexcept
    {
        std::unique_lock<std::mutex> lock(m_sensorsMutex);
        for (auto& pair : m_sensorSubscribers)
            if (pair.first->equals(desc))
                return pair.first;

        lock.unlock();

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

        lock.lock();
        const auto token = m_nextToken++;
        auto sensor = make_sensor(std::move(*agreement), std::move(communicator), token);
        if (!sensor)
            return nonstd::make_unexpected(sensor.error());

        return std::move(*sensor);
    }

    void SensorManager::subscribeToSensor(std::shared_ptr<Sensor> sensor, SensorClient& client) noexcept
    {
        std::lock_guard<std::mutex> lock(m_sensorsMutex);
        auto it = m_sensorSubscribers.find(sensor);
        if (it != m_sensorSubscribers.end())
        {
            it->second.insert(client);
        }
        else
        {
            std::set<std::reference_wrapper<SensorClient>, SensorClientCmp> clients;
            clients.insert(client);

            m_sensorSubscribers.emplace(std::move(sensor), std::move(clients));
        }
    }

    void SensorManager::unsubscribeFromSensor(SensorClient& client, std::shared_ptr<Sensor> sensor) noexcept
    {
        std::lock_guard<std::mutex> lock(m_sensorsMutex);
        auto it = m_sensorSubscribers.find(sensor);
        it->second.erase(client);

        if (it->second.empty())
            m_sensorSubscribers.erase(it);
    }

    void SensorManager::subscribeToSensorDiscovery(SensorClient& client) noexcept
    {
        std::lock_guard<std::mutex> lock(m_discoveryMutex);
        m_discoverySubscribers.insert(client);
        m_discovering = true;
        m_discoveryCv.notify_one();
    }

    void SensorManager::notifyEvent(const ZenEvent& event) noexcept
    {
        auto it = m_sensorSubscribers.find(event.sensor);
        for (auto subscriber : it->second)
            subscriber.get().notifyEvent(event);
    }

    void SensorManager::sensorDiscoveryLoop() noexcept
    {
        while (!m_terminate)
        {
            std::unique_lock<std::mutex> lock(m_discoveryMutex);
            m_discoveryCv.wait(lock, [this]() { return m_discovering || m_terminate; });

            lock.unlock();

            const auto ioSystems = IoManager::get().getIoSystems();
            const auto nIoSystems = ioSystems.size();
            for (size_t idx = 0; idx < nIoSystems; ++idx)
            {
                if (m_terminate)
                    return;

                lock.lock();
                notifyProgress(m_discoverySubscribers, (idx + 0.5f) / nIoSystems);
                lock.unlock();

                try
                {
                    ioSystems[idx].get().listDevices(m_devices);
                }
                catch (...)
                {
                    // [TODO] Make listDevices noexcept and move try-catch block into crashing ioSystem
                    continue;
                }
            }


            lock.lock();
            for (auto& device : m_devices)
            {
                ZenEvent event{};
                event.eventType = ZenSensorEvent_SensorFound;
                event.data.sensorFound = device;

                for (auto& subscriber : m_discoverySubscribers)
                    subscriber.get().notifyEvent(event);
            }

            notifyProgress(m_discoverySubscribers, 1.0f);
            
            m_devices.clear();
            m_discovering = false;
            m_discoverySubscribers.clear();
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
        m_sensorDiscoveryThread = std::thread(&SensorManager::sensorDiscoveryLoop, this);

        while (!m_terminate)
        {
            CanManager::get().poll();
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }

        m_sensorDiscoveryThread.join();
        m_sensorSubscribers.clear();
    }
}
