#ifndef ZEN_IO_INTERFACES_ASYNCIOINTERFACE_H_
#define ZEN_IO_INTERFACES_ASYNCIOINTERFACE_H_

#include <atomic>
#include <memory>

#include "io/interfaces/BaseIoInterface.h"
#include "utility/ThreadFence.h"

namespace zen
{
    using Property_t = uint32_t;

    class AsyncIoInterface
    {
        constexpr static auto IO_TIMEOUT = std::chrono::milliseconds(15000);

    public:
        AsyncIoInterface(std::unique_ptr<BaseIoInterface> ioInterface);

        /** Poll data from IO interface */
        ZenError poll() { return m_ioInterface->poll(); }

        /** Send data to IO interface */
        ZenError send(uint8_t address, uint8_t function, const unsigned char* data, size_t length) { return m_ioInterface->send(address, function, data, length); }

        /** Returns the IO interface's baudrate (bit/s) */
        ZenError baudrate(int32_t& rate) const { return m_ioInterface->baudrate(rate); }

        /** Set Baudrate of IO interface (bit/s) */
        ZenError setBaudrate(unsigned int rate) { return m_ioInterface->setBaudrate(rate); }

        /** Returns the supported baudrates of the IO interface (bit/s) */
        ZenError supportedBaudrates(std::vector<int32_t>& outBaudrates) const { return m_ioInterface->supportedBaudrates(outBaudrates); }

        /** Returns whether the IO interface equals the sensor description */
        bool equals(const ZenSensorDesc& desc) const { return m_ioInterface->equals(desc); }

        ZenError sendAndWaitForAck(uint8_t function, Property_t property, const unsigned char* data, size_t length);

        template <typename T>
        ZenError requestAndWaitForArray(uint8_t function, Property_t property, T* outArray, size_t& outLength);

        template <typename T>
        ZenError requestAndWaitForResult(uint8_t function, Property_t property, T& outValue);

        ZenError publishAck(uint8_t function, Property_t property, bool b);

        template <typename T>
        ZenError publishArray(uint8_t function, Property_t property, const T* array, size_t length);

        template <typename T>
        ZenError publishResult(uint8_t function, Property_t property, const T& result);

    private:
        ZenError terminateWaitOnPublishOrTimeout();

        ZenError tryToWait(uint8_t function, Property_t property, bool forAck);

        bool prepareForPublishing();

        bool corruptMessage(uint8_t function, Property_t property, bool isAck);

        std::unique_ptr<BaseIoInterface> m_ioInterface;

        ThreadFence m_fence;
        std::atomic_flag m_waiting;
        std::atomic_flag m_publishing;

        ZenError m_resultError;
        Property_t m_property;
        bool m_waitingForAck;  // If not, waiting for data
        uint8_t m_function;

        void* m_resultPtr;
        size_t* m_resultSizePtr;
    };
}

#endif
