#include "sensors/BaseSensor.h"

#include <algorithm>
#include <cstring>
#include <string>

#include "io/IIoInterface.h"
#include "properties/BaseSensorPropertiesV0.h"
#include "utility/Finally.h"

namespace zen
{
    namespace
    {
        constexpr size_t sizeOfPropertyType(ZenPropertyType type)
        {
            switch (type)
            {
            case ZenPropertyType_Byte:
                return sizeof(unsigned char);

            case ZenPropertyType_Float:
                return sizeof(float);

            case ZenPropertyType_Int32:
                return sizeof(int32_t);

            default:
                return 0;
            }
        }
    }

    BaseSensor::BaseSensor(std::unique_ptr<BaseIoInterface> ioInterface)
        : IIoDataSubscriber(*ioInterface.get())
        , m_ioInterface(std::move(ioInterface))
        , m_waiting{0}
        , m_publishing{0}
        , m_updatingFirmware(false)
        , m_updatedFirmware(false)
        , m_updatingIAP(false)
        , m_updatedIAP(false)
        , m_version(0)
    {}

    BaseSensor::~BaseSensor()
    {
        // We need to wait for the firmware/IAP upload to stop
        if (m_uploadThread.joinable())
            m_uploadThread.join();
    }

    ZenError BaseSensor::init()
    {
        m_samplingRate = 200;
        return initExtension();
    }

    ZenError BaseSensor::poll()
    {
        return m_ioInterface->poll();
    }

    ZenAsyncStatus BaseSensor::updateFirmwareAsync(const char* const buffer, size_t bufferSize)
    {
        if (buffer == nullptr)
            return ZenAsync_InvalidArgument;

        if (m_updatingFirmware.exchange(true))
        {
            if (m_updatedFirmware.exchange(false))
            {
                m_uploadThread.join();

                const bool error = m_updateFirmwareError != ZenError_None;
                m_updatingFirmware = false;
                return error ? ZenAsync_Failed : ZenAsync_Finished;
            }

            return ZenAsync_Updating;
        }

        if (m_updatingIAP)
        {
            m_updatingFirmware = false;
            return ZenAsync_ThreadBusy;
        }

        std::vector<unsigned char> firmware(bufferSize);
        std::memcpy(firmware.data(), buffer, bufferSize);

        m_uploadThread = std::thread(&BaseSensor::upload, this, std::move(firmware));

        return ZenAsync_Updating;
    }

    ZenAsyncStatus BaseSensor::updateIAPAsync(const char* const buffer, size_t bufferSize)
    {
        if (buffer == nullptr)
            return ZenAsync_InvalidArgument;

        if (m_updatingIAP.exchange(true))
        {
            if (m_updatedIAP.exchange(false))
            {
                m_uploadThread.join();

                const bool error = m_updateIAPError != ZenError_None;
                m_updatingIAP = false;
                return error ? ZenAsync_Failed : ZenAsync_Finished;
            }

            return ZenAsync_Updating;
        }

        if (m_updatingFirmware)
        {
            m_updatingIAP = false;
            return ZenAsync_ThreadBusy;
        }

        std::vector<unsigned char> iap(bufferSize);
        std::memcpy(iap.data(), buffer, bufferSize);

        m_uploadThread = std::thread(&BaseSensor::upload, this, std::move(iap));

        return ZenAsync_Updating;
    }

    ZenError BaseSensor::executeDeviceCommand(ZenCommand_t command)
    {
        switch (m_version)
        {
        case 0:
        {
            const auto commandV0 = base::v0::mapCommand(command);
            if (base::v0::supportsExecutingDeviceCommand(commandV0))
                return sendAndWaitForAck(static_cast<DeviceProperty_t>(commandV0), nullptr, 0);
            else
                break;
        }

        default:
            return ZenError_Unknown;
        }

        return executeExtensionDeviceCommand(command);
    }

