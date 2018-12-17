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
        constexpr static unsigned int DEFAULT_BAUDRATE = CBR_115200;
        
    public:
        constexpr static const char KEY[] = "WindowsDevice";

        bool available() override { return true; }

        ZenError listDevices(std::vector<ZenSensorDesc>& outDevices) override;

        std::unique_ptr<BaseIoInterface> obtain(const ZenSensorDesc& desc, ZenError& outError) override;
    };
}

#endif