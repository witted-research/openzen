//===========================================================================//
//
// Copyright (C) 2020 LP-Research Inc.
//
// This file is part of OpenZen, under the MIT License.
// See https://bitbucket.org/lpresearch/openzen/src/master/LICENSE for details
// SPDX-License-Identifier: MIT
//
//===========================================================================//

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
    /** The synchronised communication pipeline */
    class SyncedModbusCommunicator
    {
    public:
        SyncedModbusCommunicator(std::unique_ptr<ModbusCommunicator> communicator) noexcept;

        /** Close the IO interface. It is no longer usable after this point! */
        void close() { m_communicator.reset(); }

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

        /** Sends data to the IO interface, and waits for an acknowledgment */
        ZenError sendAndWaitForAck(uint8_t address, uint8_t function, ZenProperty_t property, gsl::span<const std::byte> data) noexcept;

        ZenError sendAndDontWait(uint8_t address, uint8_t function, ZenProperty_t property,
            gsl::span<const std::byte> data) noexcept;

        /** Sends data to the IO interface, and waits for a result array */
        template <typename T>
        std::pair<ZenError, size_t> sendAndWaitForArray(uint8_t address, uint8_t function, ZenProperty_t property, gsl::span<const std::byte> data, gsl::span<T> outArray) noexcept;

        /** Sends data to the IO interface, and waits for a result value */
        template <typename T>
        nonstd::expected<T, ZenError> sendAndWaitForResult(uint8_t address, uint8_t function, ZenProperty_t property, gsl::span<const std::byte> data) noexcept;

        /** Publish an acknowledgement from the IO interface */
        ZenError publishAck(ZenProperty_t property, ZenError error) noexcept;

        /** Publish a result array from the IO interface */
        template <typename T>
        ZenError publishArray(ZenProperty_t property, ZenError error, gsl::span<const T> array) noexcept;

        /** Publish a result value from the IO interface */
        template <typename T>
        ZenError publishResult(ZenProperty_t property, ZenError error, T result) noexcept;

    private:
        /** Wait until a response has been published from the IO interface, or timeout. */
        ZenError terminateWaitOnPublishOrTimeout() noexcept;

        /** Try to obtain access to the IO interface. If successful, the caller gains unique access, otherwise returns an error. */
        ZenError tryToWait(ZenProperty_t property, bool forAck) noexcept;

        /** Signal that you are going to publish a response. */
        bool prepareForPublishing() noexcept;

        /** Check whether the response is corrupt. */
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
