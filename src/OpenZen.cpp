//===========================================================================//
//
// Copyright (C) 2020 LP-Research Inc.
//
// This file is part of OpenZen, under the MIT License.
// See https://bitbucket.org/lpresearch/openzen/src/master/LICENSE for details
// SPDX-License-Identifier: MIT
//
//===========================================================================//

#include "OpenZenCAPI.h"

#include <memory>
#include <mutex>
#include <numeric>
#include <unordered_map>

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_sinks.h>

#include "SensorClient.h"
#include "components/GnssComponent.h"

namespace
{
    std::unordered_map<uintptr_t, std::shared_ptr<zen::SensorClient>> g_clients;
    std::mutex g_clientsMutex;
    uintptr_t g_nextClientToken = 1;

    std::shared_ptr<zen::SensorClient> getClient(ZenClientHandle_t handle) noexcept
    {
        std::lock_guard<std::mutex> lock(g_clientsMutex);
        auto it = g_clients.find(handle.handle);
        if (it == g_clients.end())
            return nullptr;

        return it->second;
    }

    zen::SensorComponent* getComponent(std::shared_ptr<zen::Sensor>& sensor, ZenComponentHandle_t handle) noexcept
    {
        const size_t idx = handle.handle - 1;
        const auto& components = sensor->components();
        if (idx >= components.size())
            return nullptr;

        return components[idx].get();
    }
    size_t countComponentsOfType(const std::vector<std::unique_ptr<zen::SensorComponent>>& components, std::string_view type)
    {
        return std::accumulate(components.cbegin(), components.cend(), static_cast<size_t>(0), [=](size_t count, const auto& component) {
            return component->type() == type ? count + 1 : count;
        });
    }
}

ZEN_API ZenError ZenInit(ZenClientHandle_t* outHandle)
{
    if (outHandle == nullptr)
        return ZenError_IsNull;

    std::lock_guard<std::mutex> lock(g_clientsMutex);
    const auto token = g_nextClientToken++;
    g_clients.emplace(token, std::make_shared<zen::SensorClient>(token));
    outHandle->handle = token;

    return ZenError_None;
}
ZEN_API ZenError ZenShutdown(ZenClientHandle_t handle)
{
    std::lock_guard<std::mutex> lock(g_clientsMutex);
    auto it = g_clients.find(handle.handle);
    if (it == g_clients.end())
        return ZenError_InvalidClientHandle;

    g_clients.erase(it);
    return ZenError_None;
}

ZEN_API ZenError ZenSetLogLevel(ZenLogLevel logLevel) {
    const std::string loggerName = "OpenZen_console";
    if (!spdlog::get(loggerName)) {
        auto console_logger = spdlog::stdout_logger_mt(loggerName);
        spdlog::set_default_logger(console_logger);
    }

    if (logLevel == ZenLogLevel::ZenLogLevel_Off) {
        spdlog::set_level(spdlog::level::off);
    }
    else if (logLevel == ZenLogLevel::ZenLogLevel_Debug) {
        spdlog::set_level(spdlog::level::debug);
    }
    else if (logLevel == ZenLogLevel::ZenLogLevel_Info) {
        spdlog::set_level(spdlog::level::info);
    }
    else if (logLevel == ZenLogLevel::ZenLogLevel_Warning) {
        spdlog::set_level(spdlog::level::warn);
    }
    else if (logLevel == ZenLogLevel::ZenLogLevel_Error) {
        spdlog::set_level(spdlog::level::err);
    } else {
        spdlog::error("Log Level {} not suppored", logLevel);
        return ZenError_InvalidArgument;
    }

    return ZenError_None;
}

ZEN_API ZenError ZenListSensorsAsync(ZenClientHandle_t handle)
{
    if (auto client = getClient(handle))
    {
        client->listSensorsAsync();
        return ZenError_None;
    }
    else
    {
        return ZenError_InvalidClientHandle;
    }
}

