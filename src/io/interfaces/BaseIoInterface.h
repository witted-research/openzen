#ifndef ZEN_IO_INTERFACES_BASEIOINTERFACE_H_
#define ZEN_IO_INTERFACES_BASEIOINTERFACE_H_

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

        ZenError send(uint8_t address, uint8_t function, const unsigned char* data, size_t length) final override;

    protected:
        ZenError processReceivedData(const unsigned char* data, size_t length);

        virtual ZenError send(std::vector<unsigned char> frame) = 0;

    private:
        void setSubscriber(IIoDataSubscriber& subscriber) { m_subscriber = &subscriber; }

        modbus::LpFrameParser m_parser;
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
