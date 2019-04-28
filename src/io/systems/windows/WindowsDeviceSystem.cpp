#include "io/systems/windows/WindowsDeviceSystem.h"

#include "io/interfaces/windows/WindowsDeviceInterface.h"

namespace zen
{
    namespace
    {
        HANDLE openCOMPort(std::string_view filename)
        {
            return ::CreateFileA(filename.data(), GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, nullptr);
        }
    }

    ZenError WindowsDeviceSystem::listDevices(std::vector<ZenSensorDesc>& outDevices)
    {
        constexpr int MAX_COM_PORTS = 256;
        for (unsigned int i = 1; i <= MAX_COM_PORTS; ++i)
        {
            const std::string filename("\\\\.\\COM" + std::to_string(i));

            auto handle = openCOMPort(filename);
            if (handle != INVALID_HANDLE_VALUE)
            {
                ::CloseHandle(handle);

                ZenSensorDesc desc;
                std::memcpy(desc.ioType, WindowsDeviceSystem::KEY, sizeof(WindowsDeviceSystem::KEY));
                std::memcpy(desc.name, filename.c_str(), filename.size());
                desc.name[filename.size()] = '\0';
                desc.serialNumber[0] = '\0';
                desc.handle32 = i;

                outDevices.emplace_back(desc);
            }
        }

        return ZenError_None;
    }

    nonstd::expected<std::unique_ptr<IIoInterface>, ZenSensorInitError> WindowsDeviceSystem::obtain(const ZenSensorDesc& desc, IIoDataSubscriber& subscriber) noexcept
    {
        auto handle = openCOMPort(desc.name);
        if (handle == INVALID_HANDLE_VALUE)
            return nonstd::make_unexpected(ZenSensorInitError_InvalidAddress);

        // Create overlapped objects for asynchronous communication
        OVERLAPPED ioReader{ 0 };
        ioReader.hEvent = ::CreateEventA(nullptr, false, false, nullptr);
        if (!ioReader.hEvent)
            return nonstd::make_unexpected(ZenSensorInitError_IoFailed);

        OVERLAPPED ioWriter{ 0 };
        ioWriter.hEvent = ::CreateEventA(nullptr, false, false, nullptr);
        if (!ioWriter.hEvent)
            return nonstd::make_unexpected(ZenSensorInitError_IoFailed);

        auto ioInterface = std::make_unique<WindowsDeviceInterface>(subscriber, desc.name, handle, ioReader, ioWriter);

        DCB config;
        if (!::GetCommState(handle, &config))
            return nonstd::make_unexpected(ZenSensorInitError_IoFailed);

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
            return nonstd::make_unexpected(ZenSensorInitError_IoFailed);

        COMMTIMEOUTS timeoutConfig;
        timeoutConfig.ReadIntervalTimeout = 1;
        timeoutConfig.ReadTotalTimeoutMultiplier = 1;
        timeoutConfig.ReadTotalTimeoutConstant = 1;
        timeoutConfig.WriteTotalTimeoutMultiplier = 1;
        timeoutConfig.WriteTotalTimeoutConstant = 1;

        if (!::SetCommTimeouts(handle, &timeoutConfig))
            return nonstd::make_unexpected(ZenSensorInitError_IoFailed);

        //constexpr static unsigned int DEFAULT_BAUDRATE = CBR_115200;
        //if (auto error = ioInterface->setBaudRate(DEFAULT_BAUDRATE))
        //    return nonstd::make_unexpected(ZenSensorInitError_IoFailed);

        return ioInterface;
    }
}