    ZenError BaseSensor::getArrayDeviceProperty(ZenProperty_t property, ZenPropertyType type, void* const buffer, size_t* const bufferSize)
    {
        if (bufferSize == nullptr)
            return ZenError_IsNull;

        ZenPropertyType expectedType;
        DeviceProperty_t deviceProperty;
        switch (m_version)
        {
        case 0:
        {
            const auto propertyV0 = base::v0::map(property, true);
            expectedType = base::v0::supportsGettingArrayDeviceProperty(propertyV0);
            if (property == ZenSensorProperty_SupportedSamplingRates)
                expectedType = ZenPropertyType_Int32;

            deviceProperty = static_cast<DeviceProperty_t>(propertyV0);
            break;
        }

        default:
            return ZenError_Unknown;
        }

        if (property == ZenSensorProperty_SupportedBaudRates)
            expectedType = ZenPropertyType_Int32;

        if (!expectedType)
            return getExtensionArrayDeviceProperty(property, type, buffer, *bufferSize);

        if (type != expectedType)
            return ZenError_WrongDataType;

        switch (type)
        {
        case ZenPropertyType_Byte:
            return requestAndWaitForArray<unsigned char>(deviceProperty, reinterpret_cast<unsigned char*>(buffer), *bufferSize);

        case ZenPropertyType_Float:
            return requestAndWaitForArray<float>(deviceProperty, reinterpret_cast<float*>(buffer), *bufferSize);

        case ZenPropertyType_Int32:
            if (property == ZenSensorProperty_SupportedBaudRates)
                return supportedBaudRates(buffer, *bufferSize);
            else if (property == ZenSensorProperty_SupportedSamplingRates)
                return base::v0::supportedSamplingRates(reinterpret_cast<int32_t* const>(buffer), *bufferSize);
            else if (m_version == 0)
            {
                // As the communication protocol only supports uint32_t for getting arrays, we need to cast all values to guarantee the correct sign
                // This only applies to BaseSensor, as IMUSensor has no integer array properties
                uint32_t* uiBuffer = reinterpret_cast<uint32_t*>(buffer);
                if (auto error = requestAndWaitForArray<uint32_t>(deviceProperty, uiBuffer, *bufferSize))
                    return error;

                int32_t* iBuffer = reinterpret_cast<int32_t*>(buffer);
                for (size_t idx = 0; idx < *bufferSize; ++idx)
                    iBuffer[idx] = static_cast<int32_t>(uiBuffer[idx]);

                // Some properties need to be reversed
                const bool reverse = property == ZenSensorProperty_FirmwareVersion;
                if (reverse)
                    std::reverse(iBuffer, iBuffer + *bufferSize);

                return ZenError_None;
            }
            else
                return requestAndWaitForArray<int32_t>(deviceProperty, reinterpret_cast<int32_t*>(buffer), *bufferSize);

        default:
            return ZenError_WrongDataType;
        }
    }

    ZenError BaseSensor::getBoolDeviceProperty(ZenProperty_t property, bool* const outValue)
    {
        if (outValue == nullptr)
            return ZenError_IsNull;

        switch (m_version)
        {
        case 0:
        {
            const auto propertyV0 = base::v0::map(property, true);
            if (propertyV0 == EDevicePropertyV0::GetBatteryCharging)
            {
                uint32_t temp;
                if (auto error = requestAndWaitForResult<uint32_t>(static_cast<DeviceProperty_t>(propertyV0), temp))
                    return error;

                *outValue = temp != 0;
                return ZenError_None;
            }
            else if (base::v0::supportsGettingBoolDeviceProperty(propertyV0))
                return requestAndWaitForResult<bool>(static_cast<DeviceProperty_t>(propertyV0), *outValue);
            else
                break;
        }

        default:
            return ZenError_Unknown;
        }

        return getExtensionBoolDeviceProperty(property, *outValue);
        
    }

    ZenError BaseSensor::getFloatDeviceProperty(ZenProperty_t property, float* const outValue)
    {
        if (outValue == nullptr)
            return ZenError_IsNull;

        switch (m_version)
        {
        case 0:
        {
            const auto propertyV0 = base::v0::map(property, true);
            if (base::v0::supportsGettingFloatDeviceProperty(propertyV0))
                return requestAndWaitForResult<float>(static_cast<DeviceProperty_t>(propertyV0), *outValue);
            else
                break;
        }

        default:
            return ZenError_Unknown;
        }

        return getExtensionFloatDeviceProperty(property, *outValue);
    }