ZEN_API ZenSensorInitError ZenObtainSensor(ZenClientHandle_t clientHandle, const ZenSensorDesc* const desc, ZenSensorHandle_t* outSensorHandle)
{
    if (desc == nullptr)
        return ZenSensorInitError_IsNull;

    if (outSensorHandle == nullptr)
        return ZenSensorInitError_IsNull;

    if (auto client = getClient(clientHandle))
    {
        if (auto sensor = client->obtain(*desc))
        {
            outSensorHandle->handle = sensor.value()->token();
            return ZenSensorInitError_None;
        }
        else
        {
            return sensor.error();
        }
    }
    else
    {
        return ZenSensorInitError_InvalidHandle;
    }
}

ZEN_API ZenSensorInitError ZenObtainSensorByName(ZenClientHandle_t clientHandle,
    const char * ioType,
    const char * sensorIdentifier,
    uint32_t baudRate,
    ZenSensorHandle_t* outSensorHandle) {

    if (outSensorHandle == nullptr)
        return ZenSensorInitError_IsNull;

    if (auto client = getClient(clientHandle))
    {
        if (auto sensor = client->obtain(ioType, sensorIdentifier, baudRate))
        {
            outSensorHandle->handle = sensor.value()->token();
            return ZenSensorInitError_None;
        }
        else
        {
            return sensor.error();
        }
    }
    else
    {
        return ZenSensorInitError_InvalidHandle;
    }
}

ZEN_API ZenError ZenReleaseSensor(ZenClientHandle_t clientHandle, ZenSensorHandle_t sensorHandle)
{
    if (auto client = getClient(clientHandle))
    {
        if (auto sensor = client->findSensor(sensorHandle))
            return client->release(sensor);
        else
            return ZenError_InvalidSensorHandle;
    }
    else
    {
        return ZenError_InvalidClientHandle;
    }
}

ZEN_API bool ZenPollNextEvent(ZenClientHandle_t handle, ZenEvent* const outEvent)
{
    if (outEvent == nullptr)
        return false;

    if (auto client = getClient(handle))
    {
        if (auto event = client->pollNextEvent())
        {
            *outEvent = std::move(*event);
            return true;
        }
        else
        {
            return false;
        }
    }
    else
    {
        return false;
    }
}

ZEN_API bool ZenWaitForNextEvent(ZenClientHandle_t handle, ZenEvent* const outEvent)
{
    if (outEvent == nullptr)
        return false;

    if (auto client = getClient(handle))
    {
        // Prevent the thread from holding on to the client resource.
        // The SensorClient destructor guarantees that waiting threads are released before the resource is destroyed
        auto& clientRef = *client.get();
        client.reset();

        if (auto event = clientRef.waitForNextEvent())
        {
            *outEvent = std::move(*event);
            return true;
        }
        else
        {
            return false;
        }
    }
    else
    {
        return false;
    }
}

ZEN_API ZenError ZenSensorComponents(ZenClientHandle_t clientHandle, ZenSensorHandle_t sensorHandle, const char* const type, ZenComponentHandle_t** outComponentHandles, size_t* const outLength)
{
    if (outLength == nullptr)
        return ZenError_IsNull;

    if (auto client = getClient(clientHandle))
    {
        if (auto sensor = client->findSensor(sensorHandle))
        {
            const auto& components = sensor->components();
            const size_t length = !type ? components.size() : countComponentsOfType(components, type);

            *outLength = length;
            if (!components.empty())
            {
                if (outComponentHandles == nullptr)
                    return ZenError_IsNull;

                ZenComponentHandle_t* componentHandles = new ZenComponentHandle_t[length];
                size_t idx = 0;
                for (size_t componentIdx = 0; componentIdx < components.size(); ++componentIdx)
                    if (!type || components[componentIdx]->type() == std::string_view(type))
                        componentHandles[idx++].handle = componentIdx + 1;

                *outComponentHandles = componentHandles;
            }

            return ZenError_None;
        }
        else
        {
            return ZenError_InvalidSensorHandle;
        }
    }
    else
    {
        return ZenError_InvalidClientHandle;
    }
}

