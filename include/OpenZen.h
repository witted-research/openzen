#ifndef ZEN_API_OPENZEN_H_
#define ZEN_API_OPENZEN_H_

/**
This is the C++14 API to the OpenZen library. It is a header-only wrapper around
the C-API.

Use the zen::make_client function to create a ZenClient object which you can then
use to list all available sensors with the ZenClient::listSensorsAsync() method. With all
available sensors, you can then use the ZenClient::obtainSensor() method to
connect to a sensor and receive its measurement data.

You can use the waitForNextEvent() and pollNextEvent() of the ZenClient class to get
ZenEvents about sensor discovery results and incoming measurement data.
*/

#include "OpenZenCAPI.h"

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstring>
#include <thread>
#include <utility>
#include <vector>

#if __cplusplus >= 201703L
#include <optional>
#include <string_view>
#endif

namespace details
{
    template <typename T>
    struct PropertyType
    {};

#if __cplusplus >= 201703L
    template <> struct PropertyType<std::byte>
#else
    template <> struct PropertyType<unsigned char>
#endif
    {
        using type = std::integral_constant<ZenPropertyType, ZenPropertyType_Byte>;
    };

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

    /**
    A sensor component represents one measurement data source on a sensor, for example an
    inertial measurement unit (IMU) or GPS receiver.
    */
    class ZenSensorComponent
    {
    private:
        ZenClientHandle_t m_clientHandle;
        ZenSensorHandle_t m_sensorHandle;
        ZenComponentHandle_t m_componentHandle;

    public:
        ZenSensorComponent(ZenClientHandle_t clientHandle, ZenSensorHandle_t sensorHandle, ZenComponentHandle_t componentHandle) noexcept
            : m_clientHandle(clientHandle)
            , m_sensorHandle(sensorHandle)
            , m_componentHandle(componentHandle)
        {}

        ZenSensorComponent(const ZenSensorComponent& other) noexcept
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

#if __cplusplus >= 201703L
        std::string_view type() const noexcept
#else
        const char* type() const noexcept
#endif
        {
            return ZenSensorComponentType(m_clientHandle, m_sensorHandle, m_componentHandle);
        }

        ZenError executeProperty(ZenProperty_t property) noexcept
        {
            return ZenSensorComponentExecuteProperty(m_clientHandle, m_sensorHandle, m_componentHandle, property);
        }

        template <typename T>
        std::pair<ZenError, size_t> getArrayProperty(ZenProperty_t property, T* const array, size_t length) noexcept
        {
            auto result = std::make_pair(ZenError_None, length);
            result.first = ZenSensorComponentGetArrayProperty(m_clientHandle, m_sensorHandle, m_componentHandle, property, details::PropertyType<T>::type::value, array, &result.second);
            return result;
        }

        std::pair<ZenError, bool> getBoolProperty(ZenProperty_t property) noexcept
        {
            auto result = std::make_pair(ZenError_None, false);
            result.first = ZenSensorComponentGetBoolProperty(m_clientHandle, m_sensorHandle, m_componentHandle, property, &result.second);
            return result;
        }

        std::pair<ZenError, float> getFloatProperty(ZenProperty_t property) noexcept
        {
            auto result = std::make_pair(ZenError_None, 0.f);
            result.first = ZenSensorComponentGetFloatProperty(m_clientHandle, m_sensorHandle, m_componentHandle, property, &result.second);
            return result;
        }

        std::pair<ZenError, int32_t> getInt32Property(ZenProperty_t property) noexcept
        {
            auto result = std::make_pair(ZenError_None, 0);
            result.first = ZenSensorComponentGetInt32Property(m_clientHandle, m_sensorHandle, m_componentHandle, property, &result.second);
            return result;
        }

        std::pair<ZenError, uint64_t> getUInt64Property(ZenProperty_t property) noexcept
        {
            auto result = std::make_pair(ZenError_None, uint64_t(0));
            result.first = ZenSensorComponentGetUInt64Property(m_clientHandle, m_sensorHandle, m_componentHandle, property, &result.second);
            return result;
        }