    ZenError BaseSensor::getInt32DeviceProperty(ZenProperty_t property, int32_t* const outValue)
    {
        if (outValue == nullptr)
            return ZenError_IsNull;

        switch (m_version)
        {
        case 0:
            if (property == ZenSensorProperty_BaudRate)
                return m_ioInterface->baudrate(*outValue);
            else if (property == ZenSensorProperty_SamplingRate)
            {
                *outValue = m_samplingRate;
                return ZenError_None;
            }
            else
            {
                const auto propertyV0 = base::v0::map(property, true);
                if (base::v0::supportsGettingInt32DeviceProperty(propertyV0))
                {
                    // Communication protocol only supports uint32_t
                    uint32_t uiValue;
                    if (auto error = requestAndWaitForResult<uint32_t>(static_cast<DeviceProperty_t>(propertyV0), uiValue))
                        return error;

                    *outValue = static_cast<int32_t>(uiValue);
                    return ZenError_None;
                }
                else
                    break;
            }

        default:
            return ZenError_UnknownProperty;
        }

        return getExtensionInt32DeviceProperty(property, *outValue);
    }

    ZenError BaseSensor::getMatrix33DeviceProperty(ZenProperty_t property, ZenMatrix3x3f* const outValue)
    {
        if (outValue == nullptr)
            return ZenError_IsNull;

        size_t length = 9;
        switch (m_version)
        {
        case 0:
        {
            const auto propertyV0 = base::v0::map(property, true);
            if (base::v0::supportsGettingMatrix33DeviceProperty(propertyV0))
                return requestAndWaitForArray<float>(static_cast<DeviceProperty_t>(propertyV0), outValue->data, length);
            else
                break;
        }

        default:
            return ZenError_Unknown;
        }

        return getExtensionMatrix33DeviceProperty(property, *outValue);
    }

    ZenError BaseSensor::getStringDeviceProperty(ZenProperty_t property, char* const buffer, size_t* const bufferSize)
    {
        if (bufferSize == nullptr)
            return ZenError_IsNull;

        switch (m_version)
        {
        case 0:
        {
            const auto propertyV0 = base::v0::map(property, true);
            if (base::v0::supportsGettingStringDeviceProperty(propertyV0))
                return requestAndWaitForArray<char>(static_cast<DeviceProperty_t>(propertyV0), buffer, *bufferSize);
            else
                break;
        }

        default:
            return ZenError_Unknown;
        }

        return getExtensionStringDeviceProperty(property, buffer, *bufferSize);
    }

    ZenError BaseSensor::setArrayDeviceProperty(ZenProperty_t property, ZenPropertyType type, const void* const buffer, size_t bufferSize)
    {
        if (buffer == nullptr)
            return ZenError_IsNull;

        ZenPropertyType expectedType;
        DeviceProperty_t deviceProperty;
        switch (m_version)
        {
        case 0:
        {
            const auto propertyV0 = base::v0::map(property, false);
            expectedType = base::v0::supportsSettingArrayDeviceProperty(propertyV0);
            deviceProperty = static_cast<DeviceProperty_t>(propertyV0);
            break;
        }
            break;

        default:
            return ZenError_Unknown;
        }

        if (!expectedType)
            return setExtensionArrayDeviceProperty(property, type, buffer, bufferSize);

        if (type != expectedType)
            return ZenError_WrongDataType;

        const size_t typeSize = sizeOfPropertyType(type);
        return sendAndWaitForAck(deviceProperty, reinterpret_cast<const unsigned char*>(buffer), typeSize * bufferSize);
    }

    ZenError BaseSensor::setBoolDeviceProperty(ZenProperty_t property, bool value)
    {
        switch (m_version)
        {
        case 0:
        {
            const auto propertyV0 = base::v0::map(property, false);
            if (base::v0::supportsSettingBoolDeviceProperty(propertyV0))
                return sendAndWaitForAck(static_cast<DeviceProperty_t>(propertyV0), reinterpret_cast<unsigned char*>(&value), sizeof(value));
            else
                break;
        }

        default:
            return ZenError_Unknown;
        }

        return setExtensionBoolDeviceProperty(property, value);
    }

