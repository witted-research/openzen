//===========================================================================//
//
// Copyright (C) 2020 LP-Research Inc.
//
// This file is part of OpenZen, under the MIT License.
// See https://bitbucket.org/lpresearch/openzen/src/master/LICENSE for details
// SPDX-License-Identifier: MIT
//
//===========================================================================//

#ifndef ZEN_COMMUNICATION_MODBUSCOMMUNICATOR_H_
#define ZEN_COMMUNICATION_MODBUSCOMMUNICATOR_H_

#include <atomic>

#include "Modbus.h"
#include "io/IIoInterface.h"

namespace zen
{
    class IModbusFrameSubscriber;

    class ModbusCommunicator : public IIoDataSubscriber
    {
    public:
        friend class IModbusFrameSubscriber;

        ModbusCommunicator(IModbusFrameSubscriber& subscriber,
          std::unique_ptr<modbus::IFrameFactory> factory, std::unique_ptr<modbus::IFrameParser> parser) noexcept;

        virtual ~ModbusCommunicator() = default;

        void init(std::unique_ptr<IIoInterface> ioInterface) noexcept;

        virtual ZenError send(uint8_t address, uint8_t function, gsl::span<const std::byte> data) noexcept;

        /** Returns whether the IO interface equals the sensor description */
        bool equals(const ZenSensorDesc& desc) const noexcept { return m_ioInterface->equals(desc); }

        /** Returns the IO interface's baudrate (bit/s) */
        nonstd::expected<int32_t, ZenError> baudRate() const noexcept { return m_ioInterface->baudRate(); }

        /** Set Baudrate of IO interface (bit/s) */
        virtual ZenError setBaudRate(unsigned int rate) noexcept { return m_ioInterface->setBaudRate(rate); }

        /** Returns the supported baudrates of the IO interface (bit/s) */
        nonstd::expected<std::vector<int32_t>, ZenError> supportedBaudRates() const noexcept {
           return m_ioInterface->supportedBaudRates(); }

        /** Returns the type of IO interface */
        std::string_view ioType() const noexcept { return m_ioInterface->type(); }

        void setSubscriber(IModbusFrameSubscriber& subscriber) noexcept { m_subscriber = &subscriber; }
        void setFrameFactory(std::unique_ptr<modbus::IFrameFactory> factory) noexcept { m_factory = std::move(factory); }
        void setFrameParser(std::unique_ptr<modbus::IFrameParser> parser) noexcept
        {
            // The frame parser is only set once - on initialisation of a new sensor. We have chosen to
            // use a spinlock, as it will be virtually cost-free (no system calls) in all other use cases.
            while (m_parserBusy.test_and_set(std::memory_order_acquire)) { /* Spin lock */; }
            m_parser = std::move(parser);
            m_parserBusy.clear(std::memory_order_release);
        }

        /** Forcefully reset the parser state. This is useful when starting to parse the data stream
         *  and we are not sure whether we found the start of a package properly */
        void resetParser() {
            m_parser->reset();
        }

    protected:
        IModbusFrameSubscriber* m_subscriber;

    private:
        ZenError processData(gsl::span<const std::byte> data) noexcept override;

        std::unique_ptr<modbus::IFrameFactory> m_factory;

        /** Access to the parser is only allowed if the m_parserBusy flag is true, because the
            parser object might be replaced after the connection to the sensor is established.
         */
        std::unique_ptr<modbus::IFrameParser> m_parser;
        std::atomic_flag m_parserBusy = ATOMIC_FLAG_INIT;
        std::unique_ptr<IIoInterface> m_ioInterface;
    };

    class IModbusFrameSubscriber
    {
    public:
        virtual ZenError processReceivedData(uint8_t address, uint8_t function,
          gsl::span<const std::byte> data) noexcept = 0;
    };
}

#endif
