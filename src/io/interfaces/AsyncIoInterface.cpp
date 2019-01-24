#include "io/interfaces/AsyncIoInterface.h"

#include <cstring>

#include "utility/Finally.h"

namespace zen
{
    AsyncIoInterface::AsyncIoInterface(std::unique_ptr<BaseIoInterface> ioInterface)
        : m_ioInterface(std::move(ioInterface))
        , m_waiting{ 0 }
        , m_publishing{ 0 }
    {}

    ZenError AsyncIoInterface::sendAndWaitForAck(uint8_t address, uint8_t function, ZenProperty_t property, gsl::span<const unsigned char> data)
    {
        if (auto error = tryToWait(property, true))
            return error;

        auto guard = finally([this]() {
            m_waiting.clear();
        });

        if (auto error = m_ioInterface->send(address, function, data.data(), data.size()))
            return error;

        return terminateWaitOnPublishOrTimeout();
    }

    template <typename T>
    ZenError AsyncIoInterface::sendAndWaitForArray(uint8_t address, uint8_t function, ZenProperty_t property, gsl::span<const unsigned char> data, T* outArray, size_t& outLength)
    {
        if (auto error = tryToWait(property, false))
            return error;

        m_resultPtr = outArray;
        m_resultSizePtr = &outLength;

        auto guard = finally([this]() {
            m_resultPtr = nullptr;
            m_resultSizePtr = nullptr;
            m_waiting.clear();
        });

        if (auto error = m_ioInterface->send(address, function, data.data(), data.size()))
            return error;

        return terminateWaitOnPublishOrTimeout();
    }

    template <typename T>
    ZenError AsyncIoInterface::sendAndWaitForResult(uint8_t address, uint8_t function, ZenProperty_t property, gsl::span<const unsigned char> data, T& outValue)
    {
        if (auto error = tryToWait(property, false))
            return error;

        m_resultPtr = &outValue;

        auto guard = finally([this]() {
            m_resultPtr = nullptr;
            m_waiting.clear();
        });

        if (auto error = m_ioInterface->send(address, function, data.data(), data.size()))
            return error;

        return terminateWaitOnPublishOrTimeout();
    }

    ZenError AsyncIoInterface::publishAck(ZenProperty_t property, ZenError error)
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
    ZenError AsyncIoInterface::publishArray(ZenProperty_t property, ZenError error, const T* array, size_t length)
    {
        if (!prepareForPublishing())
            return ZenError_None;

        auto guard = finally([this]() {
            m_publishing.clear();
        });

        if (corruptMessage(property, false))
            return m_resultError = error;

        const auto bufferLength = *m_resultSizePtr;
        *m_resultSizePtr = length;

        auto guard2 = finally([this]() {
            m_fence.terminate();
        });

        if (m_resultPtr == nullptr)
            return m_resultError = ZenError_IsNull;

        if (length > bufferLength)
            return m_resultError = ZenError_BufferTooSmall;

        std::memcpy(m_resultPtr, array, sizeof(T) * length);
        return m_resultError = ZenError_None;
    }

    template <typename T>
    ZenError AsyncIoInterface::publishResult(ZenProperty_t property, ZenError error, const T& result)
    {
        if (!prepareForPublishing())
            return ZenError_None;

        auto guard = finally([this]() {
            m_publishing.clear();
        });

        if (corruptMessage(property, false))
            return m_resultError = error;

        *reinterpret_cast<T*>(m_resultPtr) = result;
        m_fence.terminate();
        return ZenError_None;
    }

    ZenError AsyncIoInterface::tryToWait(ZenProperty_t property, bool forAck)
    {
        if (m_waiting.test_and_set())
            return ZenError_Io_Busy;

        m_waitingForAck = forAck;
        m_property = property;
        m_resultError = ZenError_None;

        return ZenError_None;
    }