ZEN_API ZenError ZenSensorComponentsByNumber(ZenClientHandle_t clientHandle, ZenSensorHandle_t sensorHandle, const char* const type, size_t number, ZenComponentHandle_t* outComponentHandle)
{
    ZenComponentHandle_t* handles = nullptr;
    size_t nComponents;
    if (auto error = ZenSensorComponents(clientHandle, sensorHandle, type, &handles, &nComponents)) {
        outComponentHandle->handle = 0;
        return error;
    }

    if (nComponents <= number) {
        outComponentHandle->handle = 0;
        return ZenError_InvalidArgument;
    }

    *outComponentHandle = handles[number];
    return ZenError_None;
}

ZEN_API const char* ZenSensorIoType(ZenClientHandle_t clientHandle, ZenSensorHandle_t sensorHandle)
{
    if (auto client = getClient(clientHandle))
    {
        if (auto sensor = client->findSensor(sensorHandle))
            return sensor->ioType().data();
        else
            return nullptr;
    }
    else
    {
        return nullptr;
    }
}

ZEN_API bool ZenSensorEquals(ZenClientHandle_t clientHandle, ZenSensorHandle_t sensorHandle, const ZenSensorDesc* const desc)
{
    if (desc == nullptr)
        return false;

    if (auto client = getClient(clientHandle))
    {
        if (auto sensor = client->findSensor(sensorHandle))
            return sensor->equals(*desc);
        else
            return false;
    }
    else
    {
        return false;
    }
}

ZEN_API ZenAsyncStatus ZenSensorUpdateFirmwareAsync(ZenClientHandle_t clientHandle, ZenSensorHandle_t sensorHandle, const unsigned char* const buffer, size_t bufferSize)
{
    if (auto client = getClient(clientHandle))
    {
        if (auto sensor = client->findSensor(sensorHandle))
            return sensor->updateFirmwareAsync(gsl::make_span(reinterpret_cast<const std::byte*>(buffer), bufferSize));
        else
            return ZenAsync_InvalidArgument;
    }
    else
    {
        return ZenAsync_InvalidArgument;
    }
}

ZEN_API ZenAsyncStatus ZenSensorUpdateIAPAsync(ZenClientHandle_t clientHandle, ZenSensorHandle_t sensorHandle, const unsigned char* const buffer, size_t bufferSize)
{
    if (auto client = getClient(clientHandle))
    {
        if (auto sensor = client->findSensor(sensorHandle))
            return sensor->updateIAPAsync(gsl::make_span(reinterpret_cast<const std::byte*>(buffer), bufferSize));
        else
            return ZenAsync_InvalidArgument;
    }
    else
    {
        return ZenAsync_InvalidArgument;
    }
}

ZEN_API ZenError ZenPublishEvents(ZenClientHandle_t clientHandle, ZenSensorHandle_t sensorHandle, const char* endpoint) {
    if (auto client = getClient(clientHandle))
    {
        if (auto sensor = client->findSensor(sensorHandle))
            return client->publishEvents(sensor, endpoint);
        else
            return ZenError_InvalidSensorHandle;
    }
    else
    {
        return ZenError_InvalidClientHandle;
    }
}

ZEN_API ZenError ZenSensorExecuteProperty(ZenClientHandle_t clientHandle, ZenSensorHandle_t sensorHandle, ZenProperty_t property)
{
    if (auto client = getClient(clientHandle))
    {
        if (auto sensor = client->findSensor(sensorHandle))
            return sensor->properties()->execute(property);
        else
            return ZenError_InvalidSensorHandle;
    }
    else
    {
        return ZenError_InvalidClientHandle;
    }
}

ZEN_API ZenError ZenSensorGetArrayProperty(ZenClientHandle_t clientHandle, ZenSensorHandle_t sensorHandle, ZenProperty_t property,
    ZenPropertyType type, void* const buffer, size_t* bufferSize)
{
    if (bufferSize == nullptr)
        return ZenError_IsNull;

    if (auto client = getClient(clientHandle))
    {
        if (auto sensor = client->findSensor(sensorHandle))
        {
            const auto[error, size] = sensor->properties()->getArray(property, type, gsl::make_span(reinterpret_cast<std::byte*>(buffer), *bufferSize));
            *bufferSize = size;
            return error;
        }
        else
        {
            return ZenError_InvalidSensorHandle;
        }
    }
    else
    {
        return ZenError_InvalidClientHandle;
    }
}

