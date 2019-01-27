#ifndef ZEN_API_OPENZEN_H_
#define ZEN_API_OPENZEN_H_

#include "IZenSensorManager.h"
#include "IZenSensor.h"

#include <algorithm>
#include <optional>
#include <utility>

namespace details
{
    template <typename T>
    struct PropertyType
    {};

    template <> struct PropertyType<bool>
    {
        using type = std::integral_constant<ZenPropertyType, ZenPropertyType_Bool>;
    };

    template <> struct PropertyType<float>
    {
        using type = std::integral_constant<ZenPropertyType, ZenPropertyType_Float>;
    };

    template <> struct PropertyType<int32_t>
    {
        using type = std::integral_constant<ZenPropertyType, ZenPropertyType_Int32>;
    };

    template <> struct PropertyType<uint64_t>
    {
        using type = std::integral_constant<ZenPropertyType, ZenPropertyType_UInt64>;
    };
}

namespace zen
{
    class ZenSensorProperties
    {
    private:
        IZenSensorProperties& m_properties;

    public:
        ZenSensorProperties(IZenSensorProperties& properties)
            : m_properties(properties)
        {}

        IZenSensorProperties& operator*() { return m_properties; }

        ZenError execute(ZenProperty_t property)
        {
            return m_properties.execute(property);
        }

        template <typename T>
        std::pair<ZenError, size_t> getArray(ZenProperty_t property, T* const array, size_t length)
        {
            auto result = std::make_pair(ZenError_None, length);
            result.first = m_properties.getArray(property, details::PropertyType<T>::type(), array, &result.second);
            return result;
        }

        std::pair<ZenError, bool> getBool(ZenProperty_t property)
        {
            auto result = std::make_pair(ZenError_None, false);
            result.first = m_properties.getBool(property, &result.second);
            return result;
        }

        std::pair<ZenError, float> getFloat(ZenProperty_t property)
        {
            auto result = std::make_pair(ZenError_None, 0.f);
            result.first = m_properties.getFloat(property, &result.second);
            return result;
        }

        std::pair<ZenError, int32_t> getInt32(ZenProperty_t property)
        {
            auto result = std::make_pair(ZenError_None, 0);
            result.first = m_properties.getInt32(property, &result.second);
            return result;
        }

        std::pair<ZenError, ZenMatrix3x3f> getMatrix33(ZenProperty_t property)
        {
            auto result = std::make_pair(ZenError_None, ZenMatrix3x3f{});
            result.first = m_properties.getMatrix33(property, &result.second);
            return result;
        }

        std::pair<ZenError, size_t> getString(ZenProperty_t property, char* const buffer, size_t length)
        {
            auto result = std::make_pair(ZenError_None, length);
            result.first = m_properties.getString(property, buffer, &result.second);
            return result;
        }

        std::pair<ZenError, uint64_t> getUInt64(ZenProperty_t property)
        {
            auto result = std::make_pair(ZenError_None, 0ull);
            result.first = m_properties.getUInt64(property, &result.second);
            return result;
        }

        template <typename T>
        ZenError setArray(ZenProperty_t property, const T* array, size_t length)
        {
            return m_properties.setArray(property, details::PropertyType<T>::type(), array, length);
        }

        ZenError setBool(ZenProperty_t property, bool value)
        {
            return m_properties.setBool(property, value);
        }

        ZenError setFloat(ZenProperty_t property, float value)
        {
            return m_properties.setFloat(property, value);
        }

        ZenError setInt32(ZenProperty_t property, int32_t value)
        {
            return m_properties.setInt32(property, value);
        }

        ZenError setMatrix33(ZenProperty_t property, const ZenMatrix3x3f& value)
        {
            return m_properties.setMatrix33(property, &value);
        }

        ZenError setString(ZenProperty_t property, std::string_view value)
        {
            return m_properties.setString(property, value.data(), value.size());
        }

        ZenError setUInt64(ZenProperty_t property, uint64_t value)
        {
            return m_properties.setUInt64(property, value);
        }
    };

    class ZenSensor
    {
    private:
        IZenSensorManager& m_manager;
        IZenSensor* m_sensor;

    public:
        ZenSensor(IZenSensorManager& manager, IZenSensor& sensor)
            : m_manager(manager)
            , m_sensor(&sensor)
        {}

        ZenSensor(ZenSensor&& other)
            : m_manager(other.m_manager)
            , m_sensor(other.m_sensor)
        {
            other.m_sensor = nullptr;
        }

        ~ZenSensor()
        {
            m_manager.release(m_sensor);
        }

        IZenSensor* operator*() { return m_sensor; }

        ZenSensorProperties properties() { return ZenSensorProperties(*m_sensor->properties()); }

        std::optional<IZenSensorComponent*> getAnyComponentOfType(std::string_view type)
        {
            IZenSensorComponent** components = nullptr;
            size_t nComponents;
            if (auto error = m_sensor->components(&components, &nComponents, type.data()))
                return std::nullopt;

            if (nComponents > 0)
                return components[0];

            return std::nullopt;
        }

        // [TODO] Remove handle
        std::pair<ZenError, ZenSensorDesc> desc()
        {
            ZenSensorDesc desc;

            size_t length = sizeof(ZenSensorDesc::name);
            if (auto error = m_sensor->properties()->getString(ZenSensorProperty_DeviceName, desc.name, &length))
                return std::make_pair(error, ZenSensorDesc{}); // [TODO] Specific error Sensor_NameTooLong

            desc.name[length] = '\0';

            length = sizeof(ZenSensorDesc::serialNumber);
            if (auto error = m_sensor->properties()->getString(ZenSensorProperty_SerialNumber, desc.serialNumber, &length))
                return std::make_pair(error, ZenSensorDesc{}); // [TODO] Specific error Sensor_SerialNumberTooLong

            desc.serialNumber[length] = '\0';

            const char* ioType = m_sensor->ioType();
            length = strnlen_s(ioType, sizeof(ZenSensorDesc::ioType));
            if (length == sizeof(ZenSensorDesc::ioType))
                return std::make_pair(ZenError_BufferTooSmall, ZenSensorDesc{}); // [TODO] Specific error Sensor_IoTypeTooLong

            std::memcpy(desc.ioType, ioType, length);
            desc.ioType[length] = '\0';
            desc.handle64 = 0;
            return std::make_pair(ZenError_None, std::move(desc));
        }
    };

    inline std::optional<ZenSensor> make_sensor(IZenSensorManager& manager, const ZenSensorDesc& desc)
    {
        IZenSensor* sensor = nullptr;
        if (auto error = manager.obtain(&desc, &sensor))
            return std::nullopt;

        return ZenSensor(manager, *sensor);
    }
}

#endif
