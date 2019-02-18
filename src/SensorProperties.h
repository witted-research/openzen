#ifndef ZEN_SENSORPROPERTIES_H_
#define ZEN_SENSORPROPERTIES_H_

#include "ISensorProperties.h"
#include "io/interfaces/AsyncIoInterface.h"

namespace zen
{
    template <typename PropertyRules>
    class SensorProperties : public ISensorProperties
    {
    public:
        SensorProperties(uint8_t id, AsyncIoInterface& ioInterface);

        /** If successful executes the command, therwise returns an error. */
        ZenError execute(ZenProperty_t property) override;

        /** If successful fills the buffer with the array of properties and sets the buffer's size.
         * Otherwise, returns an error and potentially sets the desired buffer size - if it is too small.
         */
        ZenError getArray(ZenProperty_t property, ZenPropertyType type, void* const buffer, size_t& bufferSize) override;

        /** If successful fills the value with the property's boolean value, otherwise returns an error. */
        ZenError getBool(ZenProperty_t property, bool& outValue) override;

        /** If successful fills the value with the property's floating-point value, otherwise returns an error. */
        ZenError getFloat(ZenProperty_t property, float& outValue) override;

        /** If successful fills the value with the property's integer value, otherwise returns an error. */
        ZenError getInt32(ZenProperty_t property, int32_t& outValue) override;

        /** If successful fills the value with the property's matrix value, otherwise returns an error. */
        ZenError getMatrix33(ZenProperty_t property, ZenMatrix3x3f& outValue) override;

        /** If successful fills the buffer with the property's string value and sets the buffer's string size.
         * Otherwise, returns an error and potentially sets the desired buffer size - if it is too small.
         */
        ZenError getString(ZenProperty_t property, char* const buffer, size_t& bufferSize) override;

        /** If successful fills the value with the property's unsigned integer value, otherwise returns an error. */
        ZenError getUInt64(ZenProperty_t property, uint64_t& outValue) override;

        /** If successful sets the array properties, otherwise returns an error. */
        ZenError setArray(ZenProperty_t property, ZenPropertyType type, const void* buffer, size_t bufferSize) override;

        /** If successful sets the boolean property, otherwise returns an error. */
        ZenError setBool(ZenProperty_t property, bool value) override;

        /** If successful sets the floating-point property, otherwise returns an error. */
        ZenError setFloat(ZenProperty_t property, float value) override;

        /** If successful sets the integer property, otherwise returns an error. */
        ZenError setInt32(ZenProperty_t property, int32_t value) override;

        /** If successful sets the matrix property, otherwise returns an error. */
        ZenError setMatrix33(ZenProperty_t property, const ZenMatrix3x3f& value) override;

        /** If successful sets the string property, otherwise returns an error. */
        ZenError setString(ZenProperty_t property, const char* buffer, size_t bufferSize) override;

        /** If successful sets the unsigned integer property, otherwise returns an error. */
        ZenError setUInt64(ZenProperty_t property, uint64_t value) override;

        /** Returns whether the property is an array type */
        bool isArray(ZenProperty_t property) const override { return m_rules.isArray(property); }

        /** Returns whether the property is constant. If so, the property cannot be set */
        bool isConstant(ZenProperty_t property) const override { return m_rules.isConstant(property); }

        /** Returns whether the property can be executed as a command */
        bool isExecutable(ZenProperty_t property) const override { return m_rules.isExecutable(property); }

        /** Returns the type of the property */
        ZenPropertyType type(ZenProperty_t property) const override { return m_rules.type(property); }

    private:
        template <ZenPropertyType PropertyType, typename T>
        ZenError getResult(ZenProperty_t property, T& outValue);

        template <ZenPropertyType PropertyType, typename T>
        ZenError setAndAck(ZenProperty_t property, T value);

        AsyncIoInterface& m_ioInterface;
        PropertyRules m_rules;
        const uint8_t m_id;
    };

    namespace properties
    {
        ZenError publishAck(ISensorProperties& self, AsyncIoInterface& ioInterface, ZenProperty_t property, ZenError error);

        ZenError publishResult(ISensorProperties& self, AsyncIoInterface& ioInterface, ZenProperty_t property, ZenError error, const unsigned char* data, size_t length);
    }

    constexpr bool isCoreProperty(ZenProperty_t property)
    {
        return !(property >= ZenSensorProperty_SensorSpecific_Start && property <= ZenSensorProperty_SensorSpecific_End);
    }

    constexpr size_t sizeOfPropertyType(ZenPropertyType type)
    {
        switch (type)
        {
        case ZenPropertyType_Bool:
            return sizeof(bool);

        case ZenPropertyType_Float:
            return sizeof(float);

        case ZenPropertyType_Int32:
            return sizeof(int32_t);

        case ZenPropertyType_UInt64:
            return sizeof(uint64_t);

        case ZenPropertyType_Matrix:
            return 9 * sizeof(float);

        case ZenPropertyType_String:
            return sizeof(char);

        default:
            return 0;
        }
    }
}

#endif
