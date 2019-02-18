#include "OpenZenCAPI.h"

#include <numeric>

#include "SensorManager.h"

namespace
{
    size_t countComponentsOfType(const std::vector<std::shared_ptr<zen::SensorComponent>>& components, std::string_view type)
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

    auto& manager = zen::SensorManager::get();
    if (auto error = manager.init())
    {
        outHandle->handle = 0;
        return error;
    }

    // [TODO] Create an actual client system
    outHandle->handle = *reinterpret_cast<uintptr_t*>(&manager);
    return ZenError_None;
}
ZEN_API ZenError ZenShutdown(ZenClientHandle_t handle)
{
    auto& manager = zen::SensorManager::get();
    if (*reinterpret_cast<uintptr_t*>(&manager) != handle.handle)
        return ZenError_InvalidClientHandle;

    return manager.deinit();
}

ZEN_API ZenAsyncStatus ZenListSensorsAsync(ZenClientHandle_t handle, const char* const typeFilter, ZenSensorDesc** outDesc, size_t* const outLength)
{
    if (outLength == nullptr)
        return ZenAsync_InvalidArgument;

    auto& manager = zen::SensorManager::get();
    if (*reinterpret_cast<uintptr_t*>(&manager) != handle.handle)
        return ZenAsync_InvalidArgument;

    const auto filter = typeFilter ? std::make_optional<std::string>(typeFilter) : std::nullopt;
    if (auto descriptions = manager.listSensorsAsync(filter))
    {
        if (!descriptions->empty())
        {
            ZenSensorDesc* list = new ZenSensorDesc[descriptions->size()];
            std::copy(descriptions->cbegin(), descriptions->cend(), list);

            *outDesc = list;
        }

        *outLength = descriptions->size();
        return ZenAsync_Finished;
    }
    else
    {
        return descriptions.error();
    }
}

ZEN_API ZenSensorInitError ZenObtainSensor(ZenClientHandle_t clientHandle, const ZenSensorDesc* const desc, ZenSensorHandle_t* outSensorHandle)
{
    if (desc == nullptr)
        return ZenSensorInitError_IsNull;

    if (outSensorHandle == nullptr)
        return ZenSensorInitError_IsNull;

    auto& manager = zen::SensorManager::get();
    if (*reinterpret_cast<uintptr_t*>(&manager) != clientHandle.handle)
        return ZenSensorInitError_InvalidHandle;

    auto sensor = manager.obtain(*desc);
    if (!sensor)
    {
        outSensorHandle->handle = 0;
        return sensor.error();
    }

    *reinterpret_cast<uintptr_t*>(outSensorHandle) = sensor->first;
    return ZenSensorInitError_None;
}

ZEN_API ZenError ZenReleaseSensor(ZenClientHandle_t clientHandle, ZenSensorHandle_t sensorHandle)
{
    auto& manager = zen::SensorManager::get();
    if (*reinterpret_cast<uintptr_t*>(&manager) != clientHandle.handle)
        return ZenError_InvalidClientHandle;

    return manager.release(sensorHandle.handle);
}

ZEN_API bool ZenPollNextEvent(ZenClientHandle_t handle, ZenEvent* const outEvent)
{
    if (outEvent == nullptr)
        return false;

    auto& manager = zen::SensorManager::get();
    if (*reinterpret_cast<uintptr_t*>(&manager) != handle.handle)
        return false;

    if (auto optEvent = manager.pollNextEvent())
    {
        *outEvent = std::move(*optEvent);
        return true;
    }

    return false;
}

ZEN_API bool ZenWaitForNextEvent(ZenClientHandle_t handle, ZenEvent* const outEvent)
{
    if (outEvent == nullptr)
        return false;

    auto& manager = zen::SensorManager::get();
    if (*reinterpret_cast<uintptr_t*>(&manager) != handle.handle)
        return false;

    if (auto optEvent = manager.waitForNextEvent())
    {
        *outEvent = std::move(*optEvent);
        return true;
    }

    return false;
}

ZEN_API ZenError ZenSensorComponents(ZenClientHandle_t clientHandle, ZenSensorHandle_t sensorHandle, const char* const type, ZenComponentHandle_t** outComponentHandles, size_t* const outLength)
{
    if (outLength == nullptr)
        return ZenError_IsNull;

    auto& manager = zen::SensorManager::get();
    if (*reinterpret_cast<uintptr_t*>(&manager) != clientHandle.handle)
        return ZenError_InvalidClientHandle;

    auto sensor = manager.getSensorByToken(sensorHandle.handle);
    if (!sensor)
        return ZenError_InvalidSensorHandle;

    const auto& components = sensor.value()->components();
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
                *reinterpret_cast<uintptr_t*>(&componentHandles[idx++]) = componentIdx + 1;

        *outComponentHandles = componentHandles;
    }

    return ZenError_None;
}

