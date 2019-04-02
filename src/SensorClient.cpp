#include "SensorClient.h"

#include "SensorManager.h"

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

    ZenError SensorClient::release(std::shared_ptr<Sensor> sensor) noexcept
    {
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
