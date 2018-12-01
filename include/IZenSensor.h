#ifndef ZEN_API_IZENSENSOR_H_
#define ZEN_API_IZENSENSOR_H_

#include <cstdint>

#include "ZenTypes.h"

class IZenSensor
{
protected:
    /** Protected destructor, to prevent usage on pointer. Instead call IZenSensorManager::release */
    virtual ~IZenSensor() = default;

public:
    /** On first call, tries to initialises a firmware update, and returns an error on failure.
        * Subsequent calls do not require a valid buffer and buffer size, and only report the current status:
        * Returns ZenAsync_Updating while busy updating firmware.
        * Returns ZenAsync_Finished once the entire firmware has been written to the sensor.
        * Returns ZenAsync_Failed if an error has occured while updating.
        */
    virtual ZenAsyncStatus updateFirmwareAsync(const char* const buffer, size_t bufferSize) = 0;

    /** On first call, tries to initialises am IAP update, and returns an error on failure.
        * Subsequent calls do not require a valid buffer and buffer size, and only report the current status:
        * Returns ZenAsync_Updating while busy updating IAP.
        * Returns ZenAsync_Finished once the entire IAP has been written to the sensor.
        * Returns ZenAsync_Failed if an error has occured while updating.
        */
    virtual ZenAsyncStatus updateIAPAsync(const char* const buffer, size_t bufferSize) = 0;

    /** If successfull, executes the command, otherwise returns an error. */
    virtual ZenError executeDeviceCommand(ZenCommand_t command) = 0;

    /** If successfull, fills the buffer with the array of properties and sets the buffer's size.
        * Otherwise, returns an error and potentially sets the desired buffer size - if it is too small.
        */
    virtual ZenError getArrayDeviceProperty(ZenProperty_t property, ZenPropertyType type, void* const buffer, size_t* const bufferSize) = 0;
        
    /** If successfull, fills the value with the property's boolean value, otherwise returns an error. */
    virtual ZenError getBoolDeviceProperty(ZenProperty_t property, bool* const outValue) = 0;

    /** If successfull, fills the value with the property's floating-point value, otherwise returns an error. */
    virtual ZenError getFloatDeviceProperty(ZenProperty_t property, float* const outValue) = 0;

    /** If successfull, fills the value with the property's integer value, otherwise returns an error. */
    virtual ZenError getInt32DeviceProperty(ZenProperty_t property, int32_t* const outValue) = 0;

    /** If successfull, fills the value with the property's matrix value, otherwise returns an error. */
    virtual ZenError getMatrix33DeviceProperty(ZenProperty_t property, ZenMatrix3x3f* const outValue) = 0;

    /** If successfull, fills the buffer with the property's string value and sets the buffer's string size.
    * Otherwise, returns an error and potentially sets the desired buffer size - if it is too small.
    */
    virtual ZenError getStringDeviceProperty(ZenProperty_t property, char* const buffer, size_t* const bufferSize) = 0;

    /** If successfull, sets the array propertis, otherwise returns an error. */
    virtual ZenError setArrayDeviceProperty(ZenProperty_t property, ZenPropertyType type, const void* const buffer, size_t bufferSize) = 0;

    /** If successfull, sets the boolean property, otherwise returns an error. */
    virtual ZenError setBoolDeviceProperty(ZenProperty_t property, bool value) = 0;

    /** If successfull, sets the floating-point property, otherwise returns an error. */
    virtual ZenError setFloatDeviceProperty(ZenProperty_t property, float value) = 0;

    /** If successfull, sets the integer property, otherwise returns an error. */
    virtual ZenError setInt32DeviceProperty(ZenProperty_t property, int32_t value) = 0;

    /** If successfull, sets the matrix property, otherwise returns an error. */
    virtual ZenError setMatrix33DeviceProperty(ZenProperty_t property, const ZenMatrix3x3f* const m) = 0;

    /** If successfull, sets the string property, otherwise returns an error. */
    virtual ZenError setStringDeviceProperty(ZenProperty_t property, const char* const buffer, size_t bufferSize) = 0;
        
    /** Returns the type of the sensor */
    virtual ZenSensorType type() const = 0;

    /** Returns whether the sensor is equal to the sensor description */
    virtual bool equals(const ZenSensorDesc* desc) const = 0;
};

#endif