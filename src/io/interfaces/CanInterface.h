#ifndef ZEN_IO_INTERFACES_CANINTERFACE_H_
#define ZEN_IO_INTERFACES_CANINTERFACE_H_

#include <cstdint>

#include "ZenTypes.h"

#include "io/can/ICanChannel.h"
#include "io/interfaces/BaseIoInterface.h"

namespace zen
{
    class CanInterface : public BaseIoInterface
    {
    public:
        CanInterface(uint32_t id, ICanChannel& channel);
        ~CanInterface();

        ZenError poll() override;

        /** Returns the CAN interface's baudrate (bit/s) */
        ZenError baudrate(int32_t& rate) const override;

        /** Set Baudrate of CAN interface (bit/s) */
        ZenError setBaudrate(unsigned int rate) override;

        /** Returns the supported baudrates of the CAN interface (bit/s) */
        ZenError supportedBaudrates(std::vector<int32_t>& outBaudrates) const override;

        /** Returns whether the IO interface equals the sensor description */
        bool equals(const ZenSensorDesc& desc) const override;

    private:
        ZenError send(std::vector<unsigned char> frame) override;

        ICanChannel& m_channel;

        uint32_t m_id;
    };
}

#endif