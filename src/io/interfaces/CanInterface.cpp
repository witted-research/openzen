#include "io/interfaces/CanInterface.h"

#include <vector>

using namespace zen;

// [XXX] Set the id for CAN devices
CanInterface::CanInterface(uint32_t id, ICanChannel& channel)
    : m_channel(channel)
    , m_id(id)
{}

CanInterface::~CanInterface()
{
    m_channel.unsubscribe(*this);
}

ZenError CanInterface::poll()
{
    // No logic, as it is handled by the IPlatformCanEngine
    return ZenError_None;
}

ZenError CanInterface::send(std::vector<unsigned char> frame)
{
    return m_channel.send(m_id, std::move(frame));
}

ZenError CanInterface::baudrate(int32_t& rate) const
{
    rate = m_channel.baudrate();
    return ZenError_None;
}

ZenError CanInterface::setBaudrate(unsigned int rate)
{
    return m_channel.setBaudrate(rate);
}

ZenError CanInterface::supportedBaudrates(std::vector<int32_t>& outBaudrates) const
{
    return m_channel.supportedBaudrates(outBaudrates);
}

bool CanInterface::equals(const ZenSensorDesc& desc) const
{
    if (!m_channel.equals(desc.ioType))
        return false;

    return m_id == desc.handle32;
}
