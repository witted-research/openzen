#include "io/interfaces/BaseIoInterface.h"

#include "io/Modbus.h"
#include "sensors/BaseSensor.h"
#include "utility/Finally.h"

namespace zen
{
    ZenError BaseIoInterface::send(uint8_t address, uint8_t function, const unsigned char* data, size_t length)
    {
        if (data == nullptr && length > 0)
            return ZenError_IsNull;

        if (length > std::numeric_limits<uint8_t>::max())
            return ZenError_Io_MsgTooBig;

        auto frame = modbus::makeLpFrame(address, function, data, static_cast<uint8_t>(length));
        return send(std::move(frame));
    }

    ZenError BaseIoInterface::process(uint8_t address, uint8_t function, const unsigned char* data, size_t length)
    {
        return m_subscriber->processData(address, function, data, length);
    }
}