ZEN_API const char* const ZenSensorIoType(ZenClientHandle_t clientHandle, ZenSensorHandle_t sensorHandle)
{
    auto& manager = zen::SensorManager::get();
    if (*reinterpret_cast<uintptr_t*>(&manager) != clientHandle.handle)
        return nullptr;

    if (auto sensor = manager.getSensorByToken(sensorHandle.handle))
        return sensor.value()->ioType();

    return nullptr;
}

ZEN_API bool ZenSensorEquals(ZenClientHandle_t clientHandle, ZenSensorHandle_t sensorHandle, const ZenSensorDesc* const desc)
{
    if (desc == nullptr)
        return false;

    auto& manager = zen::SensorManager::get();
    if (*reinterpret_cast<uintptr_t*>(&manager) != clientHandle.handle)
        return false;

    if (auto sensor = manager.getSensorByToken(sensorHandle.handle))
        return sensor.value()->equals(*desc);

    return false;

}

ZEN_API ZenAsyncStatus ZenSensorUpdateFirmwareAsync(ZenClientHandle_t clientHandle, ZenSensorHandle_t sensorHandle, const unsigned char* const buffer, size_t bufferSize)
{
    auto& manager = zen::SensorManager::get();
    if (*reinterpret_cast<uintptr_t*>(&manager) != clientHandle.handle)
        return ZenAsync_InvalidArgument;

    if (auto sensor = manager.getSensorByToken(sensorHandle.handle))
        return sensor.value()->updateFirmwareAsync(buffer, bufferSize);

    return ZenAsync_InvalidArgument;
}

ZEN_API ZenAsyncStatus ZenSensorUpdateIAPAsync(ZenClientHandle_t clientHandle, ZenSensorHandle_t sensorHandle, const unsigned char* const buffer, size_t bufferSize)
{
    auto& manager = zen::SensorManager::get();
    if (*reinterpret_cast<uintptr_t*>(&manager) != clientHandle.handle)
        return ZenAsync_InvalidArgument;

    if (auto sensor = manager.getSensorByToken(sensorHandle.handle))
        return sensor.value()->updateIAPAsync(buffer, bufferSize);

    return ZenAsync_InvalidArgument;
}

ZEN_API ZenError ZenSensorExecuteProperty(ZenClientHandle_t clientHandle, ZenSensorHandle_t sensorHandle, ZenProperty_t property)
{
    auto& manager = zen::SensorManager::get();
    if (*reinterpret_cast<uintptr_t*>(&manager) != clientHandle.handle)
        return ZenError_InvalidClientHandle;

    if (auto sensor = manager.getSensorByToken(sensorHandle.handle))
        return sensor.value()->properties()->execute(property);

    return ZenError_InvalidSensorHandle;
}

ZEN_API ZenError ZenSensorGetArrayProperty(ZenClientHandle_t clientHandle, ZenSensorHandle_t sensorHandle, ZenProperty_t property, ZenPropertyType type, void* const buffer, size_t* bufferSize)
{
    if (bufferSize == nullptr)
        return ZenError_IsNull;

    auto& manager = zen::SensorManager::get();
    if (*reinterpret_cast<uintptr_t*>(&manager) != clientHandle.handle)
        return ZenError_InvalidClientHandle;

    if (auto sensor = manager.getSensorByToken(sensorHandle.handle))
        return sensor.value()->properties()->getArray(property, type, buffer, *bufferSize);

    return ZenError_InvalidSensorHandle;
}

ZEN_API ZenError ZenSensorGetBoolProperty(ZenClientHandle_t clientHandle, ZenSensorHandle_t sensorHandle, ZenProperty_t property, bool* const outValue)
{
    if (outValue == nullptr)
        return ZenError_IsNull;

    auto& manager = zen::SensorManager::get();
    if (*reinterpret_cast<uintptr_t*>(&manager) != clientHandle.handle)
        return ZenError_InvalidClientHandle;

    if (auto sensor = manager.getSensorByToken(sensorHandle.handle))
        return sensor.value()->properties()->getBool(property, *outValue);

    return ZenError_InvalidSensorHandle;
}