ZEN_API ZenError ZenSensorGetBoolProperty(ZenClientHandle_t clientHandle, ZenSensorHandle_t sensorHandle, ZenProperty_t property, bool* const outValue)
{
    if (outValue == nullptr)
        return ZenError_IsNull;

    if (auto client = getClient(clientHandle))
    {
        if (auto sensor = client->findSensor(sensorHandle))
        {
            if (auto result = sensor->properties()->getBool(property))
            {
                *outValue = *result;
                return ZenError_None;
            }
            else
            {
                return result.error();
            }
        }
        else
        {
            return ZenError_InvalidSensorHandle;
        }
    }
    else
    {
        return ZenError_InvalidClientHandle;
    }
}

ZEN_API ZenError ZenSensorGetFloatProperty(ZenClientHandle_t clientHandle, ZenSensorHandle_t sensorHandle, ZenProperty_t property, float* const outValue)
{
    if (outValue == nullptr)
        return ZenError_IsNull;

    if (auto client = getClient(clientHandle))
    {
        if (auto sensor = client->findSensor(sensorHandle))
        {
            if (auto result = sensor->properties()->getFloat(property))
            {
                *outValue = *result;
                return ZenError_None;
            }
            else
            {
                return result.error();
            }
        }
        else
        {
            return ZenError_InvalidSensorHandle;
        }
    }
    else
    {
        return ZenError_InvalidClientHandle;
    }
}

ZEN_API ZenError ZenSensorGetInt32Property(ZenClientHandle_t clientHandle, ZenSensorHandle_t sensorHandle, ZenProperty_t property, int32_t* const outValue)
{
    if (outValue == nullptr)
        return ZenError_IsNull;

    if (auto client = getClient(clientHandle))
    {
        if (auto sensor = client->findSensor(sensorHandle))
        {
            if (auto result = sensor->properties()->getInt32(property))
            {
                *outValue = *result;
                return ZenError_None;
            }
            else
            {
                return result.error();
            }
        }
        else
        {
            return ZenError_InvalidSensorHandle;
        }
    }
    else
    {
        return ZenError_InvalidClientHandle;
    }
}

ZEN_API ZenError ZenSensorGetUInt64Property(ZenClientHandle_t clientHandle, ZenSensorHandle_t sensorHandle, ZenProperty_t property, uint64_t* const outValue)
{
    if (outValue == nullptr)
        return ZenError_IsNull;

    if (auto client = getClient(clientHandle))
    {
        if (auto sensor = client->findSensor(sensorHandle))
        {
            if (auto result = sensor->properties()->getUInt64(property))
            {
                *outValue = *result;
                return ZenError_None;
            }
            else
            {
                return result.error();
            }
        }
        else
        {
            return ZenError_InvalidSensorHandle;
        }
    }
    else
    {
        return ZenError_InvalidClientHandle;
    }
}

ZEN_API ZenError ZenSensorSetArrayProperty(ZenClientHandle_t clientHandle, ZenSensorHandle_t sensorHandle, ZenProperty_t property, ZenPropertyType type, const void* buffer, size_t bufferSize)
{
    if (buffer == nullptr)
        return ZenError_IsNull;

    if (auto client = getClient(clientHandle))
    {
        if (auto sensor = client->findSensor(sensorHandle))
            return sensor->properties()->setArray(property, type, gsl::make_span(reinterpret_cast<const std::byte*>(buffer), bufferSize));
        else
            return ZenError_InvalidSensorHandle;
    }
    else
    {
        return ZenError_InvalidClientHandle;
    }
}

ZEN_API ZenError ZenSensorSetBoolProperty(ZenClientHandle_t clientHandle, ZenSensorHandle_t sensorHandle, ZenProperty_t property, bool value)
{
    if (auto client = getClient(clientHandle))
    {
        if (auto sensor = client->findSensor(sensorHandle))
            return sensor->properties()->setBool(property, value);
        else
            return ZenError_InvalidSensorHandle;
    }
    else
    {
        return ZenError_InvalidClientHandle;
    }
}

