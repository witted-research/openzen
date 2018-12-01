#ifndef ZEN_IO_INTERFACES_BASEIOINTERFACE_H_
#define ZEN_IO_INTERFACES_BASEIOINTERFACE_H_

#include <vector>

#include "io/IIoInterface.h"

namespace zen
{
    class IIoDataSubscriber;

    class BaseIoInterface : public IIoInterface
    {
    public:
        friend class IIoDataSubscriber;

        ZenError send(uint8_t address, uint8_t function, const unsigned char* data, size_t length) final override;

        ZenError process(uint8_t address, uint8_t function, const unsigned char* data, size_t length);

    protected:
        virtual ZenError send(std::vector<unsigned char> frame) = 0;

    private:
        void setSubscriber(IIoDataSubscriber& subscriber) { m_subscriber = &subscriber; }

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
