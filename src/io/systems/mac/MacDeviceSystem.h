#ifndef ZEN_IO_SYSTEMS_MAC_MACDEVICESYSTEM_H_
#define ZEN_IO_SYSTEMS_MAC_MACDEVICESYSTEM_H_

#include "io/IIoSystem.h"

namespace zen
{
    class MacDeviceSystem final : public IIoSystem
    {
    public:
        constexpr static const char KEY[] = "MacDevice";

        bool available() override { return true; }

        ZenError listDevices(std::vector<ZenSensorDesc>& outDevices) override;

        nonstd::expected<std::unique_ptr<IIoInterface>, ZenSensorInitError> obtain(const ZenSensorDesc& desc, IIoDataSubscriber& subscriber) noexcept override;

        static nonstd::expected<std::vector<int32_t>, ZenError> supportedBaudRates() noexcept;
        static constexpr int32_t mapBaudRate(unsigned int baudRate) noexcept;
        static ZenError setBaudRateForFD(int fd, int speed) noexcept;

        uint32_t getDefaultBaudrate() override { return 921600; }
    };
}

#endif