ZEN_API ZenError ZenSensorSetFloatProperty(ZenClientHandle_t clientHandle, ZenSensorHandle_t sensorHandle, ZenProperty_t property, float value)
{
    if (auto client = getClient(clientHandle))
    {
        if (auto sensor = client->findSensor(sensorHandle))
            return sensor->properties()->setFloat(property, value);
        else
            return ZenError_InvalidSensorHandle;
    }
    else
    {
        return ZenError_InvalidClientHandle;
    }
}

ZEN_API ZenError ZenSensorSetInt32Property(ZenClientHandle_t clientHandle, ZenSensorHandle_t sensorHandle, ZenProperty_t property, int32_t value)
{
    if (auto client = getClient(clientHandle))
    {
        if (auto sensor = client->findSensor(sensorHandle))
            return sensor->properties()->setInt32(property, value);
        else
            return ZenError_InvalidSensorHandle;
    }
    {
        return ZenError_InvalidClientHandle;
    }
}

ZEN_API ZenError ZenSensorSetUInt64Property(ZenClientHandle_t clientHandle, ZenSensorHandle_t sensorHandle, ZenProperty_t property, uint64_t value)
{
    if (auto client = getClient(clientHandle))
    {
        if (auto sensor = client->findSensor(sensorHandle))
            return sensor->properties()->setUInt64(property, value);
        else
            return ZenError_InvalidSensorHandle;
    }
    else
    {
        return ZenError_InvalidClientHandle;
    }
}

ZEN_API bool ZenSensorIsArrayProperty(ZenClientHandle_t clientHandle, ZenSensorHandle_t sensorHandle, ZenProperty_t property)
{
    if (auto client = getClient(clientHandle))
    {
        if (auto sensor = client->findSensor(sensorHandle))
            return sensor->properties()->isArray(property);
        else
            return false;
    }
    else
    {
        return false;
    }
}

ZEN_API bool ZenSensorIsConstantProperty(ZenClientHandle_t clientHandle, ZenSensorHandle_t sensorHandle, ZenProperty_t property)
{
    if (auto client = getClient(clientHandle))
    {
        if (auto sensor = client->findSensor(sensorHandle))
            return sensor->properties()->isConstant(property);
        else
            return false;
    }
    else
    {
        return false;
    }
}

ZEN_API bool ZenSensorIsExecutableProperty(ZenClientHandle_t clientHandle, ZenSensorHandle_t sensorHandle, ZenProperty_t property)
{
    if (auto client = getClient(clientHandle))
    {
        if (auto sensor = client->findSensor(sensorHandle))
            return sensor->properties()->isExecutable(property);
        else
            return false;
    }
    else
    {
        return false;
    }
}

ZEN_API ZenPropertyType ZenSensorPropertyType(ZenClientHandle_t clientHandle, ZenSensorHandle_t sensorHandle, ZenProperty_t property)
{
    if (auto client = getClient(clientHandle))
    {
        if (auto sensor = client->findSensor(sensorHandle))
            return sensor->properties()->type(property);
        else
            return ZenPropertyType_Invalid;
    }
    else
    {
        return ZenPropertyType_Invalid;
    }
}

ZEN_API const char* ZenSensorComponentType(ZenClientHandle_t clientHandle, ZenSensorHandle_t sensorHandle, ZenComponentHandle_t componentHandle)
{
    if (auto client = getClient(clientHandle))
    {
        if (auto sensor = client->findSensor(sensorHandle))
        {
            if (auto component = getComponent(sensor, componentHandle))
                return component->type().data();
            else
                return nullptr;
        }
        else
        {
            return nullptr;
        }
    }
    else
    {
        return nullptr;
    }
}

ZEN_API ZenError ZenSensorComponentExecuteProperty(ZenClientHandle_t clientHandle, ZenSensorHandle_t sensorHandle, ZenComponentHandle_t componentHandle, ZenProperty_t property)
{
    if (auto client = getClient(clientHandle))
    {
        if (auto sensor = client->findSensor(sensorHandle))
        {
            if (auto component = getComponent(sensor, componentHandle))
                return component->properties()->execute(property);
            else
                return ZenError_InvalidComponentHandle;
        }
        else
        {
            return ZenError_InvalidSensorHandle;
        }
    }
    else
    {
        return ZenError_InvalidClientHandle;
    }
}

