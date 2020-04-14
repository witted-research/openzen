//===========================================================================//
//
// Copyright (C) 2020 LP-Research Inc.
//
// This file is part of OpenZen, under the MIT License.
// See https://bitbucket.org/lpresearch/openzen/src/master/LICENSE for details
// SPDX-License-Identifier: MIT
//
//===========================================================================//


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

// These are missing on Mac, but their values are just the numerical values, so no harm in defining them.
#ifndef B921600
#define B921600 speed_t(921600)
#endif
#ifndef B576000
#define B576000 speed_t(576000)
#endif
#ifndef B500000
#define B500000 speed_t(500000)
#endif
#ifndef B460800
#define B460800 speed_t(460800)
#endif

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

            desc.baudRate = getDefaultBaudrate();

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
        auto pfdWrite = openFD(ttyDevice, false);
        if (!pfdWrite) {
            spdlog::error("Error while opening file {} for sensor communication", ttyDevice);
            return nonstd::make_unexpected(pfdWrite.error());
        }
        int fdWrite = *pfdWrite;

        auto ioInterface = std::make_unique<PosixDeviceInterface<MacDeviceSystem>>(subscriber, ttyDevice, fdRead, fdWrite);

        return std::move(ioInterface);
    }

    nonstd::expected<std::vector<int32_t>, ZenError> MacDeviceSystem::supportedBaudRates() noexcept
    {
        std::vector<int32_t> baudRates;
        baudRates.reserve(22);

        baudRates.emplace_back(50);
        baudRates.emplace_back(75);
        baudRates.emplace_back(110);
        baudRates.emplace_back(134);
        baudRates.emplace_back(150);
        baudRates.emplace_back(200);
        baudRates.emplace_back(300);
        baudRates.emplace_back(600);
        baudRates.emplace_back(1200);
        baudRates.emplace_back(1800);
        baudRates.emplace_back(2400);
        baudRates.emplace_back(4800);
        baudRates.emplace_back(9600);
        baudRates.emplace_back(19200);
        baudRates.emplace_back(38400);
        baudRates.emplace_back(57600);
        baudRates.emplace_back(115200);
        baudRates.emplace_back(230400);
        baudRates.emplace_back(460800);
        baudRates.emplace_back(500000);
        baudRates.emplace_back(576000);
        baudRates.emplace_back(921600);

        return baudRates;
    }

    constexpr int32_t MacDeviceSystem::mapBaudRate(unsigned int baudRate) noexcept
    {
        if (baudRate > 576000)
            return B921600;
        else if (baudRate > 500000)
            return B576000;
        else if (baudRate > 460800)
            return B500000;
        else if (baudRate > 230400)
            return B460800;
        else if (baudRate > 115200)
            return B230400;
        else if (baudRate > 57600)
            return B115200;
        else if (baudRate > 38400)
            return B57600;
        else if (baudRate > 19200)
            return B38400;
        else if (baudRate > 9600)
            return B19200;
        else if (baudRate > 4800)
            return B9600;
        else if (baudRate > 2400)
            return B4800;
        else if (baudRate > 1800)
            return B2400;
        else if (baudRate > 1200)
            return B1800;
        else if (baudRate > 600)
            return B1200;
        else if (baudRate > 300)
            return B600;
        else if (baudRate > 200)
            return B300;
        else if (baudRate > 150)
            return B200;
        else if (baudRate > 134)
            return B150;
        else if (baudRate > 110)
            return B134;
        else if (baudRate > 75)
            return B110;
        else if (baudRate > 50)
            return B75;
        else if (baudRate > 0)
            return B50;
        else
            return B0;
    }

    ZenError MacDeviceSystem::setBaudRateForFD(int fd, int speed) noexcept
    {
        if (speed <= B230400) {
            struct termios config;
            if (-1 == ::tcgetattr(fd, &config)) {
                spdlog::error("Cannot get IO file's configuration");
                return ZenError_Io_GetFailed;
            }

            cfsetispeed(&config, speed);
            cfsetospeed(&config, speed);
            if (-1 == ::tcsetattr(fd, TCSANOW, &config)) {
                spdlog::error("Cannot get IO file's configuration");
                return ZenError_Io_SetFailed;
            }
        }
        else {
            // POSIX only defines baud rates up to 230400.  Once you
            // go above this, OS X needs an ioctl.
            auto result = ::ioctl(fd, IOSSIOSPEED, &speed);
            if (-1 == result) {
                spdlog::error("Cannot set IO speed");
                return ZenError_Io_SetFailed;
            }
        }

        return ZenError_None;
    }
}