    ZenError BaseSensor::setFloatDeviceProperty(ZenProperty_t property, float value)
    {
        switch (m_version)
        {
        case 0:
        {
            const auto propertyV0 = base::v0::map(property, false);
            if (base::v0::supportsSettingFloatDeviceProperty(propertyV0))
                return sendAndWaitForAck(static_cast<DeviceProperty_t>(propertyV0), reinterpret_cast<unsigned char*>(&value), sizeof(value));
            else
                break;
        }

        default:
            return ZenError_Unknown;
        }

        return setExtensionFloatDeviceProperty(property, value);
    }

    ZenError BaseSensor::setInt32DeviceProperty(ZenProperty_t property, int32_t value)
    {
        switch (m_version)
        {
        case 0:
            if (property == ZenSensorProperty_BaudRate)
                return m_ioInterface->setBaudrate(value);
            else
            {
                const auto propertyV0 = base::v0::map(property, false);
                if (propertyV0 == EDevicePropertyV0::SetSamplingRate)
                {
                    const uint32_t uiValue = base::v0::roundSamplingRate(value);
                    if (auto error = sendAndWaitForAck(static_cast<DeviceProperty_t>(propertyV0), reinterpret_cast<const unsigned char*>(&uiValue), sizeof(uiValue)))
                        return error;
                    m_samplingRate = value;
                }
                else if (base::v0::supportsSettingInt32DeviceProperty(propertyV0))
                {
                    // Communication protocol only supports uint32_t
                    const uint32_t uiValue = static_cast<uint32_t>(value);
                    return sendAndWaitForAck(static_cast<DeviceProperty_t>(propertyV0), reinterpret_cast<const unsigned char*>(&uiValue), sizeof(uiValue));
                }
                else
                    break;
            }

        default:
            return ZenError_Unknown;
        }

        return setExtensionInt32DeviceProperty(property, value);
    }

    ZenError BaseSensor::setMatrix33DeviceProperty(ZenProperty_t property, const ZenMatrix3x3f* const value)
    {
        if (value == nullptr)
            return ZenError_IsNull;

        switch (m_version)
        {
        case 0:
        {
            const auto propertyV0 = base::v0::map(property, false);
            if (base::v0::supportsSettingMatrix33DeviceProperty(propertyV0))
                return sendAndWaitForAck(static_cast<DeviceProperty_t>(propertyV0), reinterpret_cast<const unsigned char*>(value->data), 9 * sizeof(float));
            else
                break;
        }

        default:
            return ZenError_Unknown;
        }

        return setExtensionMatrix33DeviceProperty(property, *value);
    }

    ZenError BaseSensor::setStringDeviceProperty(ZenProperty_t property, const char* buffer, size_t bufferSize)
    {
        if (buffer == nullptr)
            return ZenError_IsNull;

        switch (m_version)
        {
        case 0:
        {
            const auto propertyV0 = base::v0::map(property, false);
            if (base::v0::supportsSettingStringDeviceProperty(propertyV0))
                return sendAndWaitForAck(static_cast<DeviceProperty_t>(propertyV0), reinterpret_cast<const unsigned char*>(buffer), bufferSize);
            else
                break;
        }

        default:
            return ZenError_Unknown;
        }

        return setExtensionStringDeviceProperty(property, buffer, bufferSize);
    }

    bool BaseSensor::equals(const ZenSensorDesc* desc) const
    {
        if (type() != desc->sensorType)
            return false;

        return m_ioInterface->equals(*desc);
    }

    ZenError BaseSensor::sendAndWaitForAck(DeviceProperty_t property, const unsigned char* data, size_t length)
    {
        if (auto error = tryToWait(property, true))
            return error;

        bool success;
        m_resultPtr = &success;

        auto guard = finally([this]() {
            m_resultPtr = nullptr;
            m_waiting.clear();
        });

        if (auto error = m_ioInterface->send(0, property, data, length))
            return error;

        if (auto error = terminateWaitOnPublishOrTimeout())
            return error;

        if (!success)
            return ZenError_FW_FunctionFailed;

        return ZenError_None;
    }

    template <typename T>
    ZenError BaseSensor::requestAndWaitForArray(DeviceProperty_t property, T* outArray, size_t& outLength)
    {
        if (outArray == nullptr)
            return ZenError_IsNull;

        if (auto error = tryToWait(property, false))
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

            if (auto error = m_ioInterface->send(0, property, nullptr, 0))
                return error;

            if (auto error = terminateWaitOnPublishOrTimeout())
                return error;
        }