ZEN_API ZenError ZenSensorComponentGetArrayProperty(ZenClientHandle_t clientHandle, ZenSensorHandle_t sensorHandle, ZenComponentHandle_t componentHandle, ZenProperty_t property, ZenPropertyType type, void* const buffer, size_t* bufferSize)
{
    if (bufferSize == nullptr)
        return ZenError_IsNull;

    if (auto client = getClient(clientHandle))
    {
        if (auto sensor = client->findSensor(sensorHandle))
        {
            if (auto component = getComponent(sensor, componentHandle))
            {
                const auto[error, size] = component->properties()->getArray(property, type, gsl::make_span(reinterpret_cast<std::byte*>(buffer), *bufferSize));
                *bufferSize = size;
                return error;
            }
            else
            {
                return ZenError_InvalidComponentHandle;
            }
        }
        else
        {
            return ZenError_InvalidSensorHandle;
        }
    }
    else
    {
        return ZenError_InvalidClientHandle;
    }
}

/** If successful fills the value with the property's boolean value, otherwise returns an error. */
ZEN_API ZenError ZenSensorComponentGetBoolProperty(ZenClientHandle_t clientHandle, ZenSensorHandle_t sensorHandle, ZenComponentHandle_t componentHandle, ZenProperty_t property, bool* const outValue)
{
    if (outValue == nullptr)
        return ZenError_IsNull;

    if (auto client = getClient(clientHandle))
    {
        if (auto sensor = client->findSensor(sensorHandle))
        {
            if (auto component = getComponent(sensor, componentHandle))
            {
                if (auto result = component->properties()->getBool(property))
                {
                    *outValue = *result;
                    return ZenError_None;
                }
                else
                {
                    return result.error();
                }
            }
            else
            {
                return ZenError_InvalidComponentHandle;
            }
        }
        else
        {
            return ZenError_InvalidSensorHandle;
        }
    }
    else
    {
        return ZenError_InvalidClientHandle;
    }
}

ZEN_API ZenError ZenSensorComponentGetFloatProperty(ZenClientHandle_t clientHandle, ZenSensorHandle_t sensorHandle, ZenComponentHandle_t componentHandle, ZenProperty_t property, float* const outValue)
{
    if (outValue == nullptr)
        return ZenError_IsNull;

    if (auto client = getClient(clientHandle))
    {
        if (auto sensor = client->findSensor(sensorHandle))
        {
            if (auto component = getComponent(sensor, componentHandle))
            {
                if (auto result = component->properties()->getFloat(property))
                {
                    *outValue = *result;
                    return ZenError_None;
                }
                else
                {
                    return result.error();
                }
            }
            else
            {
                return ZenError_InvalidComponentHandle;
            }
        }
        else
        {
            return ZenError_InvalidSensorHandle;
        }
    }
    else
    {
        return ZenError_InvalidClientHandle;
    }
}

ZEN_API ZenError ZenSensorComponentGetInt32Property(ZenClientHandle_t clientHandle, ZenSensorHandle_t sensorHandle, ZenComponentHandle_t componentHandle, ZenProperty_t property, int32_t* const outValue)
{
    if (outValue == nullptr)
        return ZenError_IsNull;

    if (auto client = getClient(clientHandle))
    {
        if (auto sensor = client->findSensor(sensorHandle))
        {
            if (auto component = getComponent(sensor, componentHandle))
            {
                if (auto result = component->properties()->getInt32(property))
                {
                    *outValue = *result;
                    return ZenError_None;
                }
                else
                {
                    return result.error();
                }
            }
            else
            {
                return ZenError_InvalidComponentHandle;
            }
        }
        else
        {
            return ZenError_InvalidSensorHandle;
        }
    }
    else
    {
        return ZenError_InvalidClientHandle;
    }
}

