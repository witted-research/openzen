#ifndef ZEN_IO_INTERFACES_WINDOWS_WINDOWSDEVICEINTERFACE_H_
#define ZEN_IO_INTERFACES_WINDOWS_WINDOWSDEVICEINTERFACE_H_

#include <array>
#include <string>
#include <string_view>

#define NOMINMAX
#include <Windows.h>
#undef NOMINMAX

#include "io/Modbus.h"
#include "io/interfaces/BaseIoInterface.h"

namespace zen
{
    class WindowsDeviceInterface : public BaseIoInterface
    {
    public:
        WindowsDeviceInterface(HANDLE handle);
        ~WindowsDeviceInterface();

        /** Poll data from IO interface */
        ZenError poll() override;

        /** Returns the IO interface's baudrate (bit/s) */
        ZenError baudrate(int32_t& rate) const override;

        /** Set Baudrate of IO interface (bit/s) */
        ZenError setBaudrate(unsigned int rate) override;

        /** Returns the supported baudrates of the IO interface (bit/s) */
        ZenError supportedBaudrates(std::vector<int32_t>& outBaudrates) const override;

        /** Returns whether the IO interface equals the sensor description */
        bool equals(const ZenSensorDesc& desc) const override;

    private:
        /** Send data to IO interface */
        ZenError send(std::vector<unsigned char> frame) override;

        ZenError receiveInBuffer(bool& received);
        ZenError parseBuffer();

        std::array<unsigned char, 1024> m_buffer;
        modbus::ASCIIFrameParser m_parser;
        size_t m_usedBufferSize;

        std::string m_deviceId;

        DCB m_config;
        HANDLE m_handle;

        unsigned int m_baudrate;
    };
}

#endif
