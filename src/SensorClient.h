//===========================================================================//
//
// Copyright (C) 2020 LP-Research Inc.
//
// This file is part of OpenZen, under the MIT License.
// See https://bitbucket.org/lpresearch/openzen/src/master/LICENSE for details
// SPDX-License-Identifier: MIT
//
//===========================================================================//

#ifndef ZEN_SENSORCLIENT_H_
#define ZEN_SENSORCLIENT_H_

#include <mutex>
#include <optional>
#include <unordered_map>

#include <nonstd/expected.hpp>

#include "Sensor.h"
#include "utility/LockingQueue.h"

namespace zen
{
    class SensorClient
    {
    public:
        SensorClient(uintptr_t token) noexcept;
        ~SensorClient() noexcept;

        /** Opts in to an asynchronous process that lists available sensors.
         * ZenEventData_SensorListingProgress events will be queued to indicate progress.
         * ZenEventData_SensorFound events will be queued to signal sensor descriptions.
         */
        void listSensorsAsync() noexcept;

        std::shared_ptr<Sensor> findSensor(ZenSensorHandle_t handle) noexcept;

        nonstd::expected<std::shared_ptr<Sensor>, ZenSensorInitError> obtain(const ZenSensorDesc& desc) noexcept;

        nonstd::expected<std::shared_ptr<Sensor>, ZenSensorInitError> obtain(const std::string& ioType,
            const std::string& identifier, uint32_t baudRate) noexcept;

        ZenError release(std::shared_ptr<Sensor> sensor) noexcept;

        /** Returns true and fills the next event on the queue if there is one, otherwise returns false. */
        std::optional<ZenEvent> pollNextEvent() noexcept;

        /** Returns true and fills the next event on the queue when there is a new one, otherwise returns false upon a call to ZenShutdown() */
        std::optional<ZenEvent> waitForNextEvent() noexcept;

        /** Open an OpenZen publisher socket and send all events there. This could be improved by
        having a dedicated subscriber only for the ZeroMQ submission.
        */
        ZenError publishEvents(std::shared_ptr<Sensor> sensor, const std::string & endpoint);

        /** Pushes an event to the event queue */
        void notifyEvent(const ZenEvent& event) noexcept;

    private:
        LockingQueue<ZenEvent> m_eventQueue;

        std::unordered_map<uintptr_t, std::weak_ptr<Sensor>> m_sensors;
    };
}

#endif
