#include "io/systems/windows/WindowsDeviceSystem.h"

#include "io/interfaces/windows/WindowsDeviceInterface.h"

namespace zen
{
    namespace
    {
        bool tryToAddFile(unsigned int idx, const std::string& filename, ZenSensorDesc& outDesc)
        {
            auto handle = ::CreateFileA(filename.c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, 0, nullptr);
            if (handle != INVALID_HANDLE_VALUE)
            {
                ::CloseHandle(handle);

                outDesc.handle32 = idx;
                return true;
            }

            return false;
        }
    }

    ZenError WindowsDeviceSystem::listDevices(std::vector<ZenSensorDesc>& outDevices)
    {
        ZenSensorDesc desc;
        std::memcpy(desc.ioType, WindowsDeviceSystem::KEY, sizeof(WindowsDeviceSystem::KEY));

        for (unsigned int i = 1; i <= 9; ++i)
        {
            const std::string filename("\\\\.\\COM" + std::to_string(i));
            if (tryToAddFile(i, filename, desc))
                outDevices.emplace_back(desc);
        }

        for (unsigned int i = 10; i <= std::numeric_limits<uint8_t>::max(); ++i)
        {
            const std::string filename("\\.\\COM" + std::to_string(i));
            if (tryToAddFile(i, filename, desc))
                outDevices.emplace_back(desc);
        }

        return ZenError_None;
    }

    std::unique_ptr<BaseIoInterface> WindowsDeviceSystem::obtain(const ZenSensorDesc& desc, ZenError& outError)
    {
        std::string deviceId;
        HANDLE handle = ::CreateFileA(deviceId.c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, 0, nullptr);
        if (handle == INVALID_HANDLE_VALUE)
        {
            outError = ZenError_Io_InitFailed;
            return nullptr;
        }

        auto ioInterface = std::make_unique<WindowsDeviceInterface>(handle);

        DCB config;
        if (!::GetCommState(handle, &config))
        {
            outError = ZenError_Io_GetFailed;
            return nullptr;
        }

        config.fOutxCtsFlow = false;
        config.fOutxDsrFlow = false;
        config.fDtrControl = DTR_CONTROL_ENABLE;
        config.fOutX = false;
        config.fInX = false;
        config.fRtsControl = RTS_CONTROL_ENABLE;
        config.ByteSize = 8;
        config.Parity = NOPARITY;
        config.StopBits = ONESTOPBIT;

        if (!::SetCommState(handle, &config))
        {
            outError = ZenError_Io_SetFailed;
            return nullptr;
        }

        COMMTIMEOUTS timeoutConfig;
        timeoutConfig.ReadIntervalTimeout = 1;
        timeoutConfig.ReadTotalTimeoutMultiplier = 1;
        timeoutConfig.ReadTotalTimeoutConstant = 1;
        timeoutConfig.WriteTotalTimeoutMultiplier = 1;
        timeoutConfig.WriteTotalTimeoutConstant = 1;

        if (!::SetCommTimeouts(handle, &timeoutConfig))
        {
            outError = ZenError_Io_SetFailed;
            return nullptr;
        }

        if (outError = ioInterface->setBaudrate(DEFAULT_BAUDRATE))
            return nullptr;

        return ioInterface;
    }
}
