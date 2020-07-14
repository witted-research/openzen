//===========================================================================//
//
// Copyright (C) 2020 LP-Research Inc.
//
// This file is part of OpenZen, under the MIT License.
// See https://bitbucket.org/lpresearch/openzen/src/master/LICENSE for details
// SPDX-License-Identifier: MIT
//
//===========================================================================//


#include "io/systems/linux/LinuxDeviceQuery.h"
#include "io/systems/linux/LinuxDeviceSystem.h"

#include "io/interfaces/posix/PosixDeviceInterface.h"

#include <spdlog/spdlog.h>

#include <cstring>

#include <fcntl.h>
#include <termios.h>
#include <unistd.h>

namespace zen
{
    namespace
    {
        ZenSensorInitError setupFD(int fd)
        {
            struct termios config;
            if (-1 == ::tcgetattr(fd, &config))
                return ZenSensorInitError_IoFailed;

            config.c_iflag &= ~(IGNBRK | IXANY | INLCR | IGNCR | ICRNL);
            config.c_iflag &= ~IXON;    // disable XON/XOFF flow control (output)
            config.c_iflag &= ~IXOFF;   // disable XON/XOFF flow control (input)
            config.c_cflag &= ~CRTSCTS; // disable RTS flow control
            config.c_lflag = 0;
            config.c_oflag = 0;
            config.c_cflag &= ~CSIZE;
            config.c_cflag |= CS8;                // 8-bit chars
            config.c_cflag |= CLOCAL;             // ignore modem controls
            config.c_cflag |= CREAD;              // enable reading
            config.c_cflag &= ~(PARENB | PARODD); // disable parity
            config.c_cflag &= ~CSTOPB;            // one stop bit
            config.c_cc[VMIN] = 0;                // read doesn´t block
            config.c_cc[VTIME] = 5;               // 0.5 seconds read timeout

            if (-1 == ::tcsetattr(fd, TCSANOW, &config))
                return ZenSensorInitError_IoFailed;

            return ZenSensorInitError_None;
        }
    }

    ZenError LinuxDeviceSystem::listDevices(std::vector<ZenSensorDesc>& outDevices)
    {
        // LPMS sensors use SiLabs UART interface chips, this call will list them
        // with all their serial names coming directly from the USB SiLabs driver
        const auto siLabsDevices = LinuxDeviceQuery::getSiLabsDevices();

        for (const auto& siLabsSerialDevs : siLabsDevices ) {
            // one serial name can be on multiple ports
            for (const auto& siLabsDevs: siLabsSerialDevs.second) {
                ZenSensorDesc desc;
                auto const siLabsSerialNumber = siLabsSerialDevs.first;

                std::memcpy(desc.name, siLabsSerialNumber.c_str(), siLabsSerialNumber.size());
                desc.name[siLabsSerialNumber.size()] = '\0';

                std::memcpy(desc.serialNumber, siLabsSerialNumber.c_str(), siLabsSerialNumber.size());
                desc.serialNumber[siLabsSerialNumber.size()] = '\0';

                std::memcpy(desc.ioType, LinuxDeviceSystem::KEY, sizeof(LinuxDeviceSystem::KEY));

                // output the device path (like "/dev/ttyUSB0") just for additional information
                std::memcpy(desc.identifier, siLabsDevs.c_str(), siLabsDevs.size());
                desc.identifier[siLabsDevs.size()] = '\0';

                desc.baudRate = getDefaultBaudrate();

                outDevices.emplace_back(desc);
            }
        }

        return ZenError_None;
    }

    nonstd::expected<std::unique_ptr<IIoInterface>, ZenSensorInitError> LinuxDeviceSystem::obtain(
        const ZenSensorDesc& desc, IIoDataSubscriber& subscriber) noexcept
    {
        // use identifiert field to get the serial number in case
        // no serial number is given. This can happen if the obtainSensorByName is used.
        std::string serialNumberConnectTo(desc.serialNumber);
        if (serialNumberConnectTo.size() == 0) {
            serialNumberConnectTo = std::string(desc.identifier);
        }

        const auto ttyDevices = LinuxDeviceQuery::getDeviceFileForSiLabsSerial(serialNumberConnectTo);

        if (ttyDevices.size() == 0) {
            spdlog::error("Cannot find USB sensor with serial number {0}", serialNumberConnectTo);
            return nonstd::make_unexpected(ZenSensorInitError_InvalidAddress);
        }

        if (ttyDevices.size() > 1) {
            spdlog::error("Found multiple sensors with serial number {0}. Cannot uniquely connect to sensor",
                serialNumberConnectTo);
        }

        const auto ttyDevice = ttyDevices[0];

        spdlog::info("Opening file {} for sensor communication", ttyDevice);
        const int fdRead = ::open(ttyDevice.data(), O_RDONLY | O_NOCTTY | O_SYNC);
        const int fdWrite = ::open(ttyDevice.data(), O_WRONLY | O_NOCTTY | O_SYNC);
        if (fdRead == -1 || fdWrite == -1) {
            spdlog::error("Error while opening file {} for sensor communication", ttyDevice);
            return nonstd::make_unexpected(ZenSensorInitError_InvalidAddress);
        }
        
        auto ioInterface = std::make_unique<PosixDeviceInterface<LinuxDeviceSystem>>(subscriber, ttyDevice, fdRead, fdWrite);

        if (ZenSensorInitError error = setupFD(fdRead); error != ZenSensorInitError_None)
            return nonstd::make_unexpected(error);
        if (ZenSensorInitError error = setupFD(fdWrite); error != ZenSensorInitError_None)
            return nonstd::make_unexpected(error);
        return std::move(ioInterface);
    }

    nonstd::expected<std::vector<int32_t>, ZenError> LinuxDeviceSystem::supportedBaudRates() noexcept
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

    constexpr int32_t LinuxDeviceSystem::mapBaudRate(unsigned int baudRate) noexcept
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

    ZenError LinuxDeviceSystem::setBaudRateForFD(int fd, int speed) noexcept
    {
        struct termios config;
        if (-1 == ::tcgetattr(fd, &config)) {
            spdlog::error("Cannot get configuration of io interface file");
            return ZenError_Io_GetFailed;
        }

        cfsetispeed(&config, speed);
        cfsetospeed(&config, speed);
        if (-1 == ::tcsetattr(fd, TCSANOW, &config)) {
            spdlog::error("Cannot set configuration of io interface file");
            return ZenError_Io_SetFailed;
        }
        return ZenError_None;
    }
}
