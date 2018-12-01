#include "io/can/PcanBasicChannel.h"

#include <limits>

#include "io/Modbus.h"
#include "io/can/CanManager.h"
#include "io/interfaces/CanInterface.h"
#include "io/systems/PcanBasicSystem.h"

#include "utility/Finally.h"

namespace zen
{
    namespace
    {
        std::optional<uint8_t> getAvailableId(const std::unordered_map<uint8_t, CanInterface*>& map)
        {
            for (unsigned int id = 0; id <= std::numeric_limits<uint8_t>::max(); ++id)
                if (map.find(id) == map.end())
                    return id;

            return {};
        }

        TPCANMsg makeMsg(uint32_t id, unsigned char type, const unsigned char* data, uint8_t length)
        {
            TPCANMsg m;
            m.ID = id;
            m.MSGTYPE = type;
            m.LEN = length;
            std::memcpy(m.DATA, data, length);
            return m;
        }

        ZenError sendMsg(TPCANHandle channel, uint32_t id, const unsigned char* data, uint8_t length)
        {
            TPCANMsg m = makeMsg(id, PCAN_MESSAGE_STANDARD, data, length);
            if (auto error = PcanBasicSystem::fnTable.write(channel, &m))
                return ZenError_Io_SendFailed;

            return ZenError_None;
        }

        unsigned int mapBaudrate(unsigned int rate)
        {
            // Round up to prevent aliasing
            if (rate > 800000)
                return PCAN_BAUD_1M;
            else if (rate > 500000)
                return PCAN_BAUD_800K;
            else if (rate > 250000)
                return PCAN_BAUD_500K;
            else if (rate > 125000)
                return PCAN_BAUD_250K;
            else if (rate > 100000)
                return PCAN_BAUD_125K;
            else if (rate > 95000)
                return PCAN_BAUD_100K;
            else if (rate > 83000)
                return PCAN_BAUD_95K;
            else if (rate > 50000)
                return PCAN_BAUD_83K;
            else if (rate > 47000)
                return PCAN_BAUD_50K;
            else if (rate > 33000)
                return PCAN_BAUD_47K;
            else if (rate > 20000)
                return PCAN_BAUD_33K;
            else if (rate > 10000)
                return PCAN_BAUD_20K;
            else if (rate > 5000)
                return PCAN_BAUD_10K;
            else
                return PCAN_BAUD_5K;
        }
    }

    PcanBasicChannel::PcanBasicChannel(TPCANHandle channel)
        : m_channel(channel)
    {}

    PcanBasicChannel::~PcanBasicChannel()
    {
        CanManager::get().unregisterChannel(*this);

        PcanBasicSystem::fnTable.uninitialize(m_channel);
    }

    ZenError PcanBasicChannel::subscribe(CanInterface& i)
    {
        auto it = m_subscribers2.find(&i);
        if (it != m_subscribers2.cend())
            return ZenError_None;

        if (auto id = getAvailableId(m_subscribers))
        {
            // [XXX] How do we notify the opposite side of our canId?
            m_subscribers.emplace(*id, &i);
            m_subscribers2.emplace(&i, *id);
            return ZenError_None;
        }

        return ZenError_Can_AddressOutOfRange;
    }

    void PcanBasicChannel::unsubscribe(CanInterface& i)
    {
        auto it2 = m_subscribers2.find(&i);
        if (it2 != m_subscribers2.cend())
        {
            m_subscribers.erase(it2->second);
            m_subscribers2.erase(it2);
        }
    }

    ZenError PcanBasicChannel::listDevices(std::vector<ZenSensorDesc>& outDevices)
    {
        for (uint32_t deviceId : m_deviceIds)
        {
            ZenSensorDesc desc;
            desc.handle32 = deviceId;
            desc.sensorType = ZenSensor_Imu;
            std::memcpy(desc.ioType, PcanBasicSystem::KEY, sizeof(PcanBasicSystem::KEY));
            outDevices.emplace_back(desc);
        }

        // As we do not garbage collect device ids, reset and collect again
        m_deviceIds.clear();

        return ZenError_None;
    }

    ZenError PcanBasicChannel::poll()
    {
        TPCANMsg m;
        TPCANTimestamp t;

        while (!m_parser.finished())
        {
            if (auto error = PcanBasicSystem::fnTable.read(m_channel, &m, &t))
                return error == PCAN_ERROR_QRCVEMPTY ? ZenError_None : ZenError_Io_ReadFailed; // Is there data in the receive queue?

            if (m.ID != CanManager::get().id())
            {
                m_deviceIds.emplace(m.ID);
                continue;
            }

            size_t length = m.LEN;
            if (auto error = m_parser.parse(m.DATA, length))
                return ZenError_Io_MsgCorrupt;
        }

        auto guard = finally([this]() {
            m_parser.reset();
        });

        const auto& frame = m_parser.frame();
        auto it = m_subscribers.find(frame.address);
        if (it == m_subscribers.end())
            return ZenError_None; // [XXX] Should we return an error?

        return it->second->process(frame.address, frame.function, frame.data.data(), frame.data.size());
    }

    bool PcanBasicChannel::equals(std::string_view ioType) const
    {
        return ioType == PcanBasicSystem::KEY;
    }

    ZenError PcanBasicChannel::send(uint32_t id, std::vector<unsigned char> frame)
    {
        // [XXX] Should we sleep to see if the hasBusError() dissappears?
        if (hasBusError())
            return ZenError_Can_BusError;

        const auto nFullFrames = frame.size() / 8;
        const uint8_t remainder = frame.size() % 8;

        auto it = frame.data();
        for (auto i = 0; i < nFullFrames; ++i, it += 8)
            if (auto error = sendMsg(m_channel, id, it, 8))
                return error;

        if (remainder > 0)
            if (auto error = sendMsg(m_channel, id, it, remainder))
                return error;

        return ZenError_None;
    }

    ZenError PcanBasicChannel::setBaudrate(unsigned int rate)
    {
        // [XXX] Make sure there are no messages being send
        //if (auto error = close())
        //    return error;

        rate = mapBaudrate(rate);
        if (rate == m_baudrate)
            return ZenError_None;

        m_baudrate = rate;
        //if (auto error = open())
        //    return error;

        // [XXX] Changed order. Does it cause errors?
        if (auto error = PcanBasicSystem::fnTable.reset(m_channel))
            return ZenError_Can_ResetFailed;

        return ZenError_None;
    }

    ZenError PcanBasicChannel::supportedBaudrates(std::vector<int32_t>& outBaudrates) const
    {
        outBaudrates.reserve(14);
        outBaudrates.emplace_back(5000);
        outBaudrates.emplace_back(10000);
        outBaudrates.emplace_back(20000);
        outBaudrates.emplace_back(33000);
        outBaudrates.emplace_back(47000);
        outBaudrates.emplace_back(50000);
        outBaudrates.emplace_back(83000);
        outBaudrates.emplace_back(95000);
        outBaudrates.emplace_back(100000);
        outBaudrates.emplace_back(125000);
        outBaudrates.emplace_back(250000);
        outBaudrates.emplace_back(500000);
        outBaudrates.emplace_back(800000);
        outBaudrates.emplace_back(1000000);
        return ZenError_None;
    }

    bool PcanBasicChannel::hasBusError()
    {
        return PcanBasicSystem::fnTable.getStatus(m_channel) == PCAN_ERROR_ANYBUSERR;
    }
}
