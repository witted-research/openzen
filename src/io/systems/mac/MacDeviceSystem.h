#ifndef ZEN_IO_SYSTEMS_MAC_MACDEVICESYSTEM_H_
#define ZEN_IO_SYSTEMS_MAC_MACDEVICESYSTEM_H_

#include "io/IIoSystem.h"

namespace zen
{
    class MacDeviceSystem : public IIoSystem
    {
    public:
        constexpr static const char KEY[] = "MacDevice";

        bool available() override { return true; }

        ZenError listDevices(std::vector<ZenSensorDesc>& outDevices) override;

        nonstd::expected<std::unique_ptr<IIoInterface>, ZenSensorInitError> obtain(const ZenSensorDesc& desc, IIoDataSubscriber& subscriber) noexcept override;
    };
}

#endif
