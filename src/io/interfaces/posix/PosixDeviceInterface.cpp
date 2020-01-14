#include "io/interfaces/posix/PosixDeviceInterface.h"

#include "utility/Finally.h"

#ifdef __linux__
#include "io/systems/linux/LinuxDeviceSystem.h"
#elif __APPLE__
#include "io/systems/mac/MacDeviceSystem.h"
#else
#error "Inconsistent configuration."
#endif

#include <cstring>

#include <sys/errno.h>
#include <sys/ioctl.h>
#ifdef __APPLE__
#include <IOKit/serial/ioss.h>
#endif
#include <termios.h>
#include <unistd.h>

#include "spdlog/spdlog.h"

// These are only defined on Linux, but their values are just the numerical values, so no harm in defining them.
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
        constexpr speed_t mapBaudrate(unsigned int baudrate)
        {
            if (baudrate > 576000)
                return B921600;
            else if (baudrate > 500000)
                return B576000;
            else if (baudrate > 460800)
                return B500000;
            else if (baudrate > 230400)
                return B460800;
            else if (baudrate > 115200)
                return B230400;
            else if (baudrate > 57600)
                return B115200;
            else if (baudrate > 38400)
                return B57600;
            else if (baudrate > 19200)
                return B38400;
            else if (baudrate > 9600)
                return B19200;
            else if (baudrate > 4800)
                return B9600;
            else if (baudrate > 2400)
                return B4800;
            else if (baudrate > 1800)
                return B2400;
            else if (baudrate > 1200)
                return B1800;
            else if (baudrate > 600)
                return B1200;
            else if (baudrate > 300)
                return B600;
            else if (baudrate > 200)
                return B300;
            else if (baudrate > 150)
                return B200;
            else if (baudrate > 134)
                return B150;
            else if (baudrate > 110)
                return B134;
            else if (baudrate > 75)
                return B110;
            else if (baudrate > 50)
                return B75;
            else if (baudrate > 0)
                return B50;
            else
                return B0;
        }

        ZenError setBaudRateForFD(int fd, int speed)
        {
#if defined(__linux__)
            struct termios config;
            if (-1 == ::tcgetattr(fd, &config))
                return ZenError_Io_GetFailed;

            cfsetispeed(&config, speed);
            cfsetospeed(&config, speed);
            if (-1 == ::tcsetattr(fd, TCSANOW, &config))
                return ZenError_Io_SetFailed;
#elif defined (__APPLE__)
            if (speed <= B230400) {
                struct termios config;
                if (-1 == ::tcgetattr(fd, &config))
                    return ZenError_Io_GetFailed;

                cfsetispeed(&config, speed);
                cfsetospeed(&config, speed);
                spdlog::error("not what i thought");
                if (-1 == ::tcsetattr(fd, TCSANOW, &config))
                    return ZenError_Io_SetFailed;
            }
            else {
                // POSIX only defines baud rates up to 230400.  Once you
                // go above this, OS X needs an ioctl.
                auto result = ::ioctl(fd, IOSSIOSPEED, &speed);
                if (-1 == result)
                    return ZenError_Io_SetFailed;
            }
#else
#error "Unsupported system.  Don't know how to set high baud rates."
#endif
            return ZenError_None;
        }
    }

    PosixDeviceInterface::PosixDeviceInterface(IIoDataSubscriber& subscriber, std::string_view identifier, int fdRead, int fdWrite) noexcept
        : IIoInterface(subscriber)
        , m_identifier(identifier)
        , m_fdRead(fdRead)
        , m_fdWrite(fdWrite)
        , m_terminate(false)
        , m_pollingThread(&PosixDeviceInterface::run, this)
        , m_baudrate(0)
    {
    }

    PosixDeviceInterface::~PosixDeviceInterface()
    {
        m_terminate = true;

        m_pollingThread.join();

        ::close(m_fdRead);
        ::close(m_fdWrite);
    }

    ZenError PosixDeviceInterface::send(gsl::span<const std::byte> data) noexcept
    {
        const auto res = ::write(m_fdWrite, data.data(), data.size());
        if (res == -1)
            return ZenError_Io_SendFailed;

        if (res != data.size())
            return ZenError_Io_SendFailed;

        return ZenError_None;
    }

    nonstd::expected<int32_t, ZenError> PosixDeviceInterface::baudRate() const noexcept
    {
        return m_baudrate;
    }

    ZenError PosixDeviceInterface::setBaudRate(unsigned int rate) noexcept
    {
        if (m_baudrate == rate)
            return ZenError_None;

        const auto speed = mapBaudrate(rate);
        if (ZenError error = setBaudRateForFD(m_fdRead, speed); error != ZenError_None)
            return error;
        if (ZenError error = setBaudRateForFD(m_fdWrite, speed); error != ZenError_None)
            return error;

        m_baudrate = rate;
        return ZenError_None;
    }

    nonstd::expected<std::vector<int32_t>, ZenError> PosixDeviceInterface::supportedBaudRates() const noexcept
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

    std::string_view PosixDeviceInterface::type() const noexcept
    {
#ifdef __linux__
        return LinuxDeviceSystem::KEY;
#elif __APPLE__
        return MacDeviceSystem::KEY;
#endif
    }

    bool PosixDeviceInterface::equals(const ZenSensorDesc& desc) const noexcept
    {
        if (type() != desc.ioType)
            return false;

        if (desc.name != m_identifier)
            return false;

        return true;
    }

    int PosixDeviceInterface::run()
    {
        std::array<std::byte, 256> buffer1, buffer2;

        aiocb readCB1 = {};
        readCB1.aio_fildes = m_fdRead;
        readCB1.aio_buf = buffer1.data();
        readCB1.aio_nbytes = buffer1.size();

        aiocb readCB2 = {};
        readCB2.aio_fildes = m_fdRead;
        readCB2.aio_buf = buffer2.data();
        readCB2.aio_nbytes = buffer2.size();

        aiocb* currentCB = &readCB1;
        aiocb* lastCB = &readCB2;

        if (m_terminate)
            return ZenError_None;

        if (::aio_read(currentCB) == -1)
            return ZenError_Io_ReadFailed;

        while (!m_terminate) {
            if (::aio_suspend(&currentCB, 1, nullptr) == -1)
                return ZenError_Io_ReadFailed;

            if (::aio_error(currentCB) != 0)
                return ZenError_Io_ReadFailed;

            const auto nBytesReceived = ::aio_return(currentCB);

            // Start next read, process data (if any) in parallel.
            std::swap(currentCB, lastCB);
            if (::aio_read(currentCB) == -1)
                return ZenError_Io_ReadFailed;

            if (nBytesReceived > 0) {
                if (auto error = publishReceivedData(gsl::make_span((std::byte *)lastCB->aio_buf, nBytesReceived)))
                    return error;
            }
        }

        return ZenError_None;
    }
}