        template <typename T>
        ZenError setArrayProperty(ZenProperty_t property, const T* array, size_t length) noexcept
        {
            return ZenSensorComponentSetArrayProperty(m_clientHandle, m_sensorHandle, m_componentHandle, property, details::PropertyType<T>::type::value, array, length);
        }

        ZenError setBoolProperty(ZenProperty_t property, bool value) noexcept
        {
            return ZenSensorComponentSetBoolProperty(m_clientHandle, m_sensorHandle, m_componentHandle, property, value);
        }

        ZenError setFloatProperty(ZenProperty_t property, float value) noexcept
        {
            return ZenSensorComponentSetFloatProperty(m_clientHandle, m_sensorHandle, m_componentHandle, property, value);
        }

        ZenError setInt32Property(ZenProperty_t property, int32_t value) noexcept
        {
            return ZenSensorComponentSetInt32Property(m_clientHandle, m_sensorHandle, m_componentHandle, property, value);
        }

        ZenError setUInt64Property(ZenProperty_t property, uint64_t value) noexcept
        {
            return ZenSensorComponentSetUInt64Property(m_clientHandle, m_sensorHandle, m_componentHandle, property, value);
        }
    };

    /**
    This class represents one sensor connected by OpenZen. One sensor can contain one or more
    components which deliver measurement data.
    */
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
            release();
        }

        ZenError release() noexcept
        {
            return ZenReleaseSensor(m_clientHandle, m_sensorHandle);
        }

        ZenAsyncStatus updateFirmwareAsync(const std::vector<unsigned char>& firmware) noexcept
        {
            return ZenSensorUpdateFirmwareAsync(m_clientHandle, m_sensorHandle, firmware.data(), firmware.size());
        }

        ZenAsyncStatus updateIAPAsync(const std::vector<unsigned char>& iap) noexcept
        {
            return ZenSensorUpdateIAPAsync(m_clientHandle, m_sensorHandle, iap.data(), iap.size());
        }

#if __cplusplus >= 201703L
        std::string_view ioType() const noexcept
#else
        const char* ioType() const noexcept
#endif
        {
            return ZenSensorIoType(m_clientHandle, m_sensorHandle);
        }

        bool equals(const ZenSensorDesc& desc) const noexcept
        {
            return ZenSensorEquals(m_clientHandle, m_sensorHandle, &desc);
        }

        ZenError executeProperty(ZenProperty_t property) noexcept
        {
            return ZenSensorExecuteProperty(m_clientHandle, m_sensorHandle, property);
        }

        template <typename T>
        std::pair<ZenError, size_t> getArrayProperty(ZenProperty_t property, T* const array, size_t length) noexcept
        {
            auto result = std::make_pair(ZenError_None, length);
            result.first = ZenSensorGetArrayProperty(m_clientHandle, m_sensorHandle, property, details::PropertyType<T>::type::value, array, &result.second);
            return result;
        }

        std::pair<ZenError, bool> getBoolProperty(ZenProperty_t property) noexcept
        {
            auto result = std::make_pair(ZenError_None, false);
            result.first = ZenSensorGetBoolProperty(m_clientHandle, m_sensorHandle, property, &result.second);
            return result;
        }

        std::pair<ZenError, float> getFloatProperty(ZenProperty_t property) noexcept
        {
            auto result = std::make_pair(ZenError_None, 0.f);
            result.first = ZenSensorGetFloatProperty(m_clientHandle, m_sensorHandle, property, &result.second);
            return result;
        }

        std::pair<ZenError, int32_t> getInt32Property(ZenProperty_t property) noexcept
        {
            auto result = std::make_pair(ZenError_None, 0);
            result.first = ZenSensorGetInt32Property(m_clientHandle, m_sensorHandle, property, &result.second);
            return result;
        }

        std::pair<ZenError, uint64_t> getUInt64Property(ZenProperty_t property) noexcept
        {
            auto result = std::make_pair(ZenError_None, uint64_t(0));
            result.first = ZenSensorGetUInt64Property(m_clientHandle, m_sensorHandle, property, &result.second);
            return result;
        }

        template <typename T>
        ZenError setArrayProperty(ZenProperty_t property, const T* array, size_t length) noexcept
        {
            return ZenSensorSetArrayProperty(m_clientHandle, m_sensorHandle, property, details::PropertyType<T>::type::value, array, length);
        }

        ZenError setBoolProperty(ZenProperty_t property, bool value) noexcept
        {
            return ZenSensorSetBoolProperty(m_clientHandle, m_sensorHandle, property, value);
        }

        ZenError setFloatProperty(ZenProperty_t property, float value) noexcept
        {
            return ZenSensorSetFloatProperty(m_clientHandle, m_sensorHandle, property, value);
        }

        ZenError setInt32Property(ZenProperty_t property, int32_t value) noexcept
        {
            return ZenSensorSetInt32Property(m_clientHandle, m_sensorHandle, property, value);
        }

        ZenError setUInt64Property(ZenProperty_t property, uint64_t value) noexcept
        {
            return ZenSensorSetUInt64Property(m_clientHandle, m_sensorHandle, property, value);
        }