ZEN_API ZenError ZenSensorGetFloatProperty(ZenClientHandle_t clientHandle, ZenSensorHandle_t sensorHandle, ZenProperty_t property, float* const outValue)
{
    if (outValue == nullptr)
        return ZenError_IsNull;

    auto& manager = zen::SensorManager::get();
    if (*reinterpret_cast<uintptr_t*>(&manager) != clientHandle.handle)
        return ZenError_InvalidClientHandle;

    if (auto sensor = manager.getSensorByToken(sensorHandle.handle))
        return sensor.value()->properties()->getFloat(property, *outValue);

    return ZenError_InvalidSensorHandle;
}

ZEN_API ZenError ZenSensorGetInt32Property(ZenClientHandle_t clientHandle, ZenSensorHandle_t sensorHandle, ZenProperty_t property, int32_t* const outValue)
{
    if (outValue == nullptr)
        return ZenError_IsNull;

    auto& manager = zen::SensorManager::get();
    if (*reinterpret_cast<uintptr_t*>(&manager) != clientHandle.handle)
        return ZenError_InvalidClientHandle;

    if (auto sensor = manager.getSensorByToken(sensorHandle.handle))
        return sensor.value()->properties()->getInt32(property, *outValue);

    return ZenError_InvalidSensorHandle;
}

ZEN_API ZenError ZenSensorGetMatrix33Property(ZenClientHandle_t clientHandle, ZenSensorHandle_t sensorHandle, ZenProperty_t property, ZenMatrix3x3f* const outValue)
{
    if (outValue == nullptr)
        return ZenError_IsNull;

    auto& manager = zen::SensorManager::get();
    if (*reinterpret_cast<uintptr_t*>(&manager) != clientHandle.handle)
        return ZenError_InvalidClientHandle;

    if (auto sensor = manager.getSensorByToken(sensorHandle.handle))
        return sensor.value()->properties()->getMatrix33(property, *outValue);

    return ZenError_InvalidSensorHandle;
}

ZEN_API ZenError ZenSensorGetStringProperty(ZenClientHandle_t clientHandle, ZenSensorHandle_t sensorHandle, ZenProperty_t property, char* const buffer, size_t* const bufferSize)
{
    if (bufferSize == nullptr)
        return ZenError_IsNull;

    auto& manager = zen::SensorManager::get();
    if (*reinterpret_cast<uintptr_t*>(&manager) != clientHandle.handle)
        return ZenError_InvalidClientHandle;

    if (auto sensor = manager.getSensorByToken(sensorHandle.handle))
        return sensor.value()->properties()->getString(property, buffer, *bufferSize);

    return ZenError_InvalidSensorHandle;
}

ZEN_API ZenError ZenSensorGetUInt64Property(ZenClientHandle_t clientHandle, ZenSensorHandle_t sensorHandle, ZenProperty_t property, uint64_t* const outValue)
{
    if (outValue == nullptr)
        return ZenError_IsNull;

    auto& manager = zen::SensorManager::get();
    if (*reinterpret_cast<uintptr_t*>(&manager) != clientHandle.handle)
        return ZenError_InvalidClientHandle;

    if (auto sensor = manager.getSensorByToken(sensorHandle.handle))
        return sensor.value()->properties()->getUInt64(property, *outValue);

    return ZenError_InvalidSensorHandle;
}

ZEN_API ZenError ZenSensorSetArrayProperty(ZenClientHandle_t clientHandle, ZenSensorHandle_t sensorHandle, ZenProperty_t property, ZenPropertyType type, const void* buffer, size_t bufferSize)
{
    if (buffer == nullptr)
        return ZenError_IsNull;

    auto& manager = zen::SensorManager::get();
    if (*reinterpret_cast<uintptr_t*>(&manager) != clientHandle.handle)
        return ZenError_InvalidClientHandle;

    if (auto sensor = manager.getSensorByToken(sensorHandle.handle))
        return sensor.value()->properties()->setArray(property, type, buffer, bufferSize);

    return ZenError_InvalidSensorHandle;
}

ZEN_API ZenError ZenSensorSetBoolProperty(ZenClientHandle_t clientHandle, ZenSensorHandle_t sensorHandle, ZenProperty_t property, bool value)
{
    auto& manager = zen::SensorManager::get();
    if (*reinterpret_cast<uintptr_t*>(&manager) != clientHandle.handle)
        return ZenError_InvalidClientHandle;

    if (auto sensor = manager.getSensorByToken(sensorHandle.handle))
        return sensor.value()->properties()->setBool(property, value);

    return ZenError_InvalidSensorHandle;
}

