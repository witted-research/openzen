#ifndef ZEN_IO_CAN_ICANCHANNEL_H_
#define ZEN_IO_CAN_ICANCHANNEL_H_

#include <cstdint>
#include <optional>
#include <string_view>
#include <vector>

#include "ZenTypes.h"

namespace zen
{
    class CanInterface;
    class CanManager;

    class ICanChannel
    {
    protected:
        /** Protected destructor to prevent usage of pointer to base class */
        ~ICanChannel() = default;

    public:
        friend class CanInterface;
        friend class CanManager;

        /** Subscribe IO Interface to CAN interface  */
        virtual ZenError subscribe(CanInterface& i) = 0;

        /** Unsubscribe IO Interface from CAN interface */
        virtual void unsubscribe(CanInterface& i) = 0;

        /** List devices connected to the CAN interface */
        virtual ZenError listDevices(std::vector<ZenSensorDesc>& outDevices) = 0;

        /** Poll data from CAN bus */
        virtual ZenError poll() = 0;

        /** Returns the channel Id */
        virtual unsigned int channel() const = 0;

        /** Returns the type of IO used by the CAN channel */
        virtual const char* type() const = 0;

        /** Returns whether the CAN channel equals the IO type */
        virtual bool equals(std::string_view ioType) const = 0;

    private:
        /** Send data to CAN bus */
        virtual ZenError send(uint32_t id, std::vector<unsigned char> frame) = 0;

        /** Returns the IO interface's baudrate (bit/s) */
        virtual unsigned baudrate() const = 0;

        /** Set Baudrate of CAN bus (bit/s) */
        virtual ZenError setBaudrate(unsigned int rate) = 0;

        /** Returns the supported baudrates of the IO interface (bit/s) */
        virtual ZenError supportedBaudrates(std::vector<int32_t>& outBaudrates) const = 0;
    };
}

#endif
