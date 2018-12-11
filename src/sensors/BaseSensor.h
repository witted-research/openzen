#ifndef ZEN_SENSORS_BASESENSOR_H_
#define ZEN_SENSORS_BASESENSOR_H_

#include <atomic>
#include <chrono>
#include <memory>
#include <thread>

#include "InternalTypes.h"
#include "IZenSensor.h"

#include "io/interfaces/BaseIoInterface.h"
#include "utility/ThreadFence.h"

namespace zen
{
    class BaseSensor : public IZenSensor, private IIoDataSubscriber
    {
        constexpr static auto IO_TIMEOUT = std::chrono::milliseconds(1500);

    protected:
        BaseSensor(std::unique_ptr<BaseIoInterface> ioInterface);

    public:
        ~BaseSensor();

        /** Allow the sensor to initialize variables, that require an active IO interface */
        ZenError init();

        ZenError poll();

        /** On first call, tries to initialize a firmware update, and returns an error on failure.
         * Subsequent calls do not require a valid buffer and buffer size, and only report the current status:
         * Returns ZenAsync_Updating while busy updating firmware.
         * Returns ZenAsync_Finished once the entire firmware has been written to the sensor.
         * Returns ZenAsync_Failed if an error has occured while updating.
         */
        ZenAsyncStatus updateFirmwareAsync(const char* const buffer, size_t bufferSize) override;

        /** On first call, tries to initialize an IAP update, and returns an error on failure.
         * Subsequent calls do not require a valid buffer and buffer size, and only report the current status:
         * Returns ZenAsync_Updating while busy updating IAP.
         * Returns ZenAsync_Finished once the entire IAP has been written to the sensor.
         * Returns ZenAsync_Failed if an error has occured while updating.
         */
        ZenAsyncStatus updateIAPAsync(const char* const buffer, size_t bufferSize) override;

        /** If successful executes the command, therwise returns an error. */
        ZenError executeDeviceCommand(ZenCommand_t command) override;

        /** If successful fills the buffer with the array of properties and sets the buffer's size.
         * Otherwise, returns an error and potentially sets the desired buffer size - if it is too small.
         */
        ZenError getArrayDeviceProperty(ZenProperty_t property, ZenPropertyType type, void* const buffer, size_t* bufferSize) override;

        /** If successful fills the value with the property's boolean value, otherwise returns an error. */
        ZenError getBoolDeviceProperty(ZenProperty_t property, bool* const outValue) override;

        /** If successful fills the value with the property's floating-point value, otherwise returns an error. */
        ZenError getFloatDeviceProperty(ZenProperty_t property, float* const outValue) override;

        /** If successful fills the value with the property's integer value, otherwise returns an error. */
        ZenError getInt32DeviceProperty(ZenProperty_t property, int32_t* const outValue) override;

        /** If successful fills the value with the property's matrix value, otherwise returns an error. */
        ZenError getMatrix33DeviceProperty(ZenProperty_t property, ZenMatrix3x3f* const outValue) override;

        /** If successful fills the buffer with the property's string value and sets the buffer's string size.
         * Otherwise, returns an error and potentially sets the desired buffer size - if it is too small.
         */
        ZenError getStringDeviceProperty(ZenProperty_t property, char* const buffer, size_t* const bufferSize) override;

        /** If successful sets the array properties, otherwise returns an error. */
        ZenError setArrayDeviceProperty(ZenProperty_t property, ZenPropertyType type, const void* buffer, size_t bufferSize) override;

        /** If successful sets the boolean property, otherwise returns an error. */
        ZenError setBoolDeviceProperty(ZenProperty_t property, bool value) override;

        /** If successful sets the floating-point property, otherwise returns an error. */
        ZenError setFloatDeviceProperty(ZenProperty_t property, float value) override;

        /** If successful sets the integer property, otherwise returns an error. */
        ZenError setInt32DeviceProperty(ZenProperty_t property, int32_t value) override;

        /** If successful sets the matrix property, otherwise returns an error. */
        ZenError setMatrix33DeviceProperty(ZenProperty_t property, const ZenMatrix3x3f* const m) override;

        /** If successful sets the string property, otherwise returns an error. */
        ZenError setStringDeviceProperty(ZenProperty_t property, const char* buffer, size_t bufferSize) override;

        /** Returns whether the sensor is equal to the sensor description */
        bool equals(const ZenSensorDesc* desc) const override;

    protected:
        ZenError sendAndWaitForAck(DeviceProperty_t property, const unsigned char* data, size_t length);

        template <typename T>
        ZenError requestAndWaitForArray(DeviceProperty_t property, T* outArray, size_t& outLength);

