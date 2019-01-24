#ifndef ZEN_IO_IIOINTERFACE_H_
#define ZEN_IO_IIOINTERFACE_H_

#include <cstdint>
#include <vector>
#include <string_view>

#include "LpMatrix.h"
#include "ZenTypes.h"

namespace zen
{
    class IIoInterface
    {
    public:
        virtual ~IIoInterface() = default;

        /** Poll data from IO interface */
        virtual ZenError poll() = 0;

        /** Send data to IO interface */
        virtual ZenError send(uint8_t address, uint8_t function, const unsigned char* data, size_t length) = 0;

        /** Returns the IO interface's baudrate (bit/s) */
        virtual ZenError baudrate(int32_t& rate) const = 0;

        /** Set Baudrate of IO interface (bit/s) */
        virtual ZenError setBaudrate(unsigned int rate) = 0;

        /** Returns the supported baudrates of the IO interface (bit/s) */
        virtual ZenError supportedBaudrates(std::vector<int32_t>& outBaudrates) const = 0;

        /** Returns the type of IO interface */
        virtual const char* type() const = 0;

        /** Returns whether the IO interface equals the sensor description */
        virtual bool equals(const ZenSensorDesc& desc) const = 0;
    };
}

#endif
