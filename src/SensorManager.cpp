//===========================================================================//
//
// Copyright (C) 2020 LP-Research Inc.
//
// This file is part of OpenZen, under the MIT License.
// See https://bitbucket.org/lpresearch/openzen/src/master/LICENSE for details
// SPDX-License-Identifier: MIT
//
//===========================================================================//

#include "SensorManager.h"

#include "Sensor.h"
#include "communication/ConnectionNegotiator.h"
#include "communication/EventCommunicator.h"
#include "components/ComponentFactoryManager.h"
#include "io/IoManager.h"
#include "io/can/CanManager.h"
#include "utility/StringView.h"

#include <spdlog/spdlog.h>

namespace zen
{
    namespace
    {
        void notifyProgress(std::set<std::reference_wrapper<SensorClient>, ReferenceWrapperCmp<SensorClient>>& subscribers, float progress)
        {
            ZenEvent event{};
            event.eventType = ZenEventType_SensorListingProgress;
            event.data.sensorListingProgress.progress = progress;
            event.data.sensorListingProgress.complete = progress == 1.0f;

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
        : m_nextToken(1)
        , m_discovering(false)
        , m_terminate(false)
        , m_sensorThread(&SensorManager::sensorLoop, this)
        , m_sensorDiscoveryThread(&SensorManager::sensorDiscoveryLoop, this)   
    {
#ifdef ZEN_BLUETOOTH_BLE
        // Necessary for QBluetooth
        if (QCoreApplication::instance() == nullptr)
        {
            int argv = 0;
            char* argc[]{ nullptr };
            m_app.reset(new QCoreApplication(argv, argc));
        }
#endif

        ComponentFactoryManager::get().initialize();
        IoManager::get().initialize();
    }

    SensorManager::~SensorManager() noexcept
    {
        m_terminate = true;
        m_discoveryCv.notify_all();

        if (m_sensorDiscoveryThread.joinable())
            m_sensorDiscoveryThread.join();

        if (m_sensorThread.joinable())
            m_sensorThread.join();
    }

    nonstd::expected<std::shared_ptr<Sensor>, ZenSensorInitError> SensorManager::obtain(const ZenSensorDesc& const_desc) noexcept
    {
        std::unique_lock<std::mutex> lock(m_sensorsMutex);
        ZenSensorDesc desc = const_desc;

        for (const auto& sensor : m_sensors)
            if (sensor->equals(desc))
                return sensor;

        lock.unlock();

        auto ioSystem = IoManager::get().getIoSystem(desc.ioType);
        if (!ioSystem) {
            spdlog::error("IoType {0} not supported", desc.ioType);
            return nonstd::make_unexpected(ZenSensorInitError_UnsupportedIoType);
        }

        if (!ioSystem.value().get().isHighLevel()) {
            ConnectionNegotiator negotiator;
            auto communicator = std::make_unique<ModbusCommunicator>(negotiator,
                std::make_unique<modbus::LpFrameFactory>(), std::make_unique<modbus::LpFrameParser>());

            // load the default baud rate, if needed
            if (desc.baudRate == 0) {
                desc.baudRate = ioSystem->get().getDefaultBaudrate();
            }
            spdlog::info("Obtaining sensor {0} with baudrate {1}", desc.identifier, desc.baudRate);

            if (auto ioInterface = ioSystem->get().obtain(desc, *communicator.get())) {
                communicator->init(std::move(*ioInterface));
            } else {
                spdlog::error("IO System returned error");
                return nonstd::make_unexpected(ioInterface.error());
            }

            auto agreement = negotiator.negotiate(*communicator.get(), desc.baudRate);
            if (!agreement) {
                spdlog::error("Sensor connection cannot be negotiated");
                return nonstd::make_unexpected(agreement.error());
            }

            lock.lock();
            const auto token = m_nextToken++;
            lock.unlock();

            auto sensor = make_sensor(std::move(*agreement), std::move(communicator), token);
            if (!sensor) {
                spdlog::error("Sensor object cannot be created");
                return nonstd::make_unexpected(sensor.error());
            }

            lock.lock();
            m_sensors.insert(*sensor);
            lock.unlock();

            return std::move(*sensor);
        }
        else {
            lock.lock();
            const auto token = m_nextToken++;
            lock.unlock();

            auto eventCom = std::make_unique<EventCommunicator>();
            if (auto ioInterface = ioSystem->get().obtainEventBased(desc, *(eventCom.get())))
                eventCom->init(std::move(*ioInterface));
            else
                return nonstd::make_unexpected(ioInterface.error());

            // high level sensors receive events directly and need no negogiators/communicators
            SensorConfig conf;
            conf.version = 1;
            auto sensor = make_high_level_sensor(conf, std::move(eventCom), token);
            if (!sensor)
                return nonstd::make_unexpected(sensor.error());

            lock.lock();
            m_sensors.insert(*sensor);
            lock.unlock();

            return std::move(*sensor);
        }
    }

    std::shared_ptr<Sensor> SensorManager::release(ZenSensorHandle_t sensorHandle) noexcept
    {
        std::lock_guard<std::mutex> lock(m_sensorsMutex);
        auto it = m_sensors.find(sensorHandle);

        const auto sensor = *it;
        m_sensors.erase(it);
        return sensor;
    }

    void SensorManager::subscribeToSensorDiscovery(SensorClient& client) noexcept
    {
        std::lock_guard<std::mutex> lock(m_discoveryMutex);
        m_discoverySubscribers.insert(client);
        m_discovering = true;
        m_discoveryCv.notify_one();
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
                event.eventType = ZenEventType_SensorFound;
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
        while (!m_terminate)
        {
            CanManager::get().poll();
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }
}