        template <typename T>
        ZenError requestAndWaitForResult(DeviceProperty_t property, T& outValue);

        ZenError publishAck(DeviceProperty_t property, bool b);

        template <typename T>
        ZenError publishArray(DeviceProperty_t property, const T* array, size_t length);

        template <typename T>
        ZenError publishResult(DeviceProperty_t property, const T& result);

        /** Allow the sensor extension to initialize variables, that require an active IO interface */
        virtual ZenError initExtension() = 0;

        /** If successful extension executes the command, therwise returns an error. */
        virtual ZenError executeExtensionDeviceCommand(ZenCommand_t command) = 0;

        /** If successful extension fills the buffer with the array of properties and sets the buffer's size.
         * Otherwise, returns an error and potentially sets the desired buffer size - if it is too small.
         */
        virtual ZenError getExtensionArrayDeviceProperty(ZenProperty_t property, ZenPropertyType type, void* const buffer, size_t& bufferSize) = 0;

        /** If successful fills the value with the property's boolean value, otherwise returns an error. */
        virtual ZenError getExtensionBoolDeviceProperty(ZenProperty_t property, bool& outValue) = 0;

        /** If successful fills the value with the property's floating-point value, otherwise returns an error. */
        virtual ZenError getExtensionFloatDeviceProperty(ZenProperty_t property, float& outValue) = 0;

        /** If successful fills the value with the property's integer value, otherwise returns an error. */
        virtual ZenError getExtensionInt32DeviceProperty(ZenProperty_t property, int32_t& outValue) = 0;

        /** If successful fills the value with the property's matrix value, otherwise returns an error. */
        virtual ZenError getExtensionMatrix33DeviceProperty(ZenProperty_t property, ZenMatrix3x3f& outValue) = 0;

        /** If successful fills the buffer with the property's string value and sets the buffer's string size.
         * Otherwise, returns an error and potentially sets the desired buffer size - if it is too small.
         */
        virtual ZenError getExtensionStringDeviceProperty(ZenProperty_t property, char* const buffer, size_t& bufferSize) = 0;

        /** If successful sets the array propertis, otherwise returns an error. */
        virtual ZenError setExtensionArrayDeviceProperty(ZenProperty_t property, ZenPropertyType type, const void* buffer, size_t bufferSize) = 0;

        /** If successful sets the boolean property, otherwise returns an error. */
        virtual ZenError setExtensionBoolDeviceProperty(ZenProperty_t property, bool value) = 0;

        /** If successful sets the floating-point property, otherwise returns an error. */
        virtual ZenError setExtensionFloatDeviceProperty(ZenProperty_t property, float value) = 0;

        /** If successful sets the integer property, otherwise returns an error. */
        virtual ZenError setExtensionInt32DeviceProperty(ZenProperty_t property, int32_t value) = 0;

        /** If successful sets the matrix property, otherwise returns an error. */
        virtual ZenError setExtensionMatrix33DeviceProperty(ZenProperty_t property, const ZenMatrix3x3f& m) = 0;

        /** If successful sets the string property, otherwise returns an error. */
        virtual ZenError setExtensionStringDeviceProperty(ZenProperty_t property, const char* buffer, size_t bufferSize) = 0;

        virtual ZenError extensionProcessData(uint8_t function, const unsigned char* data, size_t length) = 0;

        std::atomic_uint32_t m_samplingRate;

    private:
        ZenError processData(uint8_t address, uint8_t function, const unsigned char* data, size_t length) override;

        ZenError supportedBaudRates(void* buffer, size_t& bufferSize) const;

        ZenError terminateWaitOnPublishOrTimeout();

        ZenError tryToWait(DeviceProperty_t property, bool forAck);

        bool prepareForPublishing();

        bool corruptMessage(DeviceProperty_t property, bool isAck);

        void upload(std::vector<unsigned char> firmware);

        ThreadFence m_fence;
        std::atomic_flag m_waiting;
        std::atomic_flag m_publishing;

        std::atomic_bool m_updatingFirmware;
        std::atomic_bool m_updatedFirmware;
        ZenError m_updateFirmwareError;

        std::atomic_bool m_updatingIAP;
        std::atomic_bool m_updatedIAP;
        ZenError m_updateIAPError;

        ZenError m_resultError;
        bool m_waitingForAck;  // If not, waiting for data
        DeviceProperty_t m_property;

        void* m_resultPtr;
        size_t* m_resultSizePtr;

        std::unique_ptr<BaseIoInterface> m_ioInterface;

        unsigned int m_version;

        std::thread m_uploadThread;
    };
}

#endif