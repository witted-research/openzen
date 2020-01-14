
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

        for (const auto siLabsSerialDevs : siLabsDevices ) {
            // one serial name can be on multiple ports
            for (const auto siLabsDevs: siLabsSerialDevs.second) {
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

                desc.baudRate = 921600;

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
        
        auto ioInterface = std::make_unique<PosixDeviceInterface>(subscriber, ttyDevice, fdRead, fdWrite);

        if (ZenSensorInitError error = setupFD(fdRead); error != ZenSensorInitError_None)
            return nonstd::make_unexpected(error);
        if (ZenSensorInitError error = setupFD(fdWrite); error != ZenSensorInitError_None)
            return nonstd::make_unexpected(error);
        return std::move(ioInterface);
    }
}
