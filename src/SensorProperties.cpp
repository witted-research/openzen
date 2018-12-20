#include "SensorProperties.h"

#include "ZenProtocol.h"

#include "properties/CorePropertyRulesV1.h"
#include "properties/ImuPropertyRulesV1.h"

namespace zen
{
    class PropertyData
    {
    public:
        template <typename T>
        PropertyData(ZenProperty_t property, T value) noexcept
            : m_buffer(sizeof(property) + sizeof(value))
        {
            auto dst = m_buffer.data();
            std::memcpy(dst, &property, sizeof(property));
            std::memcpy(dst + sizeof(property), &value, sizeof(value));

        }

        PropertyData(ZenProperty_t property, const void* array, size_t length) noexcept
            : m_buffer(sizeof(property) + length)
        {
            auto dst = m_buffer.data();
            std::memcpy(dst, &property, sizeof(property));
            std::memcpy(dst + sizeof(property), array, length);
        }

        gsl::span<const unsigned char> data() const noexcept { return gsl::make_span(m_buffer.data(), m_buffer.size()); }

    private:
        std::vector<unsigned char> m_buffer;
    };

    template <typename PropertyRules>
    SensorProperties<PropertyRules>::SensorProperties(uint8_t id, AsyncIoInterface& ioInterface)
        : m_ioInterface(ioInterface)
        , m_id(id)
    {}

    template <typename PropertyRules>
    ZenError SensorProperties<PropertyRules>::execute(ZenProperty_t property)
    {
        if (m_rules.isCommand(property))
        {
            const auto span = gsl::make_span(reinterpret_cast<const unsigned char*>(&property), sizeof(property));
            return m_ioInterface.sendAndWaitForAck(m_id, ZenProtocolFunction_Execute, property, span);
        }

        return ZenError_UnknownProperty;
    }

    template <typename PropertyRules>
    ZenError SensorProperties<PropertyRules>::getArray(ZenProperty_t property, ZenPropertyType type, void* const buffer, size_t* const bufferSize)
    {
        if (bufferSize == nullptr)
            return ZenError_IsNull;

        if (m_rules.isArray(property) && m_rules.type(property) == type)
        {
            switch (type)
            {
            case ZenPropertyType_Bool:
            {
                const auto span = gsl::make_span(reinterpret_cast<const unsigned char*>(&property), sizeof(property));
                return m_ioInterface.sendAndWaitForArray(m_id, ZenProtocolFunction_Get, property, span, reinterpret_cast<bool*>(buffer), *bufferSize);
            }

            case ZenPropertyType_Float:
            {
                const auto span = gsl::make_span(reinterpret_cast<const unsigned char*>(&property), sizeof(property));
                return m_ioInterface.sendAndWaitForArray(m_id, ZenProtocolFunction_Get, property, span, reinterpret_cast<float*>(buffer), *bufferSize);
            }

            case ZenPropertyType_Int32:
            {
                const auto span = gsl::make_span(reinterpret_cast<const unsigned char*>(&property), sizeof(property));
                return m_ioInterface.sendAndWaitForArray(m_id, ZenProtocolFunction_Get, property, span, reinterpret_cast<int32_t*>(buffer), *bufferSize);
            }

            case ZenPropertyType_UInt64:
            {
                const auto span = gsl::make_span(reinterpret_cast<const unsigned char*>(&property), sizeof(property));
                return m_ioInterface.sendAndWaitForArray(m_id, ZenProtocolFunction_Get, property, span, reinterpret_cast<uint64_t*>(buffer), *bufferSize);
            }

            default:
                return ZenError_WrongDataType;
            }
        }

        return ZenError_UnknownProperty;
    }

    template <typename PropertyRules>
    ZenError SensorProperties<PropertyRules>::getBool(ZenProperty_t property, bool* const outValue)
    {
        return getResult<ZenPropertyType_Bool>(property, outValue);
    }

    template <typename PropertyRules>
    ZenError SensorProperties<PropertyRules>::getFloat(ZenProperty_t property, float* const outValue)
    {
        return getResult<ZenPropertyType_Float>(property, outValue);
    }

