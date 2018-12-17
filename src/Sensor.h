#ifndef ZEN_SENSOR_H_
#define ZEN_SENSOR_H_

#include <atomic>
#include <chrono>
#include <memory>
#include <thread>
#include <vector>

#include "InternalTypes.h"
#include "IZenSensor.h"

#include "ISensorComponent.h"
#include "io/interfaces/AsyncIoInterface.h"

namespace zen
{
    class Sensor : public IZenSensor, private IIoDataSubscriber
    {
    public:
        Sensor(std::unique_ptr<BaseIoInterface> ioInterface);
        ~Sensor();

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

        /** If successful, directs the outTypes pointer to a list of sensor components and sets its length.
         * Otherwise, sets the required buffer length in outLength and returns an error.
         */
        ZenError componentTypes(ZenSensorType** outTypes, size_t* outLength) const override;

        /** Returns whether the sensor is equal to the sensor description */
        bool equals(const ZenSensorDesc* desc) const override;

        /** Returns the sensor's sampling rate. */
        int32_t samplingRate() const { return m_samplingRate; }

    private:
        ZenError processData(uint8_t address, uint8_t function, const unsigned char* data, size_t length) override;

        ZenError supportedBaudRates(void* buffer, size_t& bufferSize) const;

        void upload(std::vector<unsigned char> firmware);

        std::vector<std::unique_ptr<ISensorComponent>> m_components;

        AsyncIoInterface m_ioInterface;

        std::atomic_int32_t m_samplingRate;

        std::atomic_bool m_updatingFirmware;
        std::atomic_bool m_updatedFirmware;
        ZenError m_updateFirmwareError;

        std::atomic_bool m_updatingIAP;
        std::atomic_bool m_updatedIAP;
        ZenError m_updateIAPError;

        unsigned int m_version;

        std::thread m_uploadThread;
    };
}

#endif
