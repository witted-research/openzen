#ifndef ZEN_IO_IIOSYSTEM_H_
#define ZEN_IO_IIOSYSTEM_H_

#include <memory>
#include <vector>

#include "ZenTypes.h"
#include "io/interfaces/BaseIoInterface.h"

namespace zen
{
    class IIoSystem
    {
    public:
        virtual ~IIoSystem() = default;

        virtual bool available() = 0;

        virtual ZenError listDevices(std::vector<ZenSensorDesc>& outDevices) = 0;

        virtual std::unique_ptr<BaseIoInterface> obtain(const ZenSensorDesc& desc, ZenError& outError) = 0;
    };
}

#endif
