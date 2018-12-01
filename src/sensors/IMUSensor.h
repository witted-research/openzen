#ifndef ZEN_SENSORS_IMUSENSOR_H_
#define ZEN_SENSORS_IMUSENSOR_H_

#include "sensors/BaseSensor.h"

#include "LpMatrix.h"

namespace zen
{
    class ImuSensor : public BaseSensor
    {
    public:
        ImuSensor() = delete;
        ImuSensor(std::unique_ptr<BaseIoInterface> ioInterface);

        ZenError initExtension() override;;

        /** If successfull, executes the command, therwise returns an error. */
        ZenError executeExtensionDeviceCommand(ZenCommand_t command) override;

        /** If successfull, fills the buffer with the array of properties and sets the buffer's size.
         * Otherwise, returns an error and potentially sets the desired buffer size - if it is too small.
         */
        ZenError getExtensionArrayDeviceProperty(ZenProperty_t property, ZenPropertyType type, void* const buffer, size_t& bufferSize) override;

        /** If successfull, fills the value with the property's boolean value, otherwise returns an error. */
        ZenError getExtensionBoolDeviceProperty(ZenProperty_t property, bool& outValue) override;

        /** If successfull, fills the value with the property's floating-point value, otherwise returns an error. */
        ZenError getExtensionFloatDeviceProperty(ZenProperty_t property, float& outValue) override;

        /** If successfull, fills the value with the property's integer value, otherwise returns an error. */
        ZenError getExtensionInt32DeviceProperty(ZenProperty_t property, int32_t& outValue) override;

        /** If successfull, fills the value with the property's matrix value, otherwise returns an error. */
        ZenError getExtensionMatrix33DeviceProperty(ZenProperty_t property, ZenMatrix3x3f& outValue) override;

        /** If successfull, fills the buffer with the property's string value and sets the buffer's string size.
         * Otherwise, returns an error and potentially sets the desired buffer size - if it is too small.
         */
        ZenError getExtensionStringDeviceProperty(ZenProperty_t property, char* const buffer, size_t& bufferSize) override;

        /** If successfull, sets the array propertis, otherwise returns an error. */
        ZenError setExtensionArrayDeviceProperty(ZenProperty_t property, ZenPropertyType type, const void* buffer, size_t bufferSize) override;

        /** If successfull, sets the boolean property, otherwise returns an error. */
        ZenError setExtensionBoolDeviceProperty(ZenProperty_t property, bool value) override;

        /** If successfull, sets the floating-point property, otherwise returns an error. */
        ZenError setExtensionFloatDeviceProperty(ZenProperty_t property, float value) override;

        /** If successfull, sets the integer property, otherwise returns an error. */
        ZenError setExtensionInt32DeviceProperty(ZenProperty_t property, int32_t value) override;

        /** If successfull, sets the matrix property, otherwise returns an error. */
        ZenError setExtensionMatrix33DeviceProperty(ZenProperty_t property, const ZenMatrix3x3f& m) override;

        /** If successfull, sets the string property, otherwise returns an error. */
        ZenError setExtensionStringDeviceProperty(ZenProperty_t property, const char* buffer, size_t bufferSize) override;

        ZenError extensionProcessData(uint8_t function, const unsigned char* data, size_t length) override;

        ZenSensorType type() const override { return ZenSensor_Imu; }

    private:
        ZenError processSensorData(const unsigned char* data, size_t length);

        bool getConfigDataFlag(unsigned int index);

        ZenError setOutputDataFlag(unsigned int index, bool value);
        ZenError setPrecisionDataFlag(bool value);

        struct IMUState
        {
            std::atomic<LpMatrix3x3f> accAlignMatrix;
            std::atomic<LpMatrix3x3f> gyrAlignMatrix;
            std::atomic<LpMatrix3x3f> softIronMatrix;
            std::atomic<LpVector3f> accBias;
            std::atomic<LpVector3f> gyrBias;
            std::atomic<LpVector3f> hardIronOffset;
            std::atomic_uint32_t outputDataBitset;
        } m_cache;

        unsigned int m_version;
        std::atomic_bool m_gyrUseThreshold;
        std::atomic_bool m_gyrAutoCalibration;
    };
}
#endif
