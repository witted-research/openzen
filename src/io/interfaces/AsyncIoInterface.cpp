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

    ZenError AsyncIoInterface::sendAndWaitForAck(uint8_t function, Property_t property, const unsigned char* data, size_t length)
    {
        if (auto error = tryToWait(function, property, true))
            return error;

        bool success;
        m_resultPtr = &success;

        auto guard = finally([this]() {
            m_resultPtr = nullptr;
            m_waiting.clear();
        });

        if (auto error = m_ioInterface->send(0, function, data, length))
            return error;

        if (auto error = terminateWaitOnPublishOrTimeout())
            return error;

        if (!success)
            return ZenError_FW_FunctionFailed;

        return ZenError_None;
    }

    template <typename T>
    ZenError AsyncIoInterface::requestAndWaitForArray(uint8_t function, Property_t property, T* outArray, size_t& outLength)
    {
        if (outArray == nullptr)
            return ZenError_IsNull;

        if (auto error = tryToWait(function, property, false))
            return error;

        const auto bufferLength = outLength;

        m_resultPtr = outArray;
        m_resultSizePtr = &outLength;

        // No need to be waiting while validating the result
        {
            auto guard = finally([this]() {
                m_resultPtr = nullptr;
                m_resultSizePtr = nullptr;
                m_waiting.clear();
            });

            if (auto error = m_ioInterface->send(0, function, nullptr, 0))
                return error;

            if (auto error = terminateWaitOnPublishOrTimeout())
                return error;
        }

        if (bufferLength < outLength)
            return ZenError_BufferTooSmall;

        return ZenError_None;
    }

    template <typename T>
    ZenError AsyncIoInterface::requestAndWaitForResult(uint8_t function, Property_t property, T& outValue)
    {
        if (auto error = tryToWait(function, property, false))
            return error;

        m_resultPtr = &outValue;

        auto guard = finally([this]() {
            m_resultPtr = nullptr;
            m_waiting.clear();
        });

        if (auto error = m_ioInterface->send(0, function, nullptr, 0))
            return error;

        return terminateWaitOnPublishOrTimeout();
    }

    ZenError AsyncIoInterface::publishAck(uint8_t function, Property_t property, bool b)
    {
        if (!prepareForPublishing())
            return ZenError_None;

        auto guard = finally([this]() {
            m_publishing.clear();
        });

        if (corruptMessage(function, property, true))
        {
            m_resultError = ZenError_Io_UnexpectedFunction;
            return ZenError_Io_UnexpectedFunction;
        }

        *reinterpret_cast<bool*>(m_resultPtr) = b;
        m_fence.terminate();
        return ZenError_None;
    }

    template <typename T>
    ZenError AsyncIoInterface::publishArray(uint8_t function, Property_t property, const T* array, size_t length)
    {
        if (!prepareForPublishing())
            return ZenError_None;

        auto guard = finally([this]() {
            m_publishing.clear();
        });

        if (corruptMessage(function, property, false))
        {
            m_resultError = ZenError_Io_UnexpectedFunction;
            return ZenError_Io_UnexpectedFunction;
        }

        if (*m_resultSizePtr >= length)
            std::memcpy(m_resultPtr, array, sizeof(T) * length);

        *m_resultSizePtr = length;
        m_fence.terminate();
        return ZenError_None;
    }

    template <typename T>
    ZenError AsyncIoInterface::publishResult(uint8_t function, Property_t property, const T& result)
    {
        if (!prepareForPublishing())
            return ZenError_None;

        auto guard = finally([this]() {
            m_publishing.clear();
        });

        if (corruptMessage(function, property, false))
        {
            m_resultError = ZenError_Io_UnexpectedFunction;
            return ZenError_Io_UnexpectedFunction;
        }

        *reinterpret_cast<T*>(m_resultPtr) = result;
        m_fence.terminate();
        return ZenError_None;
    }

    ZenError AsyncIoInterface::tryToWait(uint8_t function, Property_t property, bool forAck)
    {
        if (m_waiting.test_and_set())
            return ZenError_Io_Busy;

        m_waitingForAck = forAck;
        m_function = function,
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
        m_fence.reset();
        return ZenError_None;
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

    bool AsyncIoInterface::corruptMessage(uint8_t function, Property_t property, bool isAck)
    {
        // When we receive an acknowledgement, we can't match the property
        if (isAck)
            return !m_waitingForAck;

        if (m_waitingForAck)
            return true;

        // [XXX] Mapping of the function pairs should happen outside of the publish call (for backwards compatability)
        if (m_function != function)
            return true;

        if (m_property != property)
            return true;

        return false;
    }

    template ZenError AsyncIoInterface::publishArray(uint8_t function, Property_t, const bool*, size_t);
    template ZenError AsyncIoInterface::publishArray(uint8_t function, Property_t, const char*, size_t);
    template ZenError AsyncIoInterface::publishArray(uint8_t function, Property_t, const unsigned char*, size_t);
    template ZenError AsyncIoInterface::publishArray(uint8_t function, Property_t, const float*, size_t);
    template ZenError AsyncIoInterface::publishArray(uint8_t function, Property_t, const uint32_t*, size_t);

    template ZenError AsyncIoInterface::publishResult(uint8_t function, Property_t, const bool&);
    template ZenError AsyncIoInterface::publishResult(uint8_t function, Property_t, const char&);
    template ZenError AsyncIoInterface::publishResult(uint8_t function, Property_t, const unsigned char&);
    template ZenError AsyncIoInterface::publishResult(uint8_t function, Property_t, const float&);
    template ZenError AsyncIoInterface::publishResult(uint8_t function, Property_t, const uint32_t&);

    template ZenError AsyncIoInterface::requestAndWaitForArray(uint8_t function, Property_t, bool*, size_t&);
    template ZenError AsyncIoInterface::requestAndWaitForArray(uint8_t function, Property_t, char*, size_t&);
    template ZenError AsyncIoInterface::requestAndWaitForArray(uint8_t function, Property_t, unsigned char*, size_t&);
    template ZenError AsyncIoInterface::requestAndWaitForArray(uint8_t function, Property_t, float*, size_t&);
    template ZenError AsyncIoInterface::requestAndWaitForArray(uint8_t function, Property_t, uint32_t*, size_t&);
    template ZenError AsyncIoInterface::requestAndWaitForArray(uint8_t function, Property_t, int32_t*, size_t&);

    template ZenError AsyncIoInterface::requestAndWaitForResult(uint8_t function, Property_t, bool&);
    template ZenError AsyncIoInterface::requestAndWaitForResult(uint8_t function, Property_t, char&);
    template ZenError AsyncIoInterface::requestAndWaitForResult(uint8_t function, Property_t, unsigned char&);
    template ZenError AsyncIoInterface::requestAndWaitForResult(uint8_t function, Property_t, float&);
    template ZenError AsyncIoInterface::requestAndWaitForResult(uint8_t function, Property_t, uint32_t&);
}
