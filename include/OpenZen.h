#ifndef ZEN_API_OPENZEN_H_
#define ZEN_API_OPENZEN_H_

#include "OpenZenCAPI.h"

#include <algorithm>
#include <chrono>
#include <optional>
#include <string_view>
#include <thread>
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
    class ZenClient;

    class ZenSensorComponent
    {
    private:
        ZenClientHandle_t m_clientHandle;
        ZenSensorHandle_t m_sensorHandle;
        ZenComponentHandle_t m_componentHandle;

    public:
        ZenSensorComponent(ZenClientHandle_t clientHandle, ZenSensorHandle_t sensorHandle, ZenComponentHandle_t componentHandle)
            : m_clientHandle(clientHandle)
            , m_sensorHandle(sensorHandle)
            , m_componentHandle(componentHandle)
        {}

        ZenSensorComponent(const ZenSensorComponent& other)
            : m_clientHandle(other.m_clientHandle)
            , m_sensorHandle(other.m_sensorHandle)
            , m_componentHandle(other.m_componentHandle)
        {}

        ZenSensorHandle_t sensor() const noexcept
        {
            return m_sensorHandle;
        }

        ZenComponentHandle_t component() const noexcept
        {
            return m_componentHandle;
        }

        std::string_view type() const noexcept
        {
            return ZenSensorComponentType(m_clientHandle, m_sensorHandle, m_componentHandle);
        }

        ZenError executeProperty(ZenProperty_t property)
        {
            return ZenSensorComponentExecuteProperty(m_clientHandle, m_sensorHandle, m_componentHandle, property);
        }

        template <typename T>
        std::pair<ZenError, size_t> getArrayProperty(ZenProperty_t property, T* const array, size_t length)
        {
            auto result = std::make_pair(ZenError_None, length);
            result.first = ZenSensorComponentGetArrayProperty(m_clientHandle, m_sensorHandle, m_componentHandle, property, details::PropertyType<T>::type(), array, &result.second);
            return result;
        }

        std::pair<ZenError, bool> getBoolProperty(ZenProperty_t property)
        {
            auto result = std::make_pair(ZenError_None, false);
            result.first = ZenSensorComponentGetBoolProperty(m_clientHandle, m_sensorHandle, m_componentHandle, property, &result.second);
            return result;
        }

        std::pair<ZenError, float> getFloatProperty(ZenProperty_t property)
        {
            auto result = std::make_pair(ZenError_None, 0.f);
            result.first = ZenSensorComponentGetFloatProperty(m_clientHandle, m_sensorHandle, m_componentHandle, property, &result.second);
            return result;
        }

        std::pair<ZenError, int32_t> getInt32Property(ZenProperty_t property)
        {
            auto result = std::make_pair(ZenError_None, 0);
            result.first = ZenSensorComponentGetInt32Property(m_clientHandle, m_sensorHandle, m_componentHandle, property, &result.second);
            return result;
        }

        std::pair<ZenError, ZenMatrix3x3f> getMatrix33Property(ZenProperty_t property)
        {
            auto result = std::make_pair(ZenError_None, ZenMatrix3x3f{});
            result.first = ZenSensorComponentGetMatrix33Property(m_clientHandle, m_sensorHandle, m_componentHandle, property, &result.second);
            return result;
        }

        std::pair<ZenError, size_t> getStringProperty(ZenProperty_t property, char* const buffer, size_t length)
        {
            auto result = std::make_pair(ZenError_None, length);
            result.first = ZenSensorComponentGetStringProperty(m_clientHandle, m_sensorHandle, m_componentHandle, property, buffer, &result.second);
            return result;
        }

        std::pair<ZenError, uint64_t> getUInt64Property(ZenProperty_t property)
        {
            auto result = std::make_pair(ZenError_None, 0ull);
            result.first = ZenSensorComponentGetUInt64Property(m_clientHandle, m_sensorHandle, m_componentHandle, property, &result.second);
            return result;
        }

        template <typename T>
        ZenError setArrayProperty(ZenProperty_t property, const T* array, size_t length)
        {
            return ZenSensorComponentSetArrayProperty(m_clientHandle, m_sensorHandle, m_componentHandle, property, details::PropertyType<T>::type(), array, length);
        }

        ZenError setBoolProperty(ZenProperty_t property, bool value)
        {
            return ZenSensorComponentSetBoolProperty(m_clientHandle, m_sensorHandle, m_componentHandle, property, value);
        }

        ZenError setFloatProperty(ZenProperty_t property, float value)
        {
            return ZenSensorComponentSetFloatProperty(m_clientHandle, m_sensorHandle, m_componentHandle, property, value);
        }

        ZenError setInt32Property(ZenProperty_t property, int32_t value)
        {
            return ZenSensorComponentSetInt32Property(m_clientHandle, m_sensorHandle, m_componentHandle, property, value);
        }

        ZenError setMatrix33Property(ZenProperty_t property, const ZenMatrix3x3f& value)
        {
            return ZenSensorComponentSetMatrix33Property(m_clientHandle, m_sensorHandle, m_componentHandle, property, &value);
        }

        ZenError setStringProperty(ZenProperty_t property, std::string_view value)
        {
            return ZenSensorComponentSetStringProperty(m_clientHandle, m_sensorHandle, m_componentHandle, property, value.data(), value.size());
        }

        ZenError setUInt64Property(ZenProperty_t property, uint64_t value)
        {
            return ZenSensorComponentSetUInt64Property(m_clientHandle, m_sensorHandle, m_componentHandle, property, value);
        }
    };

    class ZenSensor
    {
    private:
        ZenClientHandle_t m_clientHandle;
        ZenSensorHandle_t m_sensorHandle;

    public:
        friend class ZenClient;

        ZenSensor(ZenClientHandle_t clientHandle, ZenSensorHandle_t sensorHandle)
            : m_clientHandle(clientHandle)
            , m_sensorHandle(sensorHandle)
        {}

        ZenSensor(ZenSensor&& other)
            : m_clientHandle(other.m_clientHandle)
            , m_sensorHandle(other.m_sensorHandle)
        {
            other.m_sensorHandle.handle = 0;
        }

        ~ZenSensor()
        {
            ZenReleaseSensor(m_clientHandle, m_sensorHandle);
        }

        ZenAsyncStatus updateFirmwareAsync(const std::vector<unsigned char>& firmware)
        {
            return ZenSensorUpdateFirmwareAsync(m_clientHandle, m_sensorHandle, firmware.data(), firmware.size());
        }

        ZenAsyncStatus updateIAPAsync(const std::vector<unsigned char>& iap)
        {
            return ZenSensorUpdateIAPAsync(m_clientHandle, m_sensorHandle, iap.data(), iap.size());
        }

        std::string_view ioType() const noexcept
        {
            return ZenSensorIoType(m_clientHandle, m_sensorHandle);
        }

        bool equals(const ZenSensorDesc& desc) const noexcept
        {
            return ZenSensorEquals(m_clientHandle, m_sensorHandle, &desc);
        }

        ZenError executeProperty(ZenProperty_t property)
        {
            return ZenSensorExecuteProperty(m_clientHandle, m_sensorHandle, property);
        }

        template <typename T>
        std::pair<ZenError, size_t> getArrayProperty(ZenProperty_t property, T* const array, size_t length)
        {
            auto result = std::make_pair(ZenError_None, length);
            result.first = ZenSensorGetArrayProperty(m_clientHandle, m_sensorHandle, property, details::PropertyType<T>::type(), array, &result.second);
            return result;
        }

        std::pair<ZenError, bool> getBoolProperty(ZenProperty_t property)
        {
            auto result = std::make_pair(ZenError_None, false);
            result.first = ZenSensorGetBoolProperty(m_clientHandle, m_sensorHandle, property, &result.second);
            return result;
        }

        std::pair<ZenError, float> getFloatProperty(ZenProperty_t property)
        {
            auto result = std::make_pair(ZenError_None, 0.f);
            result.first = ZenSensorGetFloatProperty(m_clientHandle, m_sensorHandle, property, &result.second);
            return result;
        }

        std::pair<ZenError, int32_t> getInt32Property(ZenProperty_t property)
        {
            auto result = std::make_pair(ZenError_None, 0);
            result.first = ZenSensorGetInt32Property(m_clientHandle, m_sensorHandle, property, &result.second);
            return result;
        }

        std::pair<ZenError, ZenMatrix3x3f> getMatrix33Property(ZenProperty_t property)
        {
            auto result = std::make_pair(ZenError_None, ZenMatrix3x3f{});
            result.first = ZenSensorGetMatrix33Property(m_clientHandle, m_sensorHandle, property, &result.second);
            return result;
        }

        std::pair<ZenError, size_t> getStringProperty(ZenProperty_t property, char* const buffer, size_t length)
        {
            auto result = std::make_pair(ZenError_None, length);
            result.first = ZenSensorGetStringProperty(m_clientHandle, m_sensorHandle, property, buffer, &result.second);
            return result;
        }

        std::pair<ZenError, uint64_t> getUInt64Property(ZenProperty_t property)
        {
            auto result = std::make_pair(ZenError_None, 0ull);
            result.first = ZenSensorGetUInt64Property(m_clientHandle, m_sensorHandle, property, &result.second);
            return result;
        }

        template <typename T>
        ZenError setArrayProperty(ZenProperty_t property, const T* array, size_t length)
        {
            return ZenSensorSetArrayProperty(m_clientHandle, m_sensorHandle, property, details::PropertyType<T>::type(), array, length);
        }

        ZenError setBoolProperty(ZenProperty_t property, bool value)
        {
            return ZenSensorSetBoolProperty(m_clientHandle, m_sensorHandle, property, value);
        }

        ZenError setFloatProperty(ZenProperty_t property, float value)
        {
            return ZenSensorSetFloatProperty(m_clientHandle, m_sensorHandle, property, value);
        }

        ZenError setInt32Property(ZenProperty_t property, int32_t value)
        {
            return ZenSensorSetInt32Property(m_clientHandle, m_sensorHandle, property, value);
        }

        ZenError setMatrix33Property(ZenProperty_t property, const ZenMatrix3x3f& value)
        {
            return ZenSensorSetMatrix33Property(m_clientHandle, m_sensorHandle, property, &value);
        }

        ZenError setStringProperty(ZenProperty_t property, std::string_view value)
        {
            return ZenSensorSetStringProperty(m_clientHandle, m_sensorHandle, property, value.data(), value.size());
        }

        ZenError setUInt64Property(ZenProperty_t property, uint64_t value)
        {
            return ZenSensorSetUInt64Property(m_clientHandle, m_sensorHandle, property, value);
        }

        std::optional<ZenSensorComponent> getAnyComponentOfType(std::string_view type)
        {
            ZenComponentHandle_t* handles = nullptr;
            size_t nComponents;
            if (auto error = ZenSensorComponents(m_clientHandle, m_sensorHandle, type.data(), &handles, &nComponents))
                return std::nullopt;

            if (nComponents > 0)
                return ZenSensorComponent(m_clientHandle, m_sensorHandle, handles[0]);

            return std::nullopt;
        }

        // [TODO] Remove handle
        std::pair<ZenError, ZenSensorDesc> desc()
        {
            ZenSensorDesc desc;

            auto [nameError, nameLength] = getStringProperty(ZenSensorProperty_DeviceName, desc.name, sizeof(ZenSensorDesc::name));
            if (nameError)
                return std::make_pair(nameError, ZenSensorDesc{}); // [TODO] Specific error Sensor_NameTooLong

            desc.name[nameLength] = '\0';

            auto [serialError, serialLength] = getStringProperty(ZenSensorProperty_SerialNumber, desc.serialNumber, sizeof(ZenSensorDesc::serialNumber));
            if (serialError)
                return std::make_pair(serialError, ZenSensorDesc{}); // [TODO] Specific error Sensor_SerialNumberTooLong

            desc.serialNumber[serialLength] = '\0';

            const auto type = ioType();
            if (type.size() >= sizeof(ZenSensorDesc::ioType))
                return std::make_pair(ZenError_BufferTooSmall, ZenSensorDesc{}); // [TODO] Specific error Sensor_IoTypeTooLong

            std::memcpy(desc.ioType, type.data(), type.size());
            desc.ioType[type.size()] = '\0';
            desc.handle64 = 0;
            return std::make_pair(ZenError_None, std::move(desc));
        }
    };

    class ZenClient
    {
    private:
        ZenClientHandle_t m_handle;

    public:
        ZenClient(ZenClientHandle_t handle) noexcept
            : m_handle(handle)
        {}

        ZenClient(ZenClient&& other) noexcept
            : m_handle(other.m_handle)
        {
            other.m_handle.handle = 0;
        }

        ~ZenClient() noexcept
        {
            close();
        }

        ZenError close() noexcept
        {
            return ZenShutdown(m_handle);
        }

        ZenError listSensorsAsync() noexcept
        {
            return ZenListSensorsAsync(m_handle);
        }

        std::pair<ZenSensorInitError, ZenSensor> obtainSensor(const ZenSensorDesc& desc) noexcept
        {
            ZenSensorHandle_t sensorHandle;
            const auto error = ZenObtainSensor(m_handle, &desc, &sensorHandle);
            return std::make_pair(error, ZenSensor(m_handle, sensorHandle));
        }

        ZenError releaseSensor(ZenSensor& sensor) noexcept
        {
            if (auto error = ZenReleaseSensor(m_handle, sensor.m_sensorHandle))
                return error;

            sensor.m_sensorHandle.handle = 0;
            return ZenError_None;
        }

        std::optional<ZenEvent> pollNextEvent() noexcept
        {
            ZenEvent event;
            if (ZenPollNextEvent(m_handle, &event))
                return std::move(event);

            return std::nullopt;
        }

        std::optional<ZenEvent> waitForNextEvent() noexcept
        {
            ZenEvent event;
            if (ZenWaitForNextEvent(m_handle, &event))
                return std::move(event);

            return std::nullopt;
        }
    };

    inline std::pair<ZenError, ZenClient> make_client() noexcept
    {
        ZenClientHandle_t handle;
        const auto error = ZenInit(&handle);
        return std::make_pair(error, ZenClient(handle));
    }
}

#endif