    template <typename PropertyRules>
    ZenError SensorProperties<PropertyRules>::getInt32(ZenProperty_t property, int32_t* const outValue)
    {
        return getResult<ZenPropertyType_Int32>(property, outValue);
    }

    template <typename PropertyRules>
    ZenError SensorProperties<PropertyRules>::getMatrix33(ZenProperty_t property, ZenMatrix3x3f* const outValue)
    {
        if (outValue == nullptr)
            return ZenError_IsNull;

        if (m_rules.type(property) == ZenPropertyType_Matrix)
        {
            size_t length = 9;
            const auto span = gsl::make_span(reinterpret_cast<const unsigned char*>(&property), sizeof(property));
            return m_ioInterface.sendAndWaitForArray(m_id, ZenProtocolFunction_Get, property, span, outValue->data, length);
        }

        return ZenError_UnknownProperty;
    }

    template <typename PropertyRules>
    ZenError SensorProperties<PropertyRules>::getString(ZenProperty_t property, char* const buffer, size_t* const bufferSize)
    {
        if (bufferSize == nullptr)
            return ZenError_IsNull;

        if (m_rules.type(property) == ZenPropertyType_String)
        {
            const auto span = gsl::make_span(reinterpret_cast<const unsigned char*>(&property), sizeof(property));
            return m_ioInterface.sendAndWaitForArray(m_id, ZenProtocolFunction_Get, property, span, buffer, *bufferSize);
        }

        return ZenError_UnknownProperty;
    }

    template <typename PropertyRules>
    ZenError SensorProperties<PropertyRules>::getUInt64(ZenProperty_t property, uint64_t* const outValue)
    {
        return getResult<ZenPropertyType_UInt64>(property, outValue);
    }

    template <typename PropertyRules>
    ZenError SensorProperties<PropertyRules>::setArray(ZenProperty_t property, ZenPropertyType type, const void* const buffer, size_t bufferSize)
    {
        if (buffer == nullptr)
            return ZenError_IsNull;

        if (m_rules.isArray(property) && !m_rules.isConstant(property) && m_rules.type(property) == type)
        {
            const PropertyData wrapper(property, buffer, sizeOfPropertyType(type) * bufferSize);
            return m_ioInterface.sendAndWaitForAck(m_id, ZenProtocolFunction_Set, property, wrapper.data());
        }

        return ZenError_UnknownProperty;
    }

    template <typename PropertyRules>
    ZenError SensorProperties<PropertyRules>::setBool(ZenProperty_t property, bool value)
    {
        return setAndAck<ZenPropertyType_Bool>(property, value);
    }

    template <typename PropertyRules>
    ZenError SensorProperties<PropertyRules>::setFloat(ZenProperty_t property, float value)
    {
        return setAndAck<ZenPropertyType_Float>(property, value);
    }

    template <typename PropertyRules>
    ZenError SensorProperties<PropertyRules>::setInt32(ZenProperty_t property, int32_t value)
    {
        return setAndAck<ZenPropertyType_Int32>(property, value);
    }

    template <typename PropertyRules>
    ZenError SensorProperties<PropertyRules>::setMatrix33(ZenProperty_t property, const ZenMatrix3x3f* const value)
    {
        if (value == nullptr)
            return ZenError_IsNull;

        if (!m_rules.isConstant(property) && m_rules.type(property) == ZenPropertyType_Matrix)
        {
            const PropertyData wrapper(property, value->data, 9 * sizeof(float));
            return m_ioInterface.sendAndWaitForAck(m_id, ZenProtocolFunction_Set, property, wrapper.data());
        }

        return ZenError_UnknownProperty;
    }

    template <typename PropertyRules>
    ZenError SensorProperties<PropertyRules>::setString(ZenProperty_t property, const char* buffer, size_t bufferSize)
    {
        if (buffer == nullptr)
            return ZenError_IsNull;

        if (!m_rules.isConstant(property) && m_rules.type(property) == ZenPropertyType_String)
        {
            const PropertyData wrapper(property, buffer, bufferSize);
            return m_ioInterface.sendAndWaitForAck(m_id, ZenProtocolFunction_Set, property, wrapper.data());
        }

        return ZenError_UnknownProperty;
    }

