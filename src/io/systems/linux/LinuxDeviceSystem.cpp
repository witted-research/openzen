#include "io/systems/linux/LinuxDeviceSystem.h"

#include "io/interfaces/linux/LinuxDeviceInterface.h"

#include <cstring>

#include <fcntl.h>
#include <termios.h>
#include <unistd.h>

namespace zen
{
    namespace
    {
        int openFD(std::string_view filename)
        {
            return ::open(filename.data(), O_RDWR | O_NOCTTY | O_SYNC);
        }
    }

    ZenError LinuxDeviceSystem::listDevices(std::vector<ZenSensorDesc>& outDevices)
    {
        constexpr int MAX_USB_PORTS = 100;
        for (unsigned int i = 0; i <= MAX_USB_PORTS; ++i)
        {
            const std::string filename("/dev/ttyUSB" + std::to_string(i));

            const int fd = openFD(filename);
            if (fd != -1)
            {
                ::close(fd);

                const std::string name("USB PORT #" + std::to_string(i));

                ZenSensorDesc desc;
                std::memcpy(desc.name, name.c_str(), name.size());
                desc.name[name.size()] = '\0';

                desc.serialNumber[0] = '\0';
                std::memcpy(desc.ioType, LinuxDeviceSystem::KEY, sizeof(LinuxDeviceSystem::KEY));

                std::memcpy(desc.identifier, filename.c_str(), filename.size());
                desc.identifier[filename.size()] = '\0';

                outDevices.emplace_back(desc);
            }
        }

        return ZenError_None;
    }

    nonstd::expected<std::unique_ptr<IIoInterface>, ZenSensorInitError> LinuxDeviceSystem::obtain(const ZenSensorDesc& desc, IIoDataSubscriber& subscriber) noexcept
    {
        const int fd = openFD(desc.identifier);
        if (fd == -1)
            return nonstd::make_unexpected(ZenSensorInitError_InvalidAddress);

        auto ioInterface = std::make_unique<LinuxDeviceInterface>(subscriber, desc.identifier, fd);

        struct termios config;
        if (-1 == ::tcgetattr(fd, &config))
            return nonstd::make_unexpected(ZenSensorInitError_IoFailed);

        config.c_iflag &= ~(IGNBRK | IXANY | INLCR | IGNCR | ICRNL);
        config.c_iflag &= ~IXON;                // disable XON/XOFF flow control (output)
        config.c_iflag &= ~IXOFF;               // disable XON/XOFF flow control (input)
        config.c_cflag &= ~CRTSCTS;             // disable RTS flow control
        config.c_lflag = 0;
        config.c_oflag = 0;
        config.c_cflag &= ~CSIZE;
        config.c_cflag |= CS8;                  // 8-bit chars
        config.c_cflag |= CLOCAL;               // ignore modem controls
        config.c_cflag |= CREAD;                // enable reading
        config.c_cflag &= ~(PARENB | PARODD);   // disable parity
        config.c_cflag &= ~CSTOPB;              // one stop bit
        config.c_cc[VMIN] = 0;                  // read doesn´t block
        config.c_cc[VTIME] = 5;                 // 0.5 seconds read timeout

        if (-1 == ::tcsetattr(fd, TCSANOW, &config))
            return nonstd::make_unexpected(ZenSensorInitError_IoFailed);

        return ioInterface;
    }
}
