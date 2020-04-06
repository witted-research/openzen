//===========================================================================//
//
// Copyright (C) 2020 LP-Research Inc.
//
// This file is part of OpenZen, under the MIT License.
// See https://bitbucket.org/lpresearch/openzen/src/master/LICENSE for details
// SPDX-License-Identifier: MIT
//
//===========================================================================//

#ifndef ZEN_SENSORMANAGER_H_
#define ZEN_SENSORMANAGER_H_

#include <atomic>
#include <condition_variable>
#include <map>
#include <mutex>
#include <set>
#include <thread>
#include <type_traits>

#ifdef ZEN_BLUETOOTH_BLE
#include <QCoreApplication>
#endif

#include "nonstd/expected.hpp"

#include "Sensor.h"
#include "SensorClient.h"
#include "utility/ReferenceCmp.h"

#include <memory>
#include <set>
#include <vector>

namespace zen
{
    /**
    Central class which marshalls access to the available IoSystems and their
    connected sensors. This class lives as a static singleton, which is contained
    in the get() method;
    */
    class SensorManager
    {
    public:
        /**
        Access to the SensorManager singleton. The SensorManager lives as static inside
        this method.
        */
        friend class Sensor;

        static SensorManager& get();

        /** Try to obtain a sensor based on a sensor description. */
        nonstd::expected<std::shared_ptr<Sensor>, ZenSensorInitError> obtain(const ZenSensorDesc& desc) noexcept;

        /** Subscribe a client to sensor discovery */
        void subscribeToSensorDiscovery(SensorClient& client) noexcept;

        void registerDataProcessor(std::unique_ptr<DataProcessor> processor) noexcept;

    private:

        SensorManager() noexcept;
        ~SensorManager() noexcept;

        /** Releases a sensor */
        std::shared_ptr<Sensor> release(ZenSensorHandle_t sensorHandle) noexcept;

        void sensorDiscoveryLoop() noexcept;
        void sensorLoop();

        std::set<std::shared_ptr<Sensor>, SensorCmp> m_sensors;
        std::set<std::reference_wrapper<SensorClient>, ReferenceWrapperCmp<SensorClient>> m_discoverySubscribers;

        /**
        This mutex needs to be held to access or modify the m_processors vector
         */
        std::mutex m_processorsMutex;
        std::vector<ZenSensorDesc> m_devices;

        std::condition_variable m_discoveryCv;

        std::mutex m_sensorsMutex;
        std::mutex m_discoveryMutex;

        uintptr_t m_nextToken;
        bool m_discovering;

        std::atomic_bool m_terminate;

        std::thread m_sensorThread;
        std::thread m_sensorDiscoveryThread;

        #ifdef ZEN_BLUETOOTH_BLE
        std::unique_ptr<QCoreApplication> m_app;
        #endif
    };
}

#endif
