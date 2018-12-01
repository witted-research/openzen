#ifndef ZEN_IO_INTERFACES_FTDIUSBINTERFACE_H_
#define ZEN_IO_INTERFACES_FTDIUSBINTERFACE_H_

#include <string>
#include <string_view>

#define NOMINMAX
#include "ftd2xx.h"
#undef NOMINMAX

#include "io/Modbus.h"
#include "io/interfaces/BaseIoInterface.h"

namespace zen
{
    class FtdiUsbInterface : public BaseIoInterface
    {
    public:
        FtdiUsbInterface(FT_HANDLE handle);
        ~FtdiUsbInterface();

        /** Poll data from IO interface */
        ZenError poll() override;

        /** Returns the IO interface's baudrate (bit/s) */
        ZenError baudrate(int32_t& rate) const override;

        /** Set Baudrate of Ftdi USB interface (bit/s) */
        ZenError setBaudrate(unsigned int rate) override;

        /** Returns the supported baudrates of the IO interface (bit/s) */
        ZenError supportedBaudrates(std::vector<int32_t>& outBaudrates) const override;

        /** Returns whether the IO interface equals the sensor description */
        bool equals(const ZenSensorDesc& desc) const override;

    private:
        ZenError send(std::vector<unsigned char> frame) override;

        ZenError receiveInBuffer(bool& received);
        ZenError parseBuffer();

        std::vector<unsigned char> m_buffer;
        modbus::LpFrameParser m_parser;

        FT_HANDLE m_handle;

        unsigned int m_baudrate;
    };
}

#endif
