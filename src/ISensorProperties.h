#ifndef ZEN_ISENSORPROPERTIES_H_
#define ZEN_ISENSORPROPERTIES_H_

#include <cstdint>
#include <cstddef>

#include "ZenTypes.h"

class ISensorProperties
{
public:
    virtual ~ISensorProperties() = default;

    /** If successful executes the property, otherwise returns an error. */
    virtual ZenError execute(ZenProperty_t property) = 0;

    /** If successful fills the buffer with the array of properties and sets the buffer's size.
     * Otherwise, returns an error and potentially sets the desired buffer size - if it is too small.
     */
    virtual ZenError getArray(ZenProperty_t property, ZenPropertyType type, void* const buffer, size_t& bufferSize) = 0;

    /** If successful fills the value with the property's boolean value, otherwise returns an error. */
    virtual ZenError getBool(ZenProperty_t property, bool& outValue) = 0;

    /** If successful fills the value with the property's floating-point value, otherwise returns an error. */
    virtual ZenError getFloat(ZenProperty_t property, float& outValue) = 0;

    /** If successful fills the value with the property's signed integer value, otherwise returns an error. */
    virtual ZenError getInt32(ZenProperty_t property, int32_t& outValue) = 0;

    /** If successful fills the value with the property's matrix value, otherwise returns an error. */
    virtual ZenError getMatrix33(ZenProperty_t property, ZenMatrix3x3f& outValue) = 0;

    /** If successful fills the buffer with the property's string value and sets the buffer's string size.
     * Otherwise, returns an error and potentially sets the desired buffer size - if it is too small.
     */
    virtual ZenError getString(ZenProperty_t property, char* const buffer, size_t& bufferSize) = 0;

    /** If successful fills the value with the property's unsigned integer value, otherwise returns an error. */
    virtual ZenError getUInt64(ZenProperty_t property, uint64_t& outValue) = 0;

    /** If successful sets the array properties, otherwise returns an error. */
    virtual ZenError setArray(ZenProperty_t property, ZenPropertyType type, const void* buffer, size_t bufferSize) = 0;

    /** If successful sets the boolean property, otherwise returns an error. */
    virtual ZenError setBool(ZenProperty_t property, bool value) = 0;

    /** If successful sets the floating-point property, otherwise returns an error. */
    virtual ZenError setFloat(ZenProperty_t property, float value) = 0;

    /** If successful sets the integer property, otherwise returns an error. */
    virtual ZenError setInt32(ZenProperty_t property, int32_t value) = 0;

    /** If successful sets the matrix property, otherwise returns an error. */
    virtual ZenError setMatrix33(ZenProperty_t property, const ZenMatrix3x3f& value) = 0;

    /** If successful sets the string property, otherwise returns an error. */
    virtual ZenError setString(ZenProperty_t property, const char* buffer, size_t bufferSize) = 0;

    /** If successful sets the unsigned integer property, otherwise returns an error. */
    virtual ZenError setUInt64(ZenProperty_t property, uint64_t value) = 0;

    /** Returns whether the property is an array type */
    virtual bool isArray(ZenProperty_t property) const = 0;

    /** Returns whether the property is constant. If so, the property cannot be set */
    virtual bool isConstant(ZenProperty_t property) const = 0;

    /** Returns whether the property can be executed as a command */
    virtual bool isExecutable(ZenProperty_t property) const = 0;

    /** Returns the type of the property */
    virtual ZenPropertyType type(ZenProperty_t property) const = 0;
};

#endif
