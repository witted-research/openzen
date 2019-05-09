#ifndef ZEN_IO_INTERFACES_LINUX_LINUXDEVICEINTERFACE_H_
#define ZEN_IO_INTERFACES_LINUX_LINUXDEVICEINTERFACE_H_

#include <array>
#include <atomic>
#include <string>
#include <string_view>
#include <thread>

#include "io/IIoInterface.h"

namespace zen
{
    class LinuxDeviceInterface : public IIoInterface
    {
    public:
        LinuxDeviceInterface(IIoDataSubscriber& subscriber, std::string_view identifier, int fd) noexcept;
        ~LinuxDeviceInterface();

        /** Send data to IO interface */
        ZenError send(gsl::span<const std::byte> data) noexcept override;

        /** Returns the IO interface's baudrate (bit/s) */
        nonstd::expected<int32_t, ZenError> baudRate() const noexcept override;

        /** Set Baudrate of IO interface (bit/s) */
        ZenError setBaudRate(unsigned int rate) noexcept override;

        /** Returns the supported baudrates of the IO interface (bit/s) */
        nonstd::expected<std::vector<int32_t>, ZenError> supportedBaudRates() const noexcept override;

        /** Returns the type of IO interface */
        std::string_view type() const noexcept override;

        /** Returns whether the IO interface equals the sensor description */
        bool equals(const ZenSensorDesc& desc) const noexcept override;

    private:
        int run();

        std::array<std::byte, 256> m_buffer;

        std::string m_identifier;

        int m_fd;

        std::atomic_bool m_terminate;
        std::thread m_pollingThread;

        unsigned int m_baudrate;
    };
}

#endif
