#ifndef ZEN_IO_INTERFACES_BLEINTERFACE_H_
#define ZEN_IO_INTERFACES_BLEINTERFACE_H_

#include <string>
#include <string_view>

#include "BaseIoInterface.h"
#include "io/ble/BleDeviceHandler.h"

namespace zen
{
    class BleInterface : public BaseIoInterface
    {
    public:
        BleInterface(std::unique_ptr<BleDeviceHandler> handler, std::unique_ptr<modbus::IFrameFactory> factory, std::unique_ptr<modbus::IFrameParser> parser) noexcept;
        ~BleInterface() = default;

        /** Poll data from IO interface */
        ZenError poll() override;

        /** Send data to IO interface */
        ZenError send(std::vector<unsigned char> frame) override;

        /** Returns the IO interface's baudrate (bit/s) */
        ZenError baudrate(int32_t& rate) const override;

        /** Set Baudrate of IO interface (bit/s) */
        ZenError setBaudrate(unsigned int rate) override;

        /** Returns the supported baudrates of the IO interface (bit/s) */
        ZenError supportedBaudrates(std::vector<int32_t>& outBaudrates) const override;

        /** Returns whether the IO interface equals the sensor description */
        bool equals(const ZenSensorDesc& desc) const override;

    private:
        std::unique_ptr<BleDeviceHandler> m_handler;
    };
}

#endif
