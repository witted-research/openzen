#ifndef ZEN_IO_INTERFACES_SIUSBINTERFACE_H_
#define ZEN_IO_INTERFACES_SIUSBINTERFACE_H_

#include <array>
#include <atomic>
#include <thread>

#define NOMINMAX
#include <Windows.h>
#undef NOMINMAX

#include "SiUSBXp.h"

#include "io/interfaces/BaseIoInterface.h"

namespace zen
{
    class SiUsbInterface : public BaseIoInterface
    {
    public:
        SiUsbInterface(HANDLE handle, OVERLAPPED ioReader, std::unique_ptr<modbus::IFrameFactory> factory, std::unique_ptr<modbus::IFrameParser> parser) noexcept;
        ~SiUsbInterface();

        /** Send data to USB interface */
        ZenError send(std::vector<unsigned char> frame) override;

        /** Returns the Si USB interface's baudrate (bit/s) */
        ZenError baudrate(int32_t& rate) const override;

        /** Set Baudrate of Si USB interface (bit/s) */
        ZenError setBaudrate(unsigned int rate) override;

        /** Returns the supported baudrates of the IO interface (bit/s) */
        ZenError supportedBaudrates(std::vector<int32_t>& outBaudrates) const override;

        /** Returns the type of IO interface */
        const char* type() const override;

        /** Returns whether the IO interface equals the sensor description */
        bool equals(const ZenSensorDesc& desc) const override;

    private:
        int run();

        std::array<unsigned char, 256> m_buffer;

        HANDLE m_handle;
        OVERLAPPED m_ioReader;

        std::atomic_bool m_terminate;
        std::thread m_pollingThread;

        unsigned int m_baudrate;
    };
}

#endif
