#ifndef ZEN_IO_CAN_PCANBASICCHANNEL_H_
#define ZEN_IO_CAN_PCANBASICCHANNEL_H_

#include <set>
#include <unordered_map>

#define NOMINMAX
#include <windows.h>
#undef NOMINMAX

#include <PCANBasic.h>

#include "io/can/ICanChannel.h"

namespace zen
{
    class PcanBasicChannel : public ICanChannel
    {
    public:
        PcanBasicChannel(TPCANHandle channel);
        ~PcanBasicChannel();

        /** Subscribe IO Interface to CAN interface  */
        ZenError subscribe(CanInterface& i) override;

        /** Unsubscribe IO Interface from CAN interface */
        void unsubscribe(CanInterface& i) override;

        /** List devices connected to the CAN interface */
        ZenError listDevices(std::vector<ZenSensorDesc>& outDevices);

        /** Poll data from CAN bus */
        ZenError poll() override;

        /** Returns the channel Id */
        unsigned int channel() const override { return m_channel; }

        /** Returns the type of IO interface */
        const char* type() const override;

        /** Returns whether the CAN channel equals the IO type */
        bool equals(std::string_view ioType) const override;

    private:
        /** Send data to CAN bus */
        ZenError send(uint32_t id, std::vector<unsigned char> frame) override;

        /** Returns the CAN bus' baudrate (bit/s) */
        unsigned baudrate() const { return m_baudrate; }

        /** Set Baudrate of CAN bus (bit/s) */
        ZenError setBaudrate(unsigned int rate) override;

        /** Returns the supported baudrates of the CAN bus (bit/s) */
        ZenError supportedBaudrates(std::vector<int32_t>& outBaudrates) const override;

        bool hasBusError();

        std::set<uint32_t> m_deviceIds;

        // Simple bi-directional map of pointers
        std::unordered_map<uint32_t, CanInterface*> m_subscribers;
        std::unordered_map<CanInterface*, uint32_t> m_subscribers2;

        TPCANHandle m_channel;
        unsigned int m_baudrate;
    };
}

#endif
