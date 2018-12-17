#ifndef ZEN_ISENSORCOMPONENT_H_
#define ZEN_ISENSORCOMPONENT_H_

#include <optional>

#include "ZenTypes.h"

namespace zen
{
    class ISensorComponent
    {
    public:
        /** Tries to initialize settings of the sensor's component that can fail. */
        virtual ZenError init() = 0;

        /** If successful, executes the command, otherwise returns an error. */
        virtual std::optional<ZenError> executeDeviceCommand(ZenCommand_t command) = 0;

        /** If successful, fills the buffer with the array of properties and sets the buffer's size.
         * Otherwise, returns an error and potentially sets the desired buffer size - if it is too small.
         */
        virtual std::optional<ZenError> getArrayDeviceProperty(ZenProperty_t property, ZenPropertyType type, void* const buffer, size_t& bufferSize) = 0;

        /** If successful, fills the value with the property's boolean value, otherwise returns an error. */
        virtual std::optional<ZenError> getBoolDeviceProperty(ZenProperty_t property, bool& outValue) = 0;

        /** If successful, fills the value with the property's floating-point value, otherwise returns an error. */
        virtual std::optional<ZenError> getFloatDeviceProperty(ZenProperty_t property, float& outValue) = 0;

        /** If successful, fills the value with the property's integer value, otherwise returns an error. */
        virtual std::optional<ZenError> getInt32DeviceProperty(ZenProperty_t property, int32_t& outValue) = 0;

        /** If successful, fills the value with the property's matrix value, otherwise returns an error. */
        virtual std::optional<ZenError> getMatrix33DeviceProperty(ZenProperty_t property, ZenMatrix3x3f& outValue) = 0;

        /** If successful, fills the buffer with the property's string value and sets the buffer's string size.
         * Otherwise, returns an error and potentially sets the desired buffer size - if it is too small.
         */
        virtual std::optional<ZenError> getStringDeviceProperty(ZenProperty_t property, char* const buffer, size_t& bufferSize) = 0;

        /** If successful, sets the array properties, otherwise returns an error. */
        virtual std::optional<ZenError> setArrayDeviceProperty(ZenProperty_t property, ZenPropertyType type, const void* buffer, size_t bufferSize) = 0;

        /** If successful, sets the boolean property, otherwise returns an error. */
        virtual std::optional<ZenError> setBoolDeviceProperty(ZenProperty_t property, bool value) = 0;

        /** If successful, sets the floating-point property, otherwise returns an error. */
        virtual std::optional<ZenError> setFloatDeviceProperty(ZenProperty_t property, float value) = 0;

        /** If successful, sets the integer property, otherwise returns an error. */
        virtual std::optional<ZenError> setInt32DeviceProperty(ZenProperty_t property, int32_t value) = 0;

        /** If successful, sets the matrix property, otherwise returns an error. */
        virtual std::optional<ZenError> setMatrix33DeviceProperty(ZenProperty_t property, const ZenMatrix3x3f& m) = 0;

        /** If successful, sets the string property, otherwise returns an error. */
        virtual std::optional<ZenError> setStringDeviceProperty(ZenProperty_t property, const char* buffer, size_t bufferSize) = 0;

        virtual std::optional<ZenError> processData(uint8_t function, const unsigned char* data, size_t length) = 0;

        /** Returns the type of the sensor component */
        virtual ZenSensorType type() const = 0;
    };
}

#endif