    template <typename PropertyRules>
    ZenError SensorProperties<PropertyRules>::setUInt64(ZenProperty_t property, uint64_t value)
    {
        return setAndAck<ZenPropertyType_UInt64>(property, value);
    }

    template <typename PropertyRules>
    template <ZenPropertyType PropertyType, typename T>
    ZenError SensorProperties<PropertyRules>::setAndAck(ZenProperty_t property, T value)
    {
        if (!m_rules.isConstant(property) && m_rules.type(property) == PropertyType)
        {
            const PropertyData wrapper(property, value);
            return m_ioInterface.sendAndWaitForAck(m_id, ZenProtocolFunction_Set, property, wrapper.data());
        }

        return ZenError_UnknownProperty;
    }

    template <typename PropertyRules>
    template <ZenPropertyType PropertyType, typename T>
    ZenError SensorProperties<PropertyRules>::getResult(ZenProperty_t property, T* const outValue)
    {
        if (outValue == nullptr)
            return ZenError_IsNull;

        if (m_rules.type(property) == PropertyType)
        {
            const auto span = gsl::make_span(reinterpret_cast<const unsigned char*>(&property), sizeof(property));
            return m_ioInterface.sendAndWaitForResult(m_id, ZenProtocolFunction_Get, property, span, *outValue);
        }

        return ZenError_UnknownProperty;
    }

    template SensorProperties<CorePropertyRulesV1>;
    template SensorProperties<ImuPropertyRulesV1>;

    namespace properties
    {
        ZenError publishAck(IZenSensorProperties& self, AsyncIoInterface& ioInterface, ZenProperty_t property, ZenError error)
        {
            if (self.isCommand(property) || (!self.isConstant(property) && self.type(property) != ZenPropertyType_Invalid))
                return ioInterface.publishAck(property, error);

            return ZenError_InvalidArgument;
        }

        ZenError publishResult(IZenSensorProperties& self, AsyncIoInterface& ioInterface, ZenProperty_t property, ZenError error, const unsigned char* data, size_t length)
        {
            if (auto type = self.type(property))
            {
                if (self.isArray(property))
                {
                    switch (type)
                    {
                    case ZenPropertyType_Bool:
                        return ioInterface.publishArray(property, error, reinterpret_cast<const bool*>(data), length / sizeof(bool));

                    case ZenPropertyType_Float:
                        return ioInterface.publishArray(property, error, reinterpret_cast<const float*>(data), length / sizeof(float));

                    case ZenPropertyType_Int32:
                        return ioInterface.publishArray(property, error, reinterpret_cast<const int32_t*>(data), length / sizeof(int32_t));

                    case ZenPropertyType_UInt64:
                        return ioInterface.publishArray(property, error, reinterpret_cast<const uint64_t*>(data), length / sizeof(uint64_t));

                    case ZenPropertyType_String:
                        return ioInterface.publishArray(property, error, reinterpret_cast<const char*>(data), length / sizeof(char));

                    default:
                        return ZenError_InvalidArgument;
                    }
                }
                else
                {
                    if (length != sizeOfPropertyType(type))
                        return ZenError_Io_MsgCorrupt;

                    switch (type)
                    {
                    case ZenPropertyType_Bool:
                        return ioInterface.publishResult(property, error, *reinterpret_cast<const bool*>(data));

                    case ZenPropertyType_Float:
                        return ioInterface.publishResult(property, error, *reinterpret_cast<const float*>(data));

                    case ZenPropertyType_Int32:
                        return ioInterface.publishResult(property, error, *reinterpret_cast<const int32_t*>(data));

                    case ZenPropertyType_UInt64:
                        return ioInterface.publishResult(property, error, *reinterpret_cast<const uint64_t*>(data));

                    case ZenPropertyType_Matrix:
                        return ioInterface.publishArray(property, error, reinterpret_cast<const float*>(data), 9);

                    default:
                        return ZenError_InvalidArgument;
                    }
                }
            }

            return ZenError_InvalidArgument;
        }
    }
}