ZEN_API ZenError ZenSensorSetFloatProperty(ZenClientHandle_t clientHandle, ZenSensorHandle_t sensorHandle, ZenProperty_t property, float value)
{
    auto& manager = zen::SensorManager::get();
    if (*reinterpret_cast<uintptr_t*>(&manager) != clientHandle.handle)
        return ZenError_InvalidClientHandle;

    if (auto sensor = manager.getSensorByToken(sensorHandle.handle))
        return sensor.value()->properties()->setFloat(property, value);

    return ZenError_InvalidSensorHandle;
}

ZEN_API ZenError ZenSensorSetInt32Property(ZenClientHandle_t clientHandle, ZenSensorHandle_t sensorHandle, ZenProperty_t property, int32_t value)
{
    auto& manager = zen::SensorManager::get();
    if (*reinterpret_cast<uintptr_t*>(&manager) != clientHandle.handle)
        return ZenError_InvalidClientHandle;

    if (auto sensor = manager.getSensorByToken(sensorHandle.handle))
        return sensor.value()->properties()->setInt32(property, value);

    return ZenError_InvalidSensorHandle;
}

ZEN_API ZenError ZenSensorSetMatrix33Property(ZenClientHandle_t clientHandle, ZenSensorHandle_t sensorHandle, ZenProperty_t property, const ZenMatrix3x3f* const value)
{
    if (value == nullptr)
        return ZenError_IsNull;

    auto& manager = zen::SensorManager::get();
    if (*reinterpret_cast<uintptr_t*>(&manager) != clientHandle.handle)
        return ZenError_InvalidClientHandle;

    if (auto sensor = manager.getSensorByToken(sensorHandle.handle))
        return sensor.value()->properties()->setMatrix33(property, *value);

    return ZenError_InvalidSensorHandle;
}

ZEN_API ZenError ZenSensorSetStringProperty(ZenClientHandle_t clientHandle, ZenSensorHandle_t sensorHandle, ZenProperty_t property, const char* buffer, size_t bufferSize)
{
    if (buffer == nullptr)
        return ZenError_IsNull;

    auto& manager = zen::SensorManager::get();
    if (*reinterpret_cast<uintptr_t*>(&manager) != clientHandle.handle)
        return ZenError_InvalidClientHandle;

    if (auto sensor = manager.getSensorByToken(sensorHandle.handle))
        return sensor.value()->properties()->setString(property, buffer, bufferSize);

    return ZenError_InvalidSensorHandle;
}

ZEN_API ZenError ZenSensorSetUInt64Property(ZenClientHandle_t clientHandle, ZenSensorHandle_t sensorHandle, ZenProperty_t property, uint64_t value)
{
    auto& manager = zen::SensorManager::get();
    if (*reinterpret_cast<uintptr_t*>(&manager) != clientHandle.handle)
        return ZenError_InvalidClientHandle;

    if (auto sensor = manager.getSensorByToken(sensorHandle.handle))
        return sensor.value()->properties()->setUInt64(property, value);

    return ZenError_InvalidSensorHandle;
}

ZEN_API bool ZenSensorIsArrayProperty(ZenClientHandle_t clientHandle, ZenSensorHandle_t sensorHandle, ZenProperty_t property)
{
    auto& manager = zen::SensorManager::get();
    if (*reinterpret_cast<uintptr_t*>(&manager) != clientHandle.handle)
        return false;

    if (auto sensor = manager.getSensorByToken(sensorHandle.handle))
        return sensor.value()->properties()->isArray(property);

    return false;
}

ZEN_API bool ZenSensorIsConstantProperty(ZenClientHandle_t clientHandle, ZenSensorHandle_t sensorHandle, ZenProperty_t property)
{
    auto& manager = zen::SensorManager::get();
    if (*reinterpret_cast<uintptr_t*>(&manager) != clientHandle.handle)
        return false;

    if (auto sensor = manager.getSensorByToken(sensorHandle.handle))
        return sensor.value()->properties()->isConstant(property);

    return false;
}

ZEN_API bool ZenSensorIsExecutableProperty(ZenClientHandle_t clientHandle, ZenSensorHandle_t sensorHandle, ZenProperty_t property)
{
    auto& manager = zen::SensorManager::get();
    if (*reinterpret_cast<uintptr_t*>(&manager) != clientHandle.handle)
        return false;

    if (auto sensor = manager.getSensorByToken(sensorHandle.handle))
        return sensor.value()->properties()->isExecutable(property);

    return false;
}