    ZenError AsyncIoInterface::terminateWaitOnPublishOrTimeout()
    {
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

    bool AsyncIoInterface::prepareForPublishing()
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

    bool AsyncIoInterface::corruptMessage(ZenProperty_t property, bool isAck)
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

    template ZenError AsyncIoInterface::publishArray(ZenProperty_t, ZenError, const bool*, size_t);
    template ZenError AsyncIoInterface::publishArray(ZenProperty_t, ZenError, const char*, size_t);
    template ZenError AsyncIoInterface::publishArray(ZenProperty_t, ZenError, const unsigned char*, size_t);
    template ZenError AsyncIoInterface::publishArray(ZenProperty_t, ZenError, const float*, size_t);
    template ZenError AsyncIoInterface::publishArray(ZenProperty_t, ZenError, const int32_t*, size_t);
    template ZenError AsyncIoInterface::publishArray(ZenProperty_t, ZenError, const uint64_t*, size_t);
    // [TODO] Remove after we have removed backwards compatability with version 0
    template ZenError AsyncIoInterface::publishArray(ZenProperty_t, ZenError, const uint32_t*, size_t);

    template ZenError AsyncIoInterface::publishResult(ZenProperty_t, ZenError, const char&);
    template ZenError AsyncIoInterface::publishResult(ZenProperty_t, ZenError, const unsigned char&);
    template ZenError AsyncIoInterface::publishResult(ZenProperty_t, ZenError, const bool&);
    template ZenError AsyncIoInterface::publishResult(ZenProperty_t, ZenError, const float&);
    template ZenError AsyncIoInterface::publishResult(ZenProperty_t, ZenError, const int32_t&);
    template ZenError AsyncIoInterface::publishResult(ZenProperty_t, ZenError, const uint64_t&);
    // [TODO] Remove after we have removed backwards compatability with version 0
    template ZenError AsyncIoInterface::publishResult(ZenProperty_t, ZenError, const uint32_t&);

    template ZenError AsyncIoInterface::sendAndWaitForArray(uint8_t, uint8_t, ZenProperty_t, gsl::span<const unsigned char>, bool*, size_t&);
    template ZenError AsyncIoInterface::sendAndWaitForArray(uint8_t, uint8_t, ZenProperty_t, gsl::span<const unsigned char>, char*, size_t&);
    template ZenError AsyncIoInterface::sendAndWaitForArray(uint8_t, uint8_t, ZenProperty_t, gsl::span<const unsigned char>, unsigned char*, size_t&);
    template ZenError AsyncIoInterface::sendAndWaitForArray(uint8_t, uint8_t, ZenProperty_t, gsl::span<const unsigned char>, float*, size_t&);
    template ZenError AsyncIoInterface::sendAndWaitForArray(uint8_t, uint8_t, ZenProperty_t, gsl::span<const unsigned char>, int32_t*, size_t&);
    template ZenError AsyncIoInterface::sendAndWaitForArray(uint8_t, uint8_t, ZenProperty_t, gsl::span<const unsigned char>, uint64_t*, size_t&);
    // [TODO] Remove after we have removed backwards compatability with version 0
    template ZenError AsyncIoInterface::sendAndWaitForArray(uint8_t, uint8_t, ZenProperty_t, gsl::span<const unsigned char>, uint32_t*, size_t&);

    template ZenError AsyncIoInterface::sendAndWaitForResult(uint8_t, uint8_t, ZenProperty_t, gsl::span<const unsigned char>, bool&);
    template ZenError AsyncIoInterface::sendAndWaitForResult(uint8_t, uint8_t, ZenProperty_t, gsl::span<const unsigned char>, char&);
    template ZenError AsyncIoInterface::sendAndWaitForResult(uint8_t, uint8_t, ZenProperty_t, gsl::span<const unsigned char>, unsigned char&);
    template ZenError AsyncIoInterface::sendAndWaitForResult(uint8_t, uint8_t, ZenProperty_t, gsl::span<const unsigned char>, float&);
    template ZenError AsyncIoInterface::sendAndWaitForResult(uint8_t, uint8_t, ZenProperty_t, gsl::span<const unsigned char>, int32_t&);
    template ZenError AsyncIoInterface::sendAndWaitForResult(uint8_t, uint8_t, ZenProperty_t, gsl::span<const unsigned char>, uint64_t&);
    // [TODO] Remove after we have removed backwards compatability with version 0
    template ZenError AsyncIoInterface::sendAndWaitForResult(uint8_t, uint8_t, ZenProperty_t, gsl::span<const unsigned char>, uint32_t&);
}
