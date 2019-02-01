#ifndef ZEN_IO_INTERFACES_FTDIUSBINTERFACE_H_
#define ZEN_IO_INTERFACES_FTDIUSBINTERFACE_H_

#include <atomic>
#include <string>
#include <string_view>
#include <thread>

#define NOMINMAX
#include "ftd2xx.h"
#undef NOMINMAX

#include "io/interfaces/BaseIoInterface.h"

namespace zen
{
    class FtdiUsbInterface : public BaseIoInterface
    {
    public:
        FtdiUsbInterface(FT_HANDLE handle, std::unique_ptr<modbus::IFrameFactory> factory, std::unique_ptr<modbus::IFrameParser> parser) noexcept;
        ~FtdiUsbInterface();

        /** Returns the IO interface's baudrate (bit/s) */
        ZenError baudrate(int32_t& rate) const override;

        /** Set Baudrate of Ftdi USB interface (bit/s) */
        ZenError setBaudrate(unsigned int rate) override;

        /** Returns the supported baudrates of the IO interface (bit/s) */
        ZenError supportedBaudrates(std::vector<int32_t>& outBaudrates) const override;

        /** Returns the type of IO interface */
        const char* type() const override;

        /** Returns whether the IO interface equals the sensor description */
        bool equals(const ZenSensorDesc& desc) const override;

    private:
        ZenError send(std::vector<unsigned char> frame) override;

        ZenError receiveInBuffer(bool& received);
        int run();

        std::atomic_bool m_terminate;
        std::thread m_pollingThread;
        std::vector<unsigned char> m_buffer;

        FT_HANDLE m_handle;

        unsigned int m_baudrate;
    };
}

#endif
