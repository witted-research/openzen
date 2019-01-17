#ifndef ZEN_PROPERTIES_LEGACYCOREPROPERTIES_H_
#define ZEN_PROPERTIES_LEGACYCOREPROPERTIES_H_

#include <atomic>

#include "IZenSensor.h"
#include "io/interfaces/AsyncIoInterface.h"

namespace zen
{
    class LegacyCoreProperties : public IZenSensorProperties
    {
    public:
        LegacyCoreProperties(AsyncIoInterface& ioInterface);

        /** If successful executes the command, therwise returns an error. */
        ZenError execute(ZenProperty_t property) override;

        /** If successful fills the buffer with the array of properties and sets the buffer's size.
         * Otherwise, returns an error and potentially sets the desired buffer size - if it is too small.
         */
        ZenError getArray(ZenProperty_t property, ZenPropertyType type, void* const buffer, size_t* bufferSize) override;

        /** If successful fills the value with the property's boolean value, otherwise returns an error. */
        ZenError getBool(ZenProperty_t property, bool* const outValue) override;

        /** If successful fills the value with the property's floating-point value, otherwise returns an error. */
        ZenError getFloat(ZenProperty_t property, float* const outValue) override;

        /** If successful fills the value with the property's integer value, otherwise returns an error. */
        ZenError getInt32(ZenProperty_t property, int32_t* const outValue) override;

        /** If successful fills the value with the property's matrix value, otherwise returns an error. */
        ZenError getMatrix33(ZenProperty_t property, ZenMatrix3x3f* const outValue) override;

        /** If successful fills the buffer with the property's string value and sets the buffer's string size.
         * Otherwise, returns an error and potentially sets the desired buffer size - if it is too small.
         */
        ZenError getString(ZenProperty_t property, char* const buffer, size_t* const bufferSize) override;

        /** If successful fills the value with the property's unsigned integer value, otherwise returns an error. */
        ZenError getUInt64(ZenProperty_t property, uint64_t* const outValue) override { return ZenError_UnknownProperty; }

        /** If successful sets the array properties, otherwise returns an error. */
        ZenError setArray(ZenProperty_t property, ZenPropertyType type, const void* buffer, size_t bufferSize) override;

        /** If successful sets the boolean property, otherwise returns an error. */
        ZenError setBool(ZenProperty_t property, bool value) override;

        /** If successful sets the floating-point property, otherwise returns an error. */
        ZenError setFloat(ZenProperty_t property, float value) override;

        /** If successful sets the integer property, otherwise returns an error. */
        ZenError setInt32(ZenProperty_t property, int32_t value) override;

        /** If successful sets the matrix property, otherwise returns an error. */
        ZenError setMatrix33(ZenProperty_t property, const ZenMatrix3x3f* const value) override;

        /** If successful sets the string property, otherwise returns an error. */
        ZenError setString(ZenProperty_t property, const char* buffer, size_t bufferSize) override;

        /** If successful sets the unsigned integer property, otherwise returns an error. */
        ZenError setUInt64(ZenProperty_t property, uint64_t value) override { return ZenError_UnknownProperty; }

        /** Returns whether the property is an array type */
        bool isArray(ZenProperty_t property) const override;

        /** Returns whether the property can be executed as a command */
        bool isCommand(ZenProperty_t property) const override;

        /** Returns whether the property is constant. If so, the property cannot be set */
        bool isConstant(ZenProperty_t property) const override;

        /** Returns the type of the property */
        ZenPropertyType type(ZenProperty_t property) const override;

    private:
        ZenError supportedBaudRates(void* buffer, size_t& bufferSize) const;

        AsyncIoInterface& m_ioInterface;

        std::atomic_int32_t m_samplingRate;
    };
}

#endif