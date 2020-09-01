//===========================================================================//
//
// Copyright (C) 2020 LP-Research Inc.
//
// This file is part of OpenZen, under the MIT License.
// See https://bitbucket.org/lpresearch/openzen/src/master/LICENSE for details
// SPDX-License-Identifier: MIT
//
//===========================================================================//

#include "SyncedModbusCommunicator.h"

#include <cstring>

#include "utility/Finally.h"

namespace zen
{
    SyncedModbusCommunicator::SyncedModbusCommunicator(std::unique_ptr<ModbusCommunicator> communicator) noexcept
        : m_communicator(std::move(communicator))
        , m_waiting ATOMIC_FLAG_INIT
        , m_publishing ATOMIC_FLAG_INIT
        , m_resultError(ZenError_None)
    {}

    ZenError SyncedModbusCommunicator::sendAndWaitForAck(uint8_t address, uint8_t function, ZenProperty_t property,
        gsl::span<const std::byte> data) noexcept
    {
        if (auto error = tryToWait(property, true)) {
            return error;
        }

        auto guard = finally([this]() {
            m_waiting.clear();
        });

        if (auto error = m_communicator->send(address, function, data))
            return error;

        return terminateWaitOnPublishOrTimeout();
    }

    ZenError SyncedModbusCommunicator::sendAndDontWait(uint8_t address, uint8_t function, ZenProperty_t,
        gsl::span<const std::byte> data) noexcept
    {
        if (auto error = m_communicator->send(address, function, data))
            return error;

        return ZenError_None;
    }

    template <typename T>
    std::pair<ZenError, size_t> SyncedModbusCommunicator::sendAndWaitForArray(uint8_t address, uint8_t function,
        ZenProperty_t property, gsl::span<const std::byte> data, gsl::span<T> outArray) noexcept
    {
        if (auto error = tryToWait(property, false))
            return std::make_pair(error, outArray.size());

        m_resultPtr = outArray.data();
        // size() returns the number of elements in the span
        // and not the buffer size in bytes;
        m_resultSize = outArray.size() * sizeof(T);

        auto guard = finally([this]() {
            m_resultPtr = nullptr;
            m_waiting.clear();
        });

        if (auto error = m_communicator->send(address, function, data))
            return std::make_pair(error, outArray.size());

        if (auto error = terminateWaitOnPublishOrTimeout())
            return std::make_pair(error, outArray.size());

        return std::make_pair(ZenError_None, m_resultSize);
    }

    template <typename T>
    nonstd::expected<T, ZenError> SyncedModbusCommunicator::sendAndWaitForResult(uint8_t address, uint8_t function,
        ZenProperty_t property, gsl::span<const std::byte> data) noexcept
    {
        if (auto error = tryToWait(property, false))
            return nonstd::make_unexpected(error);

        T result;
        m_resultPtr = &result;

        auto guard = finally([this]() {
            m_resultPtr = nullptr;
            m_waiting.clear();
        });

        if (auto error = m_communicator->send(address, function, data))
            return nonstd::make_unexpected(error);

        if (auto error = terminateWaitOnPublishOrTimeout())
            return nonstd::make_unexpected(error);

        return result;
    }

    ZenError SyncedModbusCommunicator::publishAck(ZenProperty_t property, ZenError error) noexcept
    {
        if (!prepareForPublishing())
            return ZenError_None;

        auto guard = finally([this]() {
            m_publishing.clear();
        });

        if (corruptMessage(property, true))
            return m_resultError = ZenError_Io_UnexpectedFunction;

        m_resultError = error;
        m_fence.terminate();
        return ZenError_None;
    }

    template <typename T>
    ZenError SyncedModbusCommunicator::publishArray(ZenProperty_t property, ZenError error,
        gsl::span<const T> array) noexcept
    {
        if (!prepareForPublishing())
            return ZenError_None;

        auto guard = finally([this]() {
            m_publishing.clear();
        });

        if (corruptMessage(property, false))
            return m_resultError = ZenError_Io_MsgCorrupt;

        const auto bufferLength = m_resultSize;
        // size() returns the number of elements in the span
        // and not the buffer size in bytes;
        m_resultSize = array.size() * sizeof(T);

        auto guard2 = finally([this]() {
            m_fence.terminate();
        });

        if (m_resultSize > bufferLength)
            return m_resultError = ZenError_BufferTooSmall;

        if (array.data() == nullptr)
            return m_resultError = ZenError_IsNull;

        m_resultError = error;
        std::memcpy(m_resultPtr, array.data(), array.size_bytes());
        return ZenError_None;
    }

    template <typename T>
    ZenError SyncedModbusCommunicator::publishResult(ZenProperty_t property, ZenError error, T result) noexcept
    {
        if (!prepareForPublishing())
            return ZenError_None;

        auto guard = finally([this]() {
            m_publishing.clear();
        });

        if (corruptMessage(property, false))
            return m_resultError = ZenError_Io_MsgCorrupt;

        m_resultError = error;
        *reinterpret_cast<T*>(m_resultPtr) = result;
        m_fence.terminate();
        return ZenError_None;
    }

    ZenError SyncedModbusCommunicator::tryToWait(ZenProperty_t property, bool forAck) noexcept
    {
        if (m_waiting.test_and_set())
            return ZenError_Io_Busy;

        m_waitingForAck = forAck;
        m_property = property;
        m_resultError = ZenError_None;

        return ZenError_None;
    }

