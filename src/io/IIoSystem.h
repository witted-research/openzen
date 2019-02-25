#ifndef ZEN_IO_IIOSYSTEM_H_
#define ZEN_IO_IIOSYSTEM_H_

#include <memory>
#include <vector>

#include <nonstd/expected.hpp>

#include "ZenTypes.h"
#include "io/IIoInterface.h"

namespace zen
{
    class IIoSystem
    {
    public:
        virtual ~IIoSystem() = default;

        virtual bool available() = 0;

        virtual ZenError listDevices(std::vector<ZenSensorDesc>& outDevices) = 0;

        virtual nonstd::expected<std::unique_ptr<IIoInterface>, ZenSensorInitError> obtain(const ZenSensorDesc& desc, IIoDataSubscriber& subscriber) noexcept = 0;
    };
}

#endif
