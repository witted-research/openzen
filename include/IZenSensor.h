#ifndef ZEN_API_IZENSENSOR_H_
#define ZEN_API_IZENSENSOR_H_

#include <cstdint>
#include <cstddef>

#include "ZenTypes.h"

class IZenSensorProperties
{
public:
    virtual ~IZenSensorProperties() = default;

    /** If successful executes the property, otherwise returns an error. */
    virtual ZenError execute(ZenProperty_t property) = 0;

    /** If successful fills the buffer with the array of properties and sets the buffer's size.
     * Otherwise, returns an error and potentially sets the desired buffer size - if it is too small.
     */
    virtual ZenError getArray(ZenProperty_t property, ZenPropertyType type, void* const buffer, size_t* bufferSize) = 0;

    /** If successful fills the value with the property's boolean value, otherwise returns an error. */
    virtual ZenError getBool(ZenProperty_t property, bool* const outValue) = 0;

    /** If successful fills the value with the property's floating-point value, otherwise returns an error. */
    virtual ZenError getFloat(ZenProperty_t property, float* const outValue) = 0;

    /** If successful fills the value with the property's signed integer value, otherwise returns an error. */
    virtual ZenError getInt32(ZenProperty_t property, int32_t* const outValue) = 0;

    /** If successful fills the value with the property's matrix value, otherwise returns an error. */
    virtual ZenError getMatrix33(ZenProperty_t property, ZenMatrix3x3f* const outValue) = 0;

    /** If successful fills the buffer with the property's string value and sets the buffer's string size.
     * Otherwise, returns an error and potentially sets the desired buffer size - if it is too small.
     */
    virtual ZenError getString(ZenProperty_t property, char* const buffer, size_t* const bufferSize) = 0;

    /** If successful fills the value with the property's unsigned integer value, otherwise returns an error. */
    virtual ZenError getUInt64(ZenProperty_t property, uint64_t* const outValue) = 0;

    /** If successful sets the array properties, otherwise returns an error. */
    virtual ZenError setArray(ZenProperty_t property, ZenPropertyType type, const void* buffer, size_t bufferSize) = 0;

    /** If successful sets the boolean property, otherwise returns an error. */
    virtual ZenError setBool(ZenProperty_t property, bool value) = 0;

    /** If successful sets the floating-point property, otherwise returns an error. */
    virtual ZenError setFloat(ZenProperty_t property, float value) = 0;

    /** If successful sets the integer property, otherwise returns an error. */
    virtual ZenError setInt32(ZenProperty_t property, int32_t value) = 0;

    /** If successful sets the matrix property, otherwise returns an error. */
    virtual ZenError setMatrix33(ZenProperty_t property, const ZenMatrix3x3f* const value) = 0;

    /** If successful sets the string property, otherwise returns an error. */
    virtual ZenError setString(ZenProperty_t property, const char* buffer, size_t bufferSize) = 0;

    /** If successful sets the unsigned integer property, otherwise returns an error. */
    virtual ZenError setUInt64(ZenProperty_t property, uint64_t value) = 0;

    /** Returns whether the property is an array type */
    virtual bool isArray(ZenProperty_t property) const = 0;

    /** Returns whether the property can be executed as a command */
    virtual bool isCommand(ZenProperty_t property) const = 0;

    /** Returns whether the property is constant. If so, the property cannot be set */
    virtual bool isConstant(ZenProperty_t property) const = 0;

    /** Returns the type of the property */
    virtual ZenPropertyType type(ZenProperty_t property) const = 0;
};

class IZenSensorComponent
{
protected:
    /** Protected destructor, to prevent usage on pointer. Instead call IZenSensorManager::release */
    virtual ~IZenSensorComponent() = default;

public:
    virtual IZenSensorProperties* properties() = 0;

    /** Returns the type of the sensor component */
    virtual ZenSensorType type() const = 0;
};

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

    /** On first call, tries to initialise an IAP update, and returns an error on failure.
     * Subsequent calls do not require a valid buffer and buffer size, and only report the current status:
     * Returns ZenAsync_Updating while busy updating IAP.
     * Returns ZenAsync_Finished once the entire IAP has been written to the sensor.
     * Returns ZenAsync_Failed if an error has occured while updating.
     */
    virtual ZenAsyncStatus updateIAPAsync(const char* const buffer, size_t bufferSize) = 0;

    virtual IZenSensorProperties* properties() = 0;

    /** If successful, directs the outComponents pointer to a list of sensor components and sets its length to outLength.
     * Otherwise, returns an error.
     */
    virtual ZenError components(IZenSensorComponent*** outComponents, size_t* outLength) const = 0;

    /** Returns whether the sensor is equal to the sensor description */
    virtual bool equals(const ZenSensorDesc* desc) const = 0;
};

#endif
