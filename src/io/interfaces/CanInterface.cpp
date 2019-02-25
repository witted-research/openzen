#include "io/interfaces/CanInterface.h"

#include <vector>

#include "io/can/ICanChannel.h"

using namespace zen;

CanInterface::CanInterface(IIoDataSubscriber& subscriber, ICanChannel& channel, uint32_t id) noexcept
    : IIoInterface(subscriber)
    , m_channel(channel)
    , m_id(id)
{}

CanInterface::~CanInterface()
{
    m_channel.unsubscribe(*this);
}

nonstd::expected<int32_t, ZenError> CanInterface::baudRate() const noexcept
{
    return m_channel.baudRate();
}

ZenError CanInterface::setBaudRate(unsigned int rate) noexcept
{
    return m_channel.setBaudRate(rate);
}

nonstd::expected<std::vector<int32_t>, ZenError> CanInterface::supportedBaudRates() const noexcept
{
    return m_channel.supportedBaudRates();
}

std::string_view CanInterface::type() const noexcept
{
    return m_channel.type();
}

bool CanInterface::equals(const ZenSensorDesc& desc) const noexcept
{
    if (!m_channel.equals(desc.ioType))
        return false;

    return m_id == desc.handle32;
}

ZenError CanInterface::send(gsl::span<const std::byte> data) noexcept
{
    return m_channel.send(m_id, data);
}
