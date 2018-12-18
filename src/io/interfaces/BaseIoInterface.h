#ifndef ZEN_IO_INTERFACES_BASEIOINTERFACE_H_
#define ZEN_IO_INTERFACES_BASEIOINTERFACE_H_

#include <memory>
#include <vector>

#include "io/IIoInterface.h"
#include "io/Modbus.h"

namespace zen
{
    class IIoDataSubscriber;

    class BaseIoInterface : public IIoInterface
    {
    public:
        friend class IIoDataSubscriber;

        BaseIoInterface(std::unique_ptr<modbus::IFrameFactory> factory, std::unique_ptr<modbus::IFrameParser> parser) noexcept;

        ZenError send(uint8_t address, uint8_t function, const unsigned char* data, size_t length) final override;

    protected:
        ZenError processReceivedData(const unsigned char* data, size_t length);

        virtual ZenError send(std::vector<unsigned char> frame) = 0;

    private:
        void setSubscriber(IIoDataSubscriber& subscriber) { m_subscriber = &subscriber; }

        std::unique_ptr<modbus::IFrameFactory> m_factory;
        std::unique_ptr<modbus::IFrameParser> m_parser;
        IIoDataSubscriber* m_subscriber;
    };

    class IIoDataSubscriber
    {
    public:
        IIoDataSubscriber(BaseIoInterface& ioInterface) { ioInterface.setSubscriber(*this); }

        virtual ZenError processData(uint8_t address, uint8_t function, const unsigned char* data, size_t length) = 0;
    };
}

#endif
