#ifndef ZEN_IO_INTERFACES_BLUETOOTHINTERFACE_H_
#define ZEN_IO_INTERFACES_BLUETOOTHINTERFACE_H_

#include <string>
#include <string_view>

#include "BaseIoInterface.h"
#include "io/bluetooth/BluetoothDeviceHandler.h"

namespace zen
{
    class BluetoothInterface : public BaseIoInterface
    {
    public:
        BluetoothInterface(std::unique_ptr<BluetoothDeviceHandler> handler, std::unique_ptr<modbus::IFrameFactory> factory, std::unique_ptr<modbus::IFrameParser> parser) noexcept;
        ~BluetoothInterface() = default;

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

        /** Returns the type of IO interface */
        const char* type() const override;

        /** Returns whether the IO interface equals the sensor description */
        bool equals(const ZenSensorDesc& desc) const override;

    private:
        std::unique_ptr<BluetoothDeviceHandler> m_handler;
    };
}

#endif
