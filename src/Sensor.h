#ifndef ZEN_SENSOR_H_
#define ZEN_SENSOR_H_

#include <atomic>
#include <chrono>
#include <memory>
#include <thread>
#include <vector>

#include "InternalTypes.h"
#include "IZenSensor.h"

#include "SensorComponent.h"
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

        IZenSensorProperties* properties() override { return m_properties.get(); }

        /** If successful, directs the outComponents pointer to a list of sensor components and sets its length to outLength.
         * Otherwise, returns an error.
         */
        ZenError components(IZenSensorComponent*** outComponents, size_t* outLength) const override;

        /** Returns whether the sensor is equal to the sensor description */
        bool equals(const ZenSensorDesc* desc) const override;

        /** Returns the sensor's sampling rate. */
        int32_t samplingRate() const { return m_samplingRate; }

    private:
        ZenError processData(uint8_t address, uint8_t function, const unsigned char* data, size_t length) override;

        void upload(std::vector<unsigned char> firmware);

        std::vector<std::unique_ptr<SensorComponent>> m_components;
        std::unique_ptr<IZenSensorProperties> m_properties;

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