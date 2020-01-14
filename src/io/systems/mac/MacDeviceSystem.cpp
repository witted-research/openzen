
#include "io/systems/mac/MacDeviceSystem.h"

#include <spdlog/spdlog.h>

#include <cstring>

#include <dirent.h>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>

#include <sys/ioctl.h>
#include <IOKit/serial/ioss.h>
#include <IOKit/serial/IOSerialKeys.h>

#include "io/interfaces/posix/PosixDeviceInterface.h"

namespace zen
{
    namespace
    {
        nonstd::expected<int, ZenSensorInitError> openFD(std::string_view filename, bool forReading)
        {
            int flags = O_NOCTTY | O_NONBLOCK;
            if (forReading)
                flags |= O_RDONLY;
            else
                flags |= O_WRONLY;

            auto fd = ::open(filename.data(), flags);
            if (fd == -1)
                return nonstd::make_unexpected(ZenSensorInitError_IoFailed);
            fcntl(fd, F_SETFL, 0);

            struct termios config;
            if (-1 == ::tcgetattr(fd, &config))
                return nonstd::make_unexpected(ZenSensorInitError_IoFailed);

            config.c_iflag &= ~(IGNBRK | IXANY | INLCR | IGNCR | ICRNL);
            config.c_iflag &= ~IXON;   // disable XON/XOFF flow control (output)
            config.c_iflag &= ~IXOFF;  // disable XON/XOFF flow control (input)
            config.c_cflag |= CRTSCTS; // enable RTS flow control
            config.c_lflag = 0;
            config.c_oflag = 0;
            config.c_cflag &= ~CSIZE;
            config.c_cflag |= CS8;                // 8-bit chars
            config.c_cflag |= CLOCAL;             // ignore modem controls
            config.c_cflag |= CREAD;              // enable reading
            config.c_cflag &= ~(PARENB | PARODD); // disable parity
            config.c_cflag &= ~CSTOPB;            // one stop bit
            config.c_cflag &= ~CCAR_OFLOW;        // no DCD flow control
            config.c_cc[VMIN] = 0;                // read doesn´t block
            config.c_cc[VTIME] = 5;               // 0.5 seconds read timeout

            if (-1 == ::tcsetattr(fd, TCSANOW, &config))
                return nonstd::make_unexpected(ZenSensorInitError_IoFailed);

            unsigned long mics = 1UL;
            ::ioctl(fd, IOSSDATALAT, &mics);

            int handshake = 0;
            ::ioctl(fd, TIOCMGET, &handshake);

            return fd;
        }
    }

    ZenError MacDeviceSystem::listDevices(std::vector<ZenSensorDesc>& outDevices)
    {
        // The SiLabs driver registers a device /dev/cu.SLAB_USBtoUARTnn,
        // where n is empty for the first or only device connected to the
        // system and is a monotonously increasing number for devices
        // connected while another device is already connected.
        // We therefore identify devices by looking for the common first
        // part of the string.

        // std::filesystem is barely supported on MacOS, so this uses the POSIX
        // fiilesystem iterators.
        DIR *dp = opendir("/dev/");
        if (!dp)
            return ZenError_Device_ListingFailed;
        auto closeDirGuard = gsl::finally([dp] { closedir(dp); });
        while (dirent *entry = readdir(dp)) {
            std::string_view sfn(entry->d_name, strlen(entry->d_name));
            if (sfn.substr(0, strlen("cu.SLAB_USBtoUART")) != "cu.SLAB_USBtoUART")
                continue;

            // The strings always fit in the destination fields.
            // "/dev/cu.SLAB_USBtoUART" is 22 characters long.
            ZenSensorDesc desc;
            std::memcpy(desc.name, sfn.data(), sfn.size());
            desc.name[sfn.size()] = '\0';

            std::memcpy(desc.serialNumber, sfn.data(), sfn.size());
            desc.serialNumber[sfn.size()] = '\0';

            std::memcpy(desc.ioType, MacDeviceSystem::KEY, sizeof(MacDeviceSystem::KEY));

            std::string fnWithPath = std::string("/dev/") + std::string(sfn);
            std::memcpy(desc.identifier, fnWithPath.data(), fnWithPath.size());
            desc.identifier[fnWithPath.size()] = '\0';

            desc.baudRate = 921600;

            outDevices.emplace_back(desc);
        }     
        return ZenError_None;
    }

    nonstd::expected<std::unique_ptr<IIoInterface>, ZenSensorInitError> MacDeviceSystem::obtain(
        const ZenSensorDesc& desc, IIoDataSubscriber& subscriber) noexcept
    {
        const auto ttyDevice = desc.identifier;

        spdlog::info("Opening file {} for sensor communication", ttyDevice);
        auto pfdRead = openFD(ttyDevice, true);
        if (!pfdRead) {
            spdlog::error("Error while opening file {} for sensor communication", ttyDevice);
            return nonstd::make_unexpected(pfdRead.error());
        }
        int fdRead = *pfdRead;
        if (0) {
            std::byte data[256];
            spdlog::error("reading");
            const auto nBytesReceived = ::read(fdRead, data, 256);
            spdlog::error("first read gave {}", nBytesReceived);
        }
        auto pfdWrite = openFD(ttyDevice, false);
        if (!pfdWrite) {
            spdlog::error("Error while opening file {} for sensor communication", ttyDevice);
            return nonstd::make_unexpected(pfdWrite.error());
        }
        int fdWrite = *pfdWrite;

        auto ioInterface = std::make_unique<PosixDeviceInterface>(subscriber, ttyDevice, fdRead, fdWrite);

        return std::move(ioInterface);
    }
}