ZEN_API ZenPropertyType ZenSensorPropertyType(ZenClientHandle_t clientHandle, ZenSensorHandle_t sensorHandle, ZenProperty_t property)
{
    auto& manager = zen::SensorManager::get();
    if (*reinterpret_cast<uintptr_t*>(&manager) != clientHandle.handle)
        return ZenPropertyType_Invalid;

    if (auto sensor = manager.getSensorByToken(sensorHandle.handle))
        return sensor.value()->properties()->type(property);

    return ZenPropertyType_Invalid;
}

ZEN_API const char* const ZenSensorComponentType(ZenClientHandle_t clientHandle, ZenSensorHandle_t sensorHandle, ZenComponentHandle_t componentHandle)
{
    auto& manager = zen::SensorManager::get();
    if (*reinterpret_cast<uintptr_t*>(&manager) != clientHandle.handle)
        return nullptr;

    if (auto sensor = manager.getSensorByToken(sensorHandle.handle))
    {
        const size_t idx = *reinterpret_cast<uintptr_t*>(&componentHandle) - 1;
        const auto& components = sensor.value()->components();
        if (idx >= components.size())
            return nullptr;

        return components[idx]->type().data();
    }

    return nullptr;
}

ZEN_API ZenError ZenSensorComponentExecuteProperty(ZenClientHandle_t clientHandle, ZenSensorHandle_t sensorHandle, ZenComponentHandle_t componentHandle, ZenProperty_t property)
{
    auto& manager = zen::SensorManager::get();
    if (*reinterpret_cast<uintptr_t*>(&manager) != clientHandle.handle)
        return ZenError_InvalidClientHandle;

    if (auto sensor = manager.getSensorByToken(sensorHandle.handle))
    {
        const size_t idx = *reinterpret_cast<uintptr_t*>(&componentHandle) - 1;
        const auto& components = sensor.value()->components();
        if (idx >= components.size())
            return ZenError_InvalidComponentHandle;

        return components[idx]->properties()->execute(property);
    }

    return ZenError_InvalidSensorHandle;
}

ZEN_API ZenError ZenSensorComponentGetArrayProperty(ZenClientHandle_t clientHandle, ZenSensorHandle_t sensorHandle, ZenComponentHandle_t componentHandle, ZenProperty_t property, ZenPropertyType type, void* const buffer, size_t* bufferSize)
{
    if (bufferSize == nullptr)
        return ZenError_IsNull;

    auto& manager = zen::SensorManager::get();
    if (*reinterpret_cast<uintptr_t*>(&manager) != clientHandle.handle)
        return ZenError_InvalidClientHandle;

    if (auto sensor = manager.getSensorByToken(sensorHandle.handle))
    {
        const size_t idx = *reinterpret_cast<uintptr_t*>(&componentHandle) - 1;
        const auto& components = sensor.value()->components();
        if (idx >= components.size())
            return ZenError_InvalidComponentHandle;

        return components[idx]->properties()->getArray(property, type, buffer, *bufferSize);
    }

    return ZenError_InvalidSensorHandle;
}

/** If successful fills the value with the property's boolean value, otherwise returns an error. */
ZEN_API ZenError ZenSensorComponentGetBoolProperty(ZenClientHandle_t clientHandle, ZenSensorHandle_t sensorHandle, ZenComponentHandle_t componentHandle, ZenProperty_t property, bool* const outValue)
{
    if (outValue == nullptr)
        return ZenError_IsNull;

    auto& manager = zen::SensorManager::get();
    if (*reinterpret_cast<uintptr_t*>(&manager) != clientHandle.handle)
        return ZenError_InvalidClientHandle;

    if (auto sensor = manager.getSensorByToken(sensorHandle.handle))
    {
        const size_t idx = *reinterpret_cast<uintptr_t*>(&componentHandle) - 1;
        const auto& components = sensor.value()->components();
        if (idx >= components.size())
            return ZenError_InvalidComponentHandle;

        return components[idx]->properties()->getBool(property, *outValue);
    }

    return ZenError_InvalidSensorHandle;
}

ZEN_API ZenError ZenSensorComponentGetFloatProperty(ZenClientHandle_t clientHandle, ZenSensorHandle_t sensorHandle, ZenComponentHandle_t componentHandle, ZenProperty_t property, float* const outValue)
{
    if (outValue == nullptr)
        return ZenError_IsNull;

    auto& manager = zen::SensorManager::get();
    if (*reinterpret_cast<uintptr_t*>(&manager) != clientHandle.handle)
        return ZenError_InvalidClientHandle;

    if (auto sensor = manager.getSensorByToken(sensorHandle.handle))
    {
        const size_t idx = *reinterpret_cast<uintptr_t*>(&componentHandle) - 1;
        const auto& components = sensor.value()->components();
        if (idx >= components.size())
            return ZenError_InvalidComponentHandle;

        return components[idx]->properties()->getFloat(property, *outValue);
    }

    return ZenError_InvalidSensorHandle;
}

