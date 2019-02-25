#ifndef ZEN_IO_SYSTEMS_BLESYSTEM_H_
#define ZEN_IO_SYSTEMS_BLESYSTEM_H_

#include "io/IIoSystem.h"

namespace zen
{
    class BleSystem : public IIoSystem
    {
    public:
        constexpr static const char KEY[] = "Ble";

        bool available() override;

        ZenError listDevices(std::vector<ZenSensorDesc>& outDevices) override;

        nonstd::expected<std::unique_ptr<IIoInterface>, ZenSensorInitError> obtain(const ZenSensorDesc& desc, IIoDataSubscriber& subscriber) noexcept override;
    };
}

#endif
