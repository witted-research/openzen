#include "io/interfaces/linux/LinuxDeviceInterface.h"

#include "utility/Finally.h"

#include "io/systems/linux/LinuxDeviceSystem.h"

#include <cstring>

#include <termios.h>
#include <unistd.h>

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
    }

    LinuxDeviceInterface::LinuxDeviceInterface(IIoDataSubscriber& subscriber, std::string_view identifier, int fd) noexcept
        : IIoInterface(subscriber)
        , m_identifier(identifier)
        , m_fd(fd)
        , m_terminate(false)
        , m_pollingThread(&LinuxDeviceInterface::run, this)
        , m_baudRate(0)
    {
        std::memset(&m_readCB, 0, sizeof(aiocb64));
        m_readCB.aio_fildes = m_fd;
        m_readCB.aio_buf = m_buffer.data();
        m_readCB.aio_nbytes = m_buffer.size();
    }

    LinuxDeviceInterface::~LinuxDeviceInterface()
    {
        m_terminate = true;

        m_pollingThread.join();

        ::close(m_fd);
    }

    ZenError LinuxDeviceInterface::send(gsl::span<const std::byte> data) noexcept
    {
        const auto res = ::write(m_fd, data.data(), data.size());
        if (res == -1)
            return ZenError_Io_SendFailed;

        if (res != data.size())
            return ZenError_Io_SendFailed;

        return ZenError_None;
    }

    nonstd::expected<int32_t, ZenError> LinuxDeviceInterface::baudRate() const noexcept
    {
        return m_baudrate;
    }

    ZenError LinuxDeviceInterface::setBaudRate(unsigned int rate) noexcept
    {
        if (m_baudrate == rate)
            return ZenError_None;

        struct termios config;
        if (-1 == ::tcgetattr(m_fd, &config))
            return ZenError_Io_GetFailed;

        const auto speed = mapBaudrate(rate);

        cfsetispeed(&config, speed);
        cfsetospeed(&config, speed);

        if (-1 == ::tcsetattr(m_fd, TCSANOW,&config))
            return ZenError_Io_SetFailed;

        m_baudrate = rate;
        return ZenError_None;
    }

    nonstd::expected<std::vector<int32_t>, ZenError> LinuxDeviceInterface::supportedBaudRates() const noexcept
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

    std::string_view LinuxDeviceInterface::type() const noexcept
    {
        return LinuxDeviceSystem::KEY;
    }

    bool LinuxDeviceInterface::equals(const ZenSensorDesc& desc) const noexcept
    {
        if (std::string_view(LinuxDeviceSystem::KEY) != desc.ioType)
            return false;

        if (desc.name != m_identifier)
            return false;

        return true;
    }

    int LinuxDeviceInterface::run()
    {
        struct aiocb64* const list[1] { &m_readCB };

        while (!m_terminate)
        {
            if (::aio_read64(&m_readCB) == -1)
                return ZenError_Io_ReadFailed;

            if (::aio_suspend64(list, 1, nullptr) == -1)
                return ZenError_Io_ReadFailed;

            if (::aio_error64(&m_readCB) != 0)
                return ZenError_Io_ReadFailed;

            const auto nBytesReceived = ::aio_return64(&m_readCB);
            if (nBytesReceived > 0) {
                if (auto error = publishReceivedData(gsl::make_span(m_buffer.data(), nBytesReceived)))
                    return error;
            }
        }

        return ZenError_None;
    }
}