    ZenError SyncedModbusCommunicator::terminateWaitOnPublishOrTimeout() noexcept
    {
        constexpr static auto IO_TIMEOUT = std::chrono::milliseconds(2500);

        if (!m_fence.waitFor(IO_TIMEOUT))
        {
            // Second chance, in case we timed out right after the interface started publishing
            if (!m_publishing.test_and_set())
            {
                m_publishing.clear();
                return ZenError_Io_Timeout;
            }

            m_fence.wait();
        }

        // If terminated, reset
        const auto error = m_resultError;
        m_fence.reset();
        return error;
    }

    bool SyncedModbusCommunicator::prepareForPublishing() noexcept
    {
        // In case a waiter is checking whether we are publishing, we are already too late
        if (m_publishing.test_and_set())
            return false;

        // If no one is waiting, there is no need to publish
        if (!m_waiting.test_and_set())
        {
            m_waiting.clear();
            m_publishing.clear();
            return false;
        }

        return true;
    }

    bool SyncedModbusCommunicator::corruptMessage(ZenProperty_t property, bool isAck) noexcept
    {
        // When we receive an acknowledgement, we can't match the property
        if (isAck)
            return !m_waitingForAck;

        if (m_waitingForAck)
            return true;

        if (m_property != property)
            return true;

        return false;
    }

    template ZenError SyncedModbusCommunicator::publishArray(ZenProperty_t, ZenError, gsl::span<const std::byte>) noexcept;
    template ZenError SyncedModbusCommunicator::publishArray(ZenProperty_t, ZenError, gsl::span<const bool>) noexcept;
    template ZenError SyncedModbusCommunicator::publishArray(ZenProperty_t, ZenError, gsl::span<const float>) noexcept;
    template ZenError SyncedModbusCommunicator::publishArray(ZenProperty_t, ZenError, gsl::span<const int32_t>) noexcept;
    template ZenError SyncedModbusCommunicator::publishArray(ZenProperty_t, ZenError, gsl::span<const uint64_t>) noexcept;
    // [TODO] Remove after we have removed backwards compatability with version 0
    template ZenError SyncedModbusCommunicator::publishArray(ZenProperty_t, ZenError, gsl::span<const uint32_t>) noexcept;

    template ZenError SyncedModbusCommunicator::publishResult(ZenProperty_t, ZenError, bool) noexcept;
    template ZenError SyncedModbusCommunicator::publishResult(ZenProperty_t, ZenError, float) noexcept;
    template ZenError SyncedModbusCommunicator::publishResult(ZenProperty_t, ZenError, int32_t) noexcept;
    template ZenError SyncedModbusCommunicator::publishResult(ZenProperty_t, ZenError, uint64_t) noexcept;
    // [TODO] Remove after we have removed backwards compatability with version 0
    template ZenError SyncedModbusCommunicator::publishResult(ZenProperty_t, ZenError, uint32_t) noexcept;

    template std::pair<ZenError, size_t> SyncedModbusCommunicator::sendAndWaitForArray(uint8_t, uint8_t, ZenProperty_t, gsl::span<const std::byte>, gsl::span<std::byte>) noexcept;
    template std::pair<ZenError, size_t> SyncedModbusCommunicator::sendAndWaitForArray(uint8_t, uint8_t, ZenProperty_t, gsl::span<const std::byte>, gsl::span<bool>) noexcept;
    template std::pair<ZenError, size_t> SyncedModbusCommunicator::sendAndWaitForArray(uint8_t, uint8_t, ZenProperty_t, gsl::span<const std::byte>, gsl::span<float>) noexcept;
    template std::pair<ZenError, size_t> SyncedModbusCommunicator::sendAndWaitForArray(uint8_t, uint8_t, ZenProperty_t, gsl::span<const std::byte>, gsl::span<int32_t>) noexcept;
    template std::pair<ZenError, size_t> SyncedModbusCommunicator::sendAndWaitForArray(uint8_t, uint8_t, ZenProperty_t, gsl::span<const std::byte>, gsl::span<uint64_t>) noexcept;
    // [TODO] Remove after we have removed backwards compatability with version 0
    template std::pair<ZenError, size_t> SyncedModbusCommunicator::sendAndWaitForArray(uint8_t, uint8_t, ZenProperty_t, gsl::span<const std::byte>, gsl::span<uint32_t>) noexcept;

    template nonstd::expected<bool, ZenError> SyncedModbusCommunicator::sendAndWaitForResult(uint8_t, uint8_t, ZenProperty_t, gsl::span<const std::byte>) noexcept;
    template nonstd::expected<float, ZenError> SyncedModbusCommunicator::sendAndWaitForResult(uint8_t, uint8_t, ZenProperty_t, gsl::span<const std::byte>) noexcept;
    template nonstd::expected<int32_t, ZenError> SyncedModbusCommunicator::sendAndWaitForResult(uint8_t, uint8_t, ZenProperty_t, gsl::span<const std::byte>) noexcept;
    template nonstd::expected<uint64_t, ZenError> SyncedModbusCommunicator::sendAndWaitForResult(uint8_t, uint8_t, ZenProperty_t, gsl::span<const std::byte>) noexcept;
    // [TODO] Remove after we have removed backwards compatability with version 0
    template nonstd::expected<uint32_t, ZenError> SyncedModbusCommunicator::sendAndWaitForResult(uint8_t, uint8_t, ZenProperty_t, gsl::span<const std::byte>) noexcept;
}