ZEN_API ZenError ZenSensorComponentGetInt32Property(ZenClientHandle_t clientHandle, ZenSensorHandle_t sensorHandle, ZenComponentHandle_t componentHandle, ZenProperty_t property, int32_t* const outValue)
{
    if (outValue == nullptr)
        return ZenError_IsNull;

    auto& manager = zen::SensorManager::get();
    if (*reinterpret_cast<uintptr_t*>(&manager) != clientHandle.handle)
        return ZenError_InvalidClientHandle;

    if (auto sensor = manager.getSensorByToken(sensorHandle.handle))
    {
        const size_t idx = *reinterpret_cast<uintptr_t*>(&componentHandle) - 1;
        const auto& components = sensor.value()->components();
        if (idx >= components.size())
            return ZenError_InvalidComponentHandle;

        return components[idx]->properties()->getInt32(property, *outValue);
    }

    return ZenError_InvalidSensorHandle;
}

ZEN_API ZenError ZenSensorComponentGetMatrix33Property(ZenClientHandle_t clientHandle, ZenSensorHandle_t sensorHandle, ZenComponentHandle_t componentHandle, ZenProperty_t property, ZenMatrix3x3f* const outValue)
{
    if (outValue == nullptr)
        return ZenError_IsNull;

    auto& manager = zen::SensorManager::get();
    if (*reinterpret_cast<uintptr_t*>(&manager) != clientHandle.handle)
        return ZenError_InvalidClientHandle;

    if (auto sensor = manager.getSensorByToken(sensorHandle.handle))
    {
        const size_t idx = *reinterpret_cast<uintptr_t*>(&componentHandle) - 1;
        const auto& components = sensor.value()->components();
        if (idx >= components.size())
            return ZenError_InvalidComponentHandle;

        return components[idx]->properties()->getMatrix33(property, *outValue);
    }

    return ZenError_InvalidSensorHandle;
}

ZEN_API ZenError ZenSensorComponentGetStringProperty(ZenClientHandle_t clientHandle, ZenSensorHandle_t sensorHandle, ZenComponentHandle_t componentHandle, ZenProperty_t property, char* const buffer, size_t* const bufferSize)
{
    if (bufferSize == nullptr)
        return ZenError_IsNull;

    auto& manager = zen::SensorManager::get();
    if (*reinterpret_cast<uintptr_t*>(&manager) != clientHandle.handle)
        return ZenError_InvalidClientHandle;

    if (auto sensor = manager.getSensorByToken(sensorHandle.handle))
    {
        const size_t idx = *reinterpret_cast<uintptr_t*>(&componentHandle) - 1;
        const auto& components = sensor.value()->components();
        if (idx >= components.size())
            return ZenError_InvalidComponentHandle;

        return components[idx]->properties()->getString(property, buffer, *bufferSize);
    }

    return ZenError_InvalidSensorHandle;
}

ZEN_API ZenError ZenSensorComponentGetUInt64Property(ZenClientHandle_t clientHandle, ZenSensorHandle_t sensorHandle, ZenComponentHandle_t componentHandle, ZenProperty_t property, uint64_t* const outValue)
{
    if (outValue == nullptr)
        return ZenError_IsNull;

    auto& manager = zen::SensorManager::get();
    if (*reinterpret_cast<uintptr_t*>(&manager) != clientHandle.handle)
        return ZenError_InvalidClientHandle;

    if (auto sensor = manager.getSensorByToken(sensorHandle.handle))
    {
        const size_t idx = *reinterpret_cast<uintptr_t*>(&componentHandle) - 1;
        const auto& components = sensor.value()->components();
        if (idx >= components.size())
            return ZenError_InvalidComponentHandle;

        return components[idx]->properties()->getUInt64(property, *outValue);
    }

    return ZenError_InvalidSensorHandle;
}

