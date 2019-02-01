#ifndef ZEN_IO_INTERFACES_WINDOWS_WINDOWSDEVICEINTERFACE_H_
#define ZEN_IO_INTERFACES_WINDOWS_WINDOWSDEVICEINTERFACE_H_

#include <array>
#include <atomic>
#include <string>
#include <string_view>
#include <thread>

#define NOMINMAX
#include <Windows.h>
#undef NOMINMAX

#include "io/interfaces/BaseIoInterface.h"

namespace zen
{
    class WindowsDeviceInterface : public BaseIoInterface
    {
    public:
        WindowsDeviceInterface(HANDLE handle, std::unique_ptr<modbus::IFrameFactory> factory, std::unique_ptr<modbus::IFrameParser> parser) noexcept;
        ~WindowsDeviceInterface();

        /** Returns the IO interface's baudrate (bit/s) */
        ZenError baudrate(int32_t& rate) const override;

        /** Set Baudrate of IO interface (bit/s) */
        ZenError setBaudrate(unsigned int rate) override;

        /** Returns the supported baudrates of the IO interface (bit/s) */
        ZenError supportedBaudrates(std::vector<int32_t>& outBaudrates) const override;

        /** Returns the type of IO interface */
        const char* type() const override;

        /** Returns whether the IO interface equals the sensor description */
        bool equals(const ZenSensorDesc& desc) const override;

    private:
        /** Send data to IO interface */
        ZenError send(std::vector<unsigned char> frame) override;

        ZenError parseBuffer();
        ZenError receiveInBuffer(bool& received);

        int run();

        std::array<unsigned char, 1024> m_buffer;
        size_t m_usedBufferSize;

        std::atomic_bool m_terminate;
        std::thread m_pollingThread;

        std::string m_deviceId;

        DCB m_config;
        HANDLE m_handle;

        unsigned int m_baudrate;
    };
}

#endif
