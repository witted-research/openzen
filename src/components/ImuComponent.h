#ifndef ZEN_COMPONENTS_IMUCOMPONENT_H_
#define ZEN_COMPONENTS_IMUCOMPONENT_H_

#include <atomic>

#include "ISensorComponent.h"
#include "io/interfaces/AsyncIoInterface.h"

#include "LpMatrix.h"

namespace zen
{
    class ImuComponent : public ISensorComponent
    {
    public:
        ImuComponent(class Sensor& base, AsyncIoInterface& ioInterface);

        ZenError init() override;

        /** If successful executes the command, otherwise returns an error. */
        std::optional<ZenError> executeDeviceCommand(ZenCommand_t command) override;

        /** If successful fills the buffer with the array of properties and sets the buffer's size.
         * Otherwise, returns an error and potentially sets the desired buffer size - if it is too small.
         */
        std::optional<ZenError> getArrayDeviceProperty(ZenProperty_t property, ZenPropertyType type, void* const buffer, size_t& bufferSize) override;

        /** If successful fills the value with the property's boolean value, otherwise returns an error. */
        std::optional<ZenError> getBoolDeviceProperty(ZenProperty_t property, bool& outValue) override;

        /** If successful fills the value with the property's floating-point value, otherwise returns an error. */
        std::optional<ZenError> getFloatDeviceProperty(ZenProperty_t property, float& outValue) override;

        /** If successful fills the value with the property's integer value, otherwise returns an error. */
        std::optional<ZenError> getInt32DeviceProperty(ZenProperty_t property, int32_t& outValue) override;

        /** If successful fills the value with the property's matrix value, otherwise returns an error. */
        std::optional<ZenError> getMatrix33DeviceProperty(ZenProperty_t property, ZenMatrix3x3f& outValue) override;

        /** If successful fills the buffer with the property's string value and sets the buffer's string size.
         * Otherwise, returns an error and potentially sets the desired buffer size - if it is too small.
         */
        std::optional<ZenError> getStringDeviceProperty(ZenProperty_t property, char* const buffer, size_t& bufferSize) override;

        /** If successful sets the array properties, otherwise returns an error. */
        std::optional<ZenError> setArrayDeviceProperty(ZenProperty_t property, ZenPropertyType type, const void* buffer, size_t bufferSize) override;

        /** If successful sets the boolean property, otherwise returns an error. */
        std::optional<ZenError> setBoolDeviceProperty(ZenProperty_t property, bool value) override;

        /** If successful sets the floating-point property, otherwise returns an error. */
        std::optional<ZenError> setFloatDeviceProperty(ZenProperty_t property, float value) override;

        /** If successful sets the integer property, otherwise returns an error. */
        std::optional<ZenError> setInt32DeviceProperty(ZenProperty_t property, int32_t value) override;

        /** If successful sets the matrix property, otherwise returns an error. */
        std::optional<ZenError> setMatrix33DeviceProperty(ZenProperty_t property, const ZenMatrix3x3f& m) override;

        /** If successful sets the string property, otherwise returns an error. */
        std::optional<ZenError> setStringDeviceProperty(ZenProperty_t property, const char* buffer, size_t bufferSize) override;

        std::optional<ZenError> processData(uint8_t function, const unsigned char* data, size_t length) override;

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

        class Sensor& m_base;
        AsyncIoInterface& m_ioInterface;

        unsigned int m_version;
        std::atomic_bool m_gyrUseThreshold;
        std::atomic_bool m_gyrAutoCalibration;
    };
}
#endif
