#ifndef ZEN_IO_SYSTEMS_ZEROMQSYSTEM_H_
#define ZEN_IO_SYSTEMS_ZEROMQSYSTEM_H_

#include "io/IIoSystem.h"
#include "io/IIoInterface.h"
#include "io/IIoEventInterface.h"
#include <memory>

namespace zmq {
    class context_t;
}

namespace zen
{
    class ZeroMQSystem : public IIoSystem
    {
    public:
        constexpr static const char KEY[] = "ZeroMQ";

        bool available() override;

        bool isHighLevel() override { return true; }

        // this system won't list any devices to connect to, ZenObtainSensorByName can
        // be used to use ZeroMQ
        ZenError listDevices(std::vector<ZenSensorDesc>& outDevices) override;

        /** If succesful, obtains the IO interface for the provided sensor description. Otherwise, returns an error. */
        nonstd::expected<std::unique_ptr<IIoInterface>, ZenSensorInitError> obtain(const ZenSensorDesc& desc, IIoDataSubscriber& subscriber) noexcept override;

        nonstd::expected<std::unique_ptr<IIoEventInterface>, ZenSensorInitError> obtainEventBased(const ZenSensorDesc& desc, 
            IIoEventSubscriber & ) noexcept override;
    };
}

#endif
