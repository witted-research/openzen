//===========================================================================//
//
// Copyright (C) 2020 LP-Research Inc.
//
// This file is part of OpenZen, under the MIT License.
// See https://bitbucket.org/lpresearch/openzen/src/master/LICENSE for details
// SPDX-License-Identifier: MIT
//
//===========================================================================//

#include "io/interfaces/TestSensorInterface.h"
#include "io/systems/TestSensorSystem.h"

#include <spdlog/spdlog.h>

namespace zen
{
    TestSensorInterface::TestSensorInterface(IIoEventSubscriber& subscriber,std::string const&) noexcept
        : IIoEventInterface(subscriber)
    {
        spdlog::info("Created TestSensor interface");

        m_terminate = false;
        m_pollingThread = std::thread(&TestSensorInterface::run, this);
    }

    TestSensorInterface::~TestSensorInterface()
    {
        spdlog::info("Terminating TestSensor interface.");
        m_terminate = true;
        m_pollingThread.join();
        spdlog::info("TestSensor interface terminated.");
    }

    std::string_view TestSensorInterface::type() const noexcept
    {
        return TestSensorSystem::KEY;
    }

    bool TestSensorInterface::equals(const ZenSensorDesc& desc) const noexcept
    {
        if (std::string_view(TestSensorSystem::KEY) != desc.ioType)
            return false;

        return true;
    }

    int TestSensorInterface::run()
    {
        spdlog::info("Running TestSensor interface thread");

        while(!m_terminate) {
            // simulate 100 Hz
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            ZenEvent evt;
            evt.eventType = ZenEventType_ImuData;

            evt.data.imuData.q[0] = 0.5;
            evt.data.imuData.q[1] = -0.5;
            evt.data.imuData.q[2] = -0.5;
            evt.data.imuData.q[3] = 0.5;

            evt.data.imuData.a[0] = 0.0f;
            evt.data.imuData.a[1] = 0.0f;
            evt.data.imuData.a[2] = -1.0f;

            evt.data.imuData.g[0] = 23.0f;
            evt.data.imuData.g[1] = 24.0f;
            evt.data.imuData.g[2] = 25.0f;

            ZenSensorHandle sensorHandle;
            sensorHandle.handle = 5;
            evt.sensor = sensorHandle;

            ZenComponentHandle_t componentHandle;
            // imu handle is 1 for regular sensors
            componentHandle.handle = 1;
            evt.component = componentHandle;

            publishReceivedData(evt);
        }

        // terminate this thread happily
        return ZenError_None;
    }
}