#if __cplusplus >= 201703L
        std::optional<ZenSensorComponent> getAnyComponentOfType(std::string_view type) noexcept
        {
            ZenComponentHandle_t* handles = nullptr;
            size_t nComponents;
            if (auto error = ZenSensorComponents(m_clientHandle, m_sensorHandle, type.data(), &handles, &nComponents))
                return std::nullopt;

            if (nComponents == 0)
                return std::nullopt;

            return ZenSensorComponent(m_clientHandle, m_sensorHandle, handles[0]);
        }
#else
        std::pair<bool, ZenSensorComponent> getAnyComponentOfType(const char* type) noexcept
        {
            ZenComponentHandle_t* handles = nullptr;
            size_t nComponents;
            if (auto error = ZenSensorComponents(m_clientHandle, m_sensorHandle, type, &handles, &nComponents))
                return std::make_pair(false, ZenSensorComponent(m_clientHandle, m_sensorHandle, ZenComponentHandle_t{ 0 }));

            if (nComponents == 0)
                return std::make_pair(false, ZenSensorComponent(m_clientHandle, m_sensorHandle, ZenComponentHandle_t{ 0 }));

            return std::make_pair(true, ZenSensorComponent(m_clientHandle, m_sensorHandle, handles[0]));
        }
#endif
    };

    /**
    This class is the primary access point into the OpenZen library. Use the zen::make_client
    method to obtain an instance of this class.
    */
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

#if __cplusplus >= 201703L
        std::optional<ZenEvent> pollNextEvent() noexcept
#else
        std::pair<bool, ZenEvent> pollNextEvent() noexcept
#endif
        {
            ZenEvent event;
            if (ZenPollNextEvent(m_handle, &event))
            {
#if __cplusplus >= 201703L
                return std::move(event);
#else
                return std::make_pair(true, std::move(event));
#endif
            }

#if __cplusplus >= 201703L
            return std::nullopt;
#else
            return std::make_pair(false, std::move(event));
#endif
        }

#if __cplusplus >= 201703L
        std::optional<ZenEvent> waitForNextEvent() noexcept
#else
        std::pair<bool, ZenEvent> waitForNextEvent() noexcept
#endif
        {
            ZenEvent event;
            if (ZenWaitForNextEvent(m_handle, &event))
            {
#if __cplusplus >= 201703L
                return std::move(event);
#else
                return std::make_pair(true, std::move(event));
#endif
            }

#if __cplusplus >= 201703L
            return std::nullopt;
#else
            return std::make_pair(false, std::move(event));
#endif
        }
    };

    /**
    Use this function to create a ZenClient object. This instance of
    ZenClient can then be used to list all available sensors and connect
    to them.
    */
    inline std::pair<ZenError, ZenClient> make_client() noexcept
    {
        ZenClientHandle_t handle;
        const auto error = ZenInit(&handle);
        return std::make_pair(error, ZenClient(handle));
    }
}

#endif
