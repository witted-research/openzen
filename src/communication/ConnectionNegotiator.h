#ifndef ZEN_COMMUNICATION_CONNECTIONNEGOTIATOR_H_
#define ZEN_COMMUNICATION_CONNECTIONNEGOTIATOR_H_

#include <condition_variable>
#include <mutex>

#include "nonstd/expected.hpp"

#include "SensorConfig.h"
#include "communication/ModbusCommunicator.h"
#include "io/IoManager.h"

namespace zen
{
    class ConnectionNegotiator : public IModbusFrameSubscriber
    {
    public:
        ConnectionNegotiator() noexcept;

        nonstd::expected<SensorConfig, ZenSensorInitError> negotiate(ModbusCommunicator& communicator) const noexcept;

    private:
        ZenError processReceivedData(uint8_t address, uint8_t function, gsl::span<const std::byte> data) noexcept override;

    private:
        SensorConfig m_config;

        ZenSensorInitError m_error;
        bool m_terminated;

        mutable std::condition_variable m_cv;
        mutable std::mutex m_mutex;
    };
}

#endif
