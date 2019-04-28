#ifndef ZEN_IO_SYSTEMS_WINDOWS_WINDOWSDEVICESYSTEM_H_
#define ZEN_IO_SYSTEMS_WINDOWS_WINDOWSDEVICESYSTEM_H_

#include "io/IIoSystem.h"

#define NOMINMAX
#include <Windows.h>
#undef NOMINMAX

namespace zen
{
    class WindowsDeviceSystem : public IIoSystem
    {   
    public:
        constexpr static const char KEY[] = "WindowsDevice";

        bool available() override { return true; }

        ZenError listDevices(std::vector<ZenSensorDesc>& outDevices) override;

        nonstd::expected<std::unique_ptr<IIoInterface>, ZenSensorInitError> obtain(const ZenSensorDesc& desc, IIoDataSubscriber& subscriber) noexcept override;
    };
}

#endif