        if (bufferLength < outLength)
            return ZenError_BufferTooSmall;

        return ZenError_None;
    }

    template <typename T>
    ZenError BaseSensor::requestAndWaitForResult(DeviceProperty_t property, T& outValue)
    {
        if (auto error = tryToWait(property, false))
            return error;

        m_resultPtr = &outValue;

        auto guard = finally([this]() {
            m_resultPtr = nullptr;
            m_waiting.clear();
        });

        if (auto error = m_ioInterface->send(0, property, nullptr, 0))
            return error;

        return terminateWaitOnPublishOrTimeout();
    }

    ZenError BaseSensor::publishAck(DeviceProperty_t property, bool b)
    {
        if (!prepareForPublishing())
            return ZenError_None;

        auto guard = finally([this]() {
            m_publishing.clear();
        });

        if (corruptMessage(property, true))
        {
            m_resultError = ZenError_Io_UnexpectedFunction;
            return ZenError_Io_UnexpectedFunction;
        }

        *reinterpret_cast<bool*>(m_resultPtr) = b;
        m_fence.terminate();
        return ZenError_None;
    }

    template <typename T>
    ZenError BaseSensor::publishArray(DeviceProperty_t property, const T* array, size_t length)
    {
        if (!prepareForPublishing())
            return ZenError_None;

        auto guard = finally([this]() {
            m_publishing.clear();
        });

        if (corruptMessage(property, false))
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
    ZenError BaseSensor::publishResult(DeviceProperty_t property, const T& result)
    {
        if (!prepareForPublishing())
            return ZenError_None;

        auto guard = finally([this]() {
            m_publishing.clear();
        });

        if (corruptMessage(property, false))
        {
            m_resultError = ZenError_Io_UnexpectedFunction;
            return ZenError_Io_UnexpectedFunction;
        }

        *reinterpret_cast<T*>(m_resultPtr) = result;
        m_fence.terminate();
        return ZenError_None;
    }

    ZenError BaseSensor::processData(uint8_t, uint8_t function, const unsigned char* data, size_t length)
    {
        if (auto optInternal = base::v0::internal::map(function))
        {
            switch (*optInternal)
            {
            case EDevicePropertyInternal::Ack:
                return publishAck(ZenSensorProperty_Invalid, true);

            case EDevicePropertyInternal::Nack:
                return publishAck(ZenSensorProperty_Invalid, false);

            default:
                throw std::invalid_argument(std::to_string(static_cast<DeviceProperty_t>(*optInternal)));
            }
        }
        else
        {
            const auto property = static_cast<EDevicePropertyV0>(function);
            switch (property)
            {
            case EDevicePropertyV0::GetBatteryCharging:
            case EDevicePropertyV0::GetPing:
                if (length != sizeof(uint32_t))
                    return ZenError_Io_MsgCorrupt;
                return publishResult<uint32_t>(function, *reinterpret_cast<const uint32_t*>(data));

            case EDevicePropertyV0::GetBatteryLevel:
            case EDevicePropertyV0::GetBatteryVoltage:
                if (length != sizeof(float))
                    return ZenError_Io_MsgCorrupt;
                return publishResult<float>(function, *reinterpret_cast<const float*>(data));

            case EDevicePropertyV0::GetSerialNumber:
            case EDevicePropertyV0::GetDeviceName:
            case EDevicePropertyV0::GetFirmwareInfo:
                return publishArray<char>(function, reinterpret_cast<const char*>(data), length);

            case EDevicePropertyV0::GetFirmwareVersion:
                if (length != sizeof(uint32_t) * 3)
                    return ZenError_Io_MsgCorrupt;
                return publishArray<uint32_t>(function, reinterpret_cast<const uint32_t*>(data), 3);

            default:
                return extensionProcessData(function, data, length);
            }
        }
    }

    ZenError BaseSensor::supportedBaudRates(void* buffer, size_t& bufferSize) const
    {
        std::vector<int32_t> baudrates;
        if (auto error = m_ioInterface->supportedBaudrates(baudrates))
            return error;

        if (bufferSize < baudrates.size())
        {
            bufferSize = baudrates.size();
            return ZenError_BufferTooSmall;
        }

        if (buffer == nullptr)
            return ZenError_IsNull;

        std::memcpy(buffer, baudrates.data(), baudrates.size() * sizeof(uint32_t));
        return ZenError_None;
    }

    ZenError BaseSensor::tryToWait(DeviceProperty_t property, bool forAck)
    {
        if (m_waiting.test_and_set())
            return ZenError_Io_Busy;

        m_waitingForAck = forAck;
        m_property = property;
        m_resultError = ZenError_None;

        return ZenError_None;
    }

    ZenError BaseSensor::terminateWaitOnPublishOrTimeout()
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

    bool BaseSensor::prepareForPublishing()
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

    bool BaseSensor::corruptMessage(DeviceProperty_t property, bool isAck)
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

    void BaseSensor::upload(std::vector<unsigned char> firmware)
    {
        constexpr uint32_t PAGE_SIZE = 256;

        auto& error = m_updatingFirmware ? m_updateFirmwareError : m_updateIAPError;
        auto& updated = m_updatingFirmware ? m_updatedFirmware : m_updatedIAP;
        const DeviceProperty_t property = static_cast<DeviceProperty_t>(m_updatingFirmware ? EDevicePropertyInternal::UpdateFirmware : EDevicePropertyInternal::UpdateIAP);

        auto guard = finally([&updated]() {
            updated = true;
        });

        const uint32_t nFullPages = static_cast<uint32_t>(firmware.size() / PAGE_SIZE);
        const uint32_t remainder = firmware.size() % PAGE_SIZE;
        const uint32_t nPages = remainder > 0 ? nFullPages + 1 : nFullPages;
        if (error = sendAndWaitForAck(property, reinterpret_cast<const unsigned char*>(&nPages), sizeof(uint32_t)))
            return;

        for (unsigned idx = 0; idx < nPages; ++idx)
            if (error = sendAndWaitForAck(property, firmware.data() + idx * PAGE_SIZE, PAGE_SIZE))
                return;

        if (remainder > 0)
            if (error = sendAndWaitForAck(property, firmware.data() + nFullPages * PAGE_SIZE, remainder))
                return;
    }

    template ZenError BaseSensor::publishArray(DeviceProperty_t, const bool*, size_t);
    template ZenError BaseSensor::publishArray(DeviceProperty_t, const char*, size_t);
    template ZenError BaseSensor::publishArray(DeviceProperty_t, const unsigned char*, size_t);
    template ZenError BaseSensor::publishArray(DeviceProperty_t, const float*, size_t);
    template ZenError BaseSensor::publishArray(DeviceProperty_t, const uint32_t*, size_t);

    template ZenError BaseSensor::publishResult(DeviceProperty_t, const bool&);
    template ZenError BaseSensor::publishResult(DeviceProperty_t, const char&);
    template ZenError BaseSensor::publishResult(DeviceProperty_t, const unsigned char&);
    template ZenError BaseSensor::publishResult(DeviceProperty_t, const float&);
    template ZenError BaseSensor::publishResult(DeviceProperty_t, const uint32_t&);

    template ZenError BaseSensor::requestAndWaitForArray(DeviceProperty_t, bool*, size_t&);
    template ZenError BaseSensor::requestAndWaitForArray(DeviceProperty_t, char*, size_t&);
    template ZenError BaseSensor::requestAndWaitForArray(DeviceProperty_t, unsigned char*, size_t&);
    template ZenError BaseSensor::requestAndWaitForArray(DeviceProperty_t, float*, size_t&);
    template ZenError BaseSensor::requestAndWaitForArray(DeviceProperty_t, uint32_t*, size_t&);

    template ZenError BaseSensor::requestAndWaitForResult(DeviceProperty_t, bool&);
    template ZenError BaseSensor::requestAndWaitForResult(DeviceProperty_t, char&);
    template ZenError BaseSensor::requestAndWaitForResult(DeviceProperty_t, unsigned char&);
    template ZenError BaseSensor::requestAndWaitForResult(DeviceProperty_t, float&);
    template ZenError BaseSensor::requestAndWaitForResult(DeviceProperty_t, uint32_t&);
}