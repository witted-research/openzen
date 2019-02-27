#ifndef ZEN_COMMUNICATION_MODBUSCOMMUNICATOR_H_
#define ZEN_COMMUNICATION_MODBUSCOMMUNICATOR_H_

#include <mutex>

#include "Modbus.h"
#include "io/IIoInterface.h"

namespace zen
{
    class IModbusFrameSubscriber;

    class ModbusCommunicator : public IIoDataSubscriber
    {
    public:
        friend class IModbusFrameSubscriber;

        ModbusCommunicator(IModbusFrameSubscriber& subscriber, std::unique_ptr<modbus::IFrameFactory> factory, std::unique_ptr<modbus::IFrameParser> parser) noexcept;
        ModbusCommunicator(ModbusCommunicator&&) noexcept = default;

        void init(std::unique_ptr<IIoInterface> ioInterface) noexcept;

        ZenError send(uint8_t address, uint8_t function, gsl::span<const std::byte> data) noexcept;

        /** Returns whether the IO interface equals the sensor description */
        bool equals(const ZenSensorDesc& desc) const noexcept { return m_ioInterface->equals(desc); }

        /** Returns the IO interface's baudrate (bit/s) */
        nonstd::expected<int32_t, ZenError> baudRate() const noexcept { return m_ioInterface->baudRate(); }

        /** Set Baudrate of IO interface (bit/s) */
        ZenError setBaudRate(unsigned int rate) noexcept { return m_ioInterface->setBaudRate(rate); }

        /** Returns the supported baudrates of the IO interface (bit/s) */
        nonstd::expected<std::vector<int32_t>, ZenError> supportedBaudRates() const noexcept { return m_ioInterface->supportedBaudRates(); }

        /** Returns the type of IO interface */
        std::string_view ioType() const noexcept { return m_ioInterface->type(); }

        void setSubscriber(IModbusFrameSubscriber& subscriber) noexcept { m_subscriber = &subscriber; }
        void setFrameFactory(std::unique_ptr<modbus::IFrameFactory> factory) noexcept { m_factory = std::move(factory); }
        void setFrameParser(std::unique_ptr<modbus::IFrameParser> parser) noexcept { m_parser = std::move(parser); }

    private:
        ZenError processData(gsl::span<const std::byte> data) noexcept override;

        IModbusFrameSubscriber* m_subscriber;
        std::unique_ptr<modbus::IFrameFactory> m_factory;
        std::unique_ptr<modbus::IFrameParser> m_parser;
        std::unique_ptr<IIoInterface> m_ioInterface;
    };

    class IModbusFrameSubscriber
    {
    public:
        virtual ZenError processReceivedData(uint8_t address, uint8_t function, gsl::span<const std::byte> data) noexcept = 0;
    };
}

#endif
