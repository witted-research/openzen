//===========================================================================//
//
// Copyright (C) 2020 LP-Research Inc.
//
// This file is part of OpenZen, under the MIT License.
// See https://bitbucket.org/lpresearch/openzen/src/master/LICENSE for details
// SPDX-License-Identifier: MIT
//
//===========================================================================//

#include "SensorClient.h"

#include "SensorManager.h"

#include <spdlog/spdlog.h>

#ifdef ZEN_NETWORK
#include "processors/ZmqDataProcessor.h"
#endif

namespace {
    void safeStringToChar(std::string const& str, char * ch, size_t maxCharacter) {
        std::copy_n(str.begin(), std::min(size_t(maxCharacter), str.length()), ch);
        ch[std::min(size_t(maxCharacter - 1), str.length())] = 0;
    }
}

namespace zen
{
    SensorClient::SensorClient(uintptr_t) noexcept
    {}

    SensorClient::~SensorClient() noexcept
    {
        for (auto& pair : m_sensors)
            if (auto sensor = pair.second.lock())
                sensor->unsubscribe(m_eventQueue);
    }

    void SensorClient::listSensorsAsync() noexcept
    {
        SensorManager::get().subscribeToSensorDiscovery(*this);
    }

#ifdef ZEN_NETWORK
    ZenError SensorClient::publishEvents(std::shared_ptr<Sensor> sensor, const std::string & endpoint) {
        auto processor = std::make_unique<ZmqDataProcessor>();

        if (!processor->connect(endpoint)) {
            return ZenError_InvalidArgument;
        }
        sensor->addProcessor(std::move(processor));
        spdlog::info("Publishing events to endpoint {0}", endpoint);
        return ZenError_None;
    }
#else
    ZenError SensorClient::publishEvents(std::shared_ptr<Sensor>, const std::string&) {
        spdlog::error("ZeroMQ support not available in OpenZen build, cannot publish events");
        return ZenError_NotSupported;
    }
#endif

    std::shared_ptr<Sensor> SensorClient::findSensor(ZenSensorHandle_t handle) noexcept
    {
        auto it = m_sensors.find(handle.handle);
        if (it != m_sensors.end())
        {
            if (auto sensor = it->second.lock())
                return sensor;
            else
                m_sensors.erase(it);
        }

        return nullptr;
    }

    nonstd::expected<std::shared_ptr<Sensor>, ZenSensorInitError> SensorClient::obtain(const ZenSensorDesc& desc) noexcept
    {
        auto& manager = SensorManager::get();
        if (auto sensor = manager.obtain(desc))
        {
            if (sensor.value()->subscribe(m_eventQueue))
                m_sensors.emplace(sensor.value()->token(), *sensor);

            return std::move(*sensor);
        }
        else
        {
            return nonstd::make_unexpected(sensor.error());
        }
    }

    nonstd::expected<std::shared_ptr<Sensor>, ZenSensorInitError> SensorClient::obtain(const std::string& ioType,
        const std::string& identifier, uint32_t baudRate) noexcept {

        ZenSensorDesc desc;
        desc.serialNumber[0] = 0;
        desc.baudRate = baudRate;
        safeStringToChar(ioType, desc.ioType, size_t(64));
        safeStringToChar(identifier, desc.identifier, size_t(64));

        return obtain(desc);
    }

    ZenError SensorClient::release(std::shared_ptr<Sensor> sensor) noexcept
    {
        sensor->releaseProcessors();
        sensor->unsubscribe(m_eventQueue);
        m_sensors.erase(sensor->token());
        return ZenError_None;
    }

    std::optional<ZenEvent> SensorClient::pollNextEvent() noexcept
    {
        return m_eventQueue.tryToPop();
    }

    std::optional<ZenEvent> SensorClient::waitForNextEvent() noexcept
    {
        return m_eventQueue.waitToPop();
    }

    void SensorClient::notifyEvent(const ZenEvent& event) noexcept
    {
        m_eventQueue.push(event);
    }
}