ZEN_API ZenError ZenSensorComponentSetArrayProperty(ZenClientHandle_t clientHandle, ZenSensorHandle_t sensorHandle, ZenComponentHandle_t componentHandle, ZenProperty_t property, ZenPropertyType type, const void* buffer, size_t bufferSize)
{
    if (buffer == nullptr)
        return ZenError_IsNull;

    auto& manager = zen::SensorManager::get();
    if (*reinterpret_cast<uintptr_t*>(&manager) != clientHandle.handle)
        return ZenError_InvalidClientHandle;

    if (auto sensor = manager.getSensorByToken(sensorHandle.handle))
    {
        const size_t idx = *reinterpret_cast<uintptr_t*>(&componentHandle) - 1;
        const auto& components = sensor.value()->components();
        if (idx >= components.size())
            return ZenError_InvalidComponentHandle;

        return components[idx]->properties()->setArray(property, type, buffer, bufferSize);
    }

    return ZenError_InvalidSensorHandle;
}

ZEN_API ZenError ZenSensorComponentSetBoolProperty(ZenClientHandle_t clientHandle, ZenSensorHandle_t sensorHandle, ZenComponentHandle_t componentHandle, ZenProperty_t property, bool value)
{
    auto& manager = zen::SensorManager::get();
    if (*reinterpret_cast<uintptr_t*>(&manager) != clientHandle.handle)
        return ZenError_InvalidClientHandle;

    if (auto sensor = manager.getSensorByToken(sensorHandle.handle))
    {
        const size_t idx = *reinterpret_cast<uintptr_t*>(&componentHandle) - 1;
        const auto& components = sensor.value()->components();
        if (idx >= components.size())
            return ZenError_InvalidComponentHandle;

        return components[idx]->properties()->setBool(property, value);
    }

    return ZenError_InvalidSensorHandle;
}

ZEN_API ZenError ZenSensorComponentSetFloatProperty(ZenClientHandle_t clientHandle, ZenSensorHandle_t sensorHandle, ZenComponentHandle_t componentHandle, ZenProperty_t property, float value)
{
    auto& manager = zen::SensorManager::get();
    if (*reinterpret_cast<uintptr_t*>(&manager) != clientHandle.handle)
        return ZenError_InvalidClientHandle;

    if (auto sensor = manager.getSensorByToken(sensorHandle.handle))
    {
        const size_t idx = *reinterpret_cast<uintptr_t*>(&componentHandle) - 1;
        const auto& components = sensor.value()->components();
        if (idx >= components.size())
            return ZenError_InvalidComponentHandle;

        return components[idx]->properties()->setFloat(property, value);
    }

    return ZenError_InvalidSensorHandle;
}

ZEN_API ZenError ZenSensorComponentSetInt32Property(ZenClientHandle_t clientHandle, ZenSensorHandle_t sensorHandle, ZenComponentHandle_t componentHandle, ZenProperty_t property, int32_t value)
{
    auto& manager = zen::SensorManager::get();
    if (*reinterpret_cast<uintptr_t*>(&manager) != clientHandle.handle)
        return ZenError_InvalidClientHandle;

    if (auto sensor = manager.getSensorByToken(sensorHandle.handle))
    {
        const size_t idx = *reinterpret_cast<uintptr_t*>(&componentHandle) - 1;
        const auto& components = sensor.value()->components();
        if (idx >= components.size())
            return ZenError_InvalidComponentHandle;

        return components[idx]->properties()->setInt32(property, value);
    }

    return ZenError_InvalidSensorHandle;
}

ZEN_API ZenError ZenSensorComponentSetMatrix33Property(ZenClientHandle_t clientHandle, ZenSensorHandle_t sensorHandle, ZenComponentHandle_t componentHandle, ZenProperty_t property, const ZenMatrix3x3f* const value)
{
    if (value == nullptr)
        return ZenError_IsNull;

    auto& manager = zen::SensorManager::get();
    if (*reinterpret_cast<uintptr_t*>(&manager) != clientHandle.handle)
        return ZenError_InvalidClientHandle;

    if (auto sensor = manager.getSensorByToken(sensorHandle.handle))
    {
        const size_t idx = *reinterpret_cast<uintptr_t*>(&componentHandle) - 1;
        const auto& components = sensor.value()->components();
        if (idx >= components.size())
            return ZenError_InvalidComponentHandle;

        return components[idx]->properties()->setMatrix33(property, *value);
    }

    return ZenError_InvalidSensorHandle;
}