ZEN_API ZenError ZenSensorComponentGetUInt64Property(ZenClientHandle_t clientHandle, ZenSensorHandle_t sensorHandle, ZenComponentHandle_t componentHandle, ZenProperty_t property, uint64_t* const outValue)
{
    if (outValue == nullptr)
        return ZenError_IsNull;

    if (auto client = getClient(clientHandle))
    {
        if (auto sensor = client->findSensor(sensorHandle))
        {
            if (auto component = getComponent(sensor, componentHandle))
            {
                if (auto result = component->properties()->getUInt64(property))
                {
                    *outValue = *result;
                    return ZenError_None;
                }
                else
                {
                    return result.error();
                }
            }
            else
            {
                return ZenError_InvalidComponentHandle;
            }
        }
        else
        {
            return ZenError_InvalidSensorHandle;
        }
    }
    else
    {
        return ZenError_InvalidClientHandle;
    }
}

ZEN_API ZenError ZenSensorComponentSetArrayProperty(ZenClientHandle_t clientHandle, ZenSensorHandle_t sensorHandle, ZenComponentHandle_t componentHandle, ZenProperty_t property, ZenPropertyType type, const void* buffer, size_t bufferSize)
{
    if (buffer == nullptr)
        return ZenError_IsNull;

    if (auto client = getClient(clientHandle))
    {
        if (auto sensor = client->findSensor(sensorHandle))
        {
            if (auto component = getComponent(sensor, componentHandle))
                return component->properties()->setArray(property, type, gsl::make_span(reinterpret_cast<const std::byte*>(buffer), bufferSize));
            else
                return ZenError_InvalidComponentHandle;
        }
        else
        {
            return ZenError_InvalidSensorHandle;
        }
    }
    else
    {
        return ZenError_InvalidClientHandle;
    }
}

ZEN_API ZenError ZenSensorComponentSetBoolProperty(ZenClientHandle_t clientHandle, ZenSensorHandle_t sensorHandle, ZenComponentHandle_t componentHandle, ZenProperty_t property, bool value)
{
    if (auto client = getClient(clientHandle))
    {
        if (auto sensor = client->findSensor(sensorHandle))
        {
            if (auto component = getComponent(sensor, componentHandle))
                return component->properties()->setBool(property, value);
            else
                return ZenError_InvalidComponentHandle;
        }
        else
        {
            return ZenError_InvalidSensorHandle;
        }
    }
    else
    {
        return ZenError_InvalidClientHandle;
    }
}

ZEN_API ZenError ZenSensorComponentSetFloatProperty(ZenClientHandle_t clientHandle, ZenSensorHandle_t sensorHandle, ZenComponentHandle_t componentHandle, ZenProperty_t property, float value)
{
    if (auto client = getClient(clientHandle))
    {
        if (auto sensor = client->findSensor(sensorHandle))
        {
            if (auto component = getComponent(sensor, componentHandle))
                return component->properties()->setFloat(property, value);
            else
                return ZenError_InvalidComponentHandle;
        }
        else
        {
            return ZenError_InvalidSensorHandle;
        }
    }
    else
    {
        return ZenError_InvalidClientHandle;
    }
}

ZEN_API ZenError ZenSensorComponentSetInt32Property(ZenClientHandle_t clientHandle, ZenSensorHandle_t sensorHandle, ZenComponentHandle_t componentHandle, ZenProperty_t property, int32_t value)
{
    if (auto client = getClient(clientHandle))
    {
        if (auto sensor = client->findSensor(sensorHandle))
        {
            if (auto component = getComponent(sensor, componentHandle))
                return component->properties()->setInt32(property, value);
            else
                return ZenError_InvalidComponentHandle;
        }
        else
        {
            return ZenError_InvalidSensorHandle;
        }
    }
    else
    {
        return ZenError_InvalidClientHandle;
    }
}

