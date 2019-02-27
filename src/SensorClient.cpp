#include "SensorClient.h"

#include "SensorManager.h"

namespace zen
{
    SensorClient::SensorClient(uintptr_t token) noexcept
        : m_token(token)
    {}

    SensorClient::~SensorClient() noexcept
    {
        auto& manager = SensorManager::get();
        for (auto& sensor : m_sensors)
            manager.unsubscribeFromSensor(*this, std::move(sensor));
    }

    void SensorClient::listSensorsAsync() noexcept
    {
        SensorManager::get().subscribeToSensorDiscovery(*this);
    }

    std::shared_ptr<Sensor> SensorClient::findSensor(ZenSensorHandle_t handle) const noexcept
    {
        auto it = m_sensors.find(handle);
        if (it == m_sensors.end())
            return nullptr;

        return *it;
    }

    nonstd::expected<std::shared_ptr<Sensor>, ZenSensorInitError> SensorClient::obtain(const ZenSensorDesc& desc) noexcept
    {
        auto& manager = SensorManager::get();
        if (auto sensor = manager.obtain(desc))
        {
            manager.subscribeToSensor(*sensor, *this);
            m_sensors.insert(*sensor);

            return std::move(*sensor);
        }
        else
        {
            return nonstd::make_unexpected(sensor.error());
        }
    }

    ZenError SensorClient::release(std::shared_ptr<Sensor> sensor) noexcept
    {
        m_sensors.erase(sensor);
        SensorManager::get().unsubscribeFromSensor(*this, std::move(sensor));
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
