#include "io/interfaces/TestSensorInterface.h"
#include "io/systems/TestSensorSystem.h"

#include <spdlog/spdlog.h>

namespace zen
{
    TestSensorInterface::TestSensorInterface(IIoEventSubscriber& subscriber,std::string const& endpoint) noexcept
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

        for (size_t i = 0; i < 2; i++) {
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
            ZenEvent evt;
            evt.eventType = ZenImuEvent_Sample;

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