ZEN_API ZenError ZenSensorComponentSetUInt64Property(ZenClientHandle_t clientHandle, ZenSensorHandle_t sensorHandle, ZenComponentHandle_t componentHandle, ZenProperty_t property, uint64_t value)
{
    if (auto client = getClient(clientHandle))
    {
        if (auto sensor = client->findSensor(sensorHandle))
        {
            if (auto component = getComponent(sensor, componentHandle))
                return component->properties()->setUInt64(property, value);
            else
                return ZenError_InvalidComponentHandle;
        }
        else
        {
            return ZenError_InvalidSensorHandle;
        }
    }
    else
    {
        return ZenError_InvalidClientHandle;
    }
}

ZEN_API bool ZenSensorComponentIsArrayProperty(ZenClientHandle_t clientHandle, ZenSensorHandle_t sensorHandle, ZenComponentHandle_t componentHandle, ZenProperty_t property)
{
    if (auto client = getClient(clientHandle))
    {
        if (auto sensor = client->findSensor(sensorHandle))
        {
            if (auto component = getComponent(sensor, componentHandle))
                return component->properties()->isArray(property);
            else
                return false;
        }
        else
        {
            return false;
        }
    }
    else
    {
        return false;
    }
}

ZEN_API bool ZenSensorComponentIsConstantProperty(ZenClientHandle_t clientHandle, ZenSensorHandle_t sensorHandle, ZenComponentHandle_t componentHandle, ZenProperty_t property)
{
    if (auto client = getClient(clientHandle))
    {
        if (auto sensor = client->findSensor(sensorHandle))
        {
            if (auto component = getComponent(sensor, componentHandle))
                return component->properties()->isConstant(property);
            else
                return false;
        }
        else
        {
            return false;
        }
    }
    else
    {
        return false;
    }
}

ZEN_API bool ZenSensorComponentIsExecutableProperty(ZenClientHandle_t clientHandle, ZenSensorHandle_t sensorHandle, ZenComponentHandle_t componentHandle, ZenProperty_t property)
{
    if (auto client = getClient(clientHandle))
    {
        if (auto sensor = client->findSensor(sensorHandle))
        {
            if (auto component = getComponent(sensor, componentHandle))
                return component->properties()->isExecutable(property);
            else
                return false;
        }
        else
        {
            return false;
        }
    }
    else
    {
        return false;
    }
}

ZEN_API ZenPropertyType ZenSensorComponentPropertyType(ZenClientHandle_t clientHandle, ZenSensorHandle_t sensorHandle, ZenComponentHandle_t componentHandle, ZenProperty_t property)
{
    if (auto client = getClient(clientHandle))
    {
        if (auto sensor = client->findSensor(sensorHandle))
        {
            if (auto component = getComponent(sensor, componentHandle))
                return component->properties()->type(property);
            else
                return ZenPropertyType_Invalid;
        }
        else
        {
            return ZenPropertyType_Invalid;
        }
    }
    else
    {
        return ZenPropertyType_Invalid;
    }
}


ZEN_API ZenError ZenSensorComponentGnnsForwardRtkCorrections(ZenClientHandle_t clientHandle, ZenSensorHandle_t sensorHandle, ZenComponentHandle_t componentHandle,
    const char* const rtkCorrectionSource,
    const char* const hostname,
    uint32_t port)
{
    if (auto client = getClient(clientHandle))
    {
        if (auto sensor = client->findSensor(sensorHandle))
        {
            if (auto component = getComponent(sensor, componentHandle))
            {
                auto gnssComponent = static_cast<zen::GnssComponent *>(component);
                if (strcmp(rtkCorrectionSource, "RTCM3Network") == 0) {
                    return gnssComponent->forwardRtkCorrections(zen::RtkCorrectionSource::RTCM3NetworkStream,
                        hostname, port);
                }
                else if (strcmp(rtkCorrectionSource, "RTCM3Serial") == 0) {
                    return gnssComponent->forwardRtkCorrections(zen::RtkCorrectionSource::RTCM3SerialStream,
                        // in this case port name and baud rate
                        hostname, port);
                }
                else {
                    return ZenError_InvalidArgument;
                }
            }
            else
                return ZenError_InvalidComponentHandle;
        }
        else
        {
            return ZenError_InvalidSensorHandle;
        }
    }
    else
    {
        return ZenError_InvalidClientHandle;
    }
}

