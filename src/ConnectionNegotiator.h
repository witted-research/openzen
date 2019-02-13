#ifndef ZEN_IO_BAUDRATENEGOTIATOR_H_
#define ZEN_IO_BAUDRATENEGOTIATOR_H_

#include <condition_variable>
#include <mutex>

#include "nonstd/expected.hpp"

#include "SensorConfig.h"
#include "io/interfaces/BaseIoInterface.h"

namespace zen
{
    class ConnectionNegotiator : private IIoDataSubscriber
    {
    public:
        ConnectionNegotiator(BaseIoInterface& ioInterace) noexcept;

        nonstd::expected<SensorConfig, ZenSensorInitError> negotiate() noexcept;

    private:
        ZenError processData(uint8_t address, uint8_t function, const unsigned char* data, size_t length) override;

    private:
        BaseIoInterface& m_ioInterface;
        std::condition_variable m_cv;
        std::mutex m_mutex;

        SensorConfig m_config;
        ZenSensorInitError m_error;
        bool m_terminated;
    };
}

#endif
