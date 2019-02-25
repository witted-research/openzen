#ifndef ZEN_COMMUNICATION_SYNCEDMODBUSCOMMUNICATOR_H_
#define ZEN_COMMUNICATION_SYNCEDMODBUSCOMMUNICATOR_H_

#include <atomic>
#include <memory>
#include <vector>

#include <gsl/span>
#include <nonstd/expected.hpp>

#include "communication/ModbusCommunicator.h"
#include "utility/ThreadFence.h"

namespace zen
{
    class SyncedModbusCommunicator
    {
    public:
        SyncedModbusCommunicator(std::unique_ptr<ModbusCommunicator> communicator) noexcept;

        /** Returns the IO interface's baudrate (bit/s) */
        nonstd::expected<int32_t, ZenError> baudRate() const noexcept { return m_communicator->baudRate(); }

        /** Set BaudRate of IO interface (bit/s) */
        ZenError setBaudRate(unsigned int rate) noexcept { return m_communicator->setBaudRate(rate); }

        /** Returns the supported baudrates of the IO interface (bit/s) */
        nonstd::expected<std::vector<int32_t>, ZenError> supportedBaudRates() const noexcept { return m_communicator->supportedBaudRates(); }

        /** Returns the type of IO interface */
        std::string_view ioType() const noexcept { return m_communicator->ioType(); }

        /** Returns whether the IO interface equals the sensor description */
        bool equals(const ZenSensorDesc& desc) const noexcept { return m_communicator->equals(desc); }

        ZenError sendAndWaitForAck(uint8_t address, uint8_t function, ZenProperty_t property, gsl::span<const std::byte> data) noexcept;

        template <typename T>
        std::pair<ZenError, size_t> sendAndWaitForArray(uint8_t address, uint8_t function, ZenProperty_t property, gsl::span<const std::byte> data, gsl::span<T> outArray) noexcept;

        template <typename T>
        nonstd::expected<T, ZenError> sendAndWaitForResult(uint8_t address, uint8_t function, ZenProperty_t property, gsl::span<const std::byte> data) noexcept;

        ZenError publishAck(ZenProperty_t property, ZenError error) noexcept;

        template <typename T>
        ZenError publishArray(ZenProperty_t property, ZenError error, gsl::span<const T> array) noexcept;

        template <typename T>
        ZenError publishResult(ZenProperty_t property, ZenError error, const T& result) noexcept;

    private:
        ZenError terminateWaitOnPublishOrTimeout() noexcept;

        ZenError tryToWait(ZenProperty_t property, bool forAck) noexcept;

        bool prepareForPublishing() noexcept;

        bool corruptMessage(ZenProperty_t property, bool isAck) noexcept;

        std::unique_ptr<ModbusCommunicator> m_communicator;

        ThreadFence m_fence;
        std::atomic_flag m_waiting;
        std::atomic_flag m_publishing;

        ZenError m_resultError;
        ZenProperty_t m_property;
        bool m_waitingForAck;  // If not, waiting for data

        void* m_resultPtr;
        size_t m_resultSize;
    };
}

#endif
