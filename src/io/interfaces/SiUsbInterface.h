#ifndef ZEN_IO_INTERFACES_SIUSBINTERFACE_H_
#define ZEN_IO_INTERFACES_SIUSBINTERFACE_H_

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
        SiUsbInterface(HANDLE handle, std::unique_ptr<modbus::IFrameFactory> factory, std::unique_ptr<modbus::IFrameParser> parser) noexcept;
        ~SiUsbInterface();

        /** Poll data from IO interface */
        ZenError poll() override;

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
        ZenError receiveInBuffer(bool& received);

        std::vector<unsigned char> m_buffer;

        HANDLE m_handle;

        unsigned int m_baudrate;
    };
}

#endif