ZEN_API ZenError ZenSensorComponentSetStringProperty(ZenClientHandle_t clientHandle, ZenSensorHandle_t sensorHandle, ZenComponentHandle_t componentHandle, ZenProperty_t property, const char* buffer, size_t bufferSize)
{
    if (buffer == nullptr)
        return ZenError_IsNull;

    auto& manager = zen::SensorManager::get();
    if (*reinterpret_cast<uintptr_t*>(&manager) != clientHandle.handle)
        return ZenError_InvalidClientHandle;

    if (auto sensor = manager.getSensorByToken(sensorHandle.handle))
    {
        const size_t idx = *reinterpret_cast<uintptr_t*>(&componentHandle) - 1;
        const auto& components = sensor.value()->components();
        if (idx >= components.size())
            return ZenError_InvalidComponentHandle;

        return components[idx]->properties()->setString(property, buffer, bufferSize);
    }

    return ZenError_InvalidSensorHandle;
}

ZEN_API ZenError ZenSensorComponentSetUInt64Property(ZenClientHandle_t clientHandle, ZenSensorHandle_t sensorHandle, ZenComponentHandle_t componentHandle, ZenProperty_t property, uint64_t value)
{
    auto& manager = zen::SensorManager::get();
    if (*reinterpret_cast<uintptr_t*>(&manager) != clientHandle.handle)
        return ZenError_InvalidClientHandle;

    if (auto sensor = manager.getSensorByToken(sensorHandle.handle))
    {
        const size_t idx = *reinterpret_cast<uintptr_t*>(&componentHandle) - 1;
        const auto& components = sensor.value()->components();
        if (idx >= components.size())
            return ZenError_InvalidComponentHandle;

        return components[idx]->properties()->setUInt64(property, value);
    }

    return ZenError_InvalidSensorHandle;
}

ZEN_API bool ZenSensorComponentIsArrayProperty(ZenClientHandle_t clientHandle, ZenSensorHandle_t sensorHandle, ZenComponentHandle_t componentHandle, ZenProperty_t property)
{
    auto& manager = zen::SensorManager::get();
    if (*reinterpret_cast<uintptr_t*>(&manager) != clientHandle.handle)
        return false;

    if (auto sensor = manager.getSensorByToken(sensorHandle.handle))
    {
        const size_t idx = *reinterpret_cast<uintptr_t*>(&componentHandle) - 1;
        const auto& components = sensor.value()->components();
        if (idx >= components.size())
            return false;

        return components[idx]->properties()->isArray(property);
    }

    return false;
}

ZEN_API bool ZenSensorComponentIsConstantProperty(ZenClientHandle_t clientHandle, ZenSensorHandle_t sensorHandle, ZenComponentHandle_t componentHandle, ZenProperty_t property)
{
    auto& manager = zen::SensorManager::get();
    if (*reinterpret_cast<uintptr_t*>(&manager) != clientHandle.handle)
        return false;

    if (auto sensor = manager.getSensorByToken(sensorHandle.handle))
    {
        const size_t idx = *reinterpret_cast<uintptr_t*>(&componentHandle) - 1;
        const auto& components = sensor.value()->components();
        if (idx >= components.size())
            return false;

        return components[idx]->properties()->isConstant(property);
    }

    return false;
}

ZEN_API bool ZenSensorComponentIsExecutableProperty(ZenClientHandle_t clientHandle, ZenSensorHandle_t sensorHandle, ZenComponentHandle_t componentHandle, ZenProperty_t property)
{
    auto& manager = zen::SensorManager::get();
    if (*reinterpret_cast<uintptr_t*>(&manager) != clientHandle.handle)
        return false;

    if (auto sensor = manager.getSensorByToken(sensorHandle.handle))
    {
        const size_t idx = *reinterpret_cast<uintptr_t*>(&componentHandle) - 1;
        const auto& components = sensor.value()->components();
        if (idx >= components.size())
            return false;

        return components[idx]->properties()->isExecutable(property);
    }

    return false;
}

ZEN_API ZenPropertyType ZenSensorComponentPropertyType(ZenClientHandle_t clientHandle, ZenSensorHandle_t sensorHandle, ZenComponentHandle_t componentHandle, ZenProperty_t property)
{
    auto& manager = zen::SensorManager::get();
    if (*reinterpret_cast<uintptr_t*>(&manager) != clientHandle.handle)
        return ZenPropertyType_Invalid;

    if (auto sensor = manager.getSensorByToken(sensorHandle.handle))
    {
        const size_t idx = *reinterpret_cast<uintptr_t*>(&componentHandle) - 1;
        const auto& components = sensor.value()->components();
        if (idx >= components.size())
            return ZenPropertyType_Invalid;

        return components[idx]->properties()->type(property);
    }

    return ZenPropertyType_Invalid;
}