#ifndef ZEN_API_OPENZEN_H_
#define ZEN_API_OPENZEN_H_

#include "IZenSensorManager.h"
#include "IZenSensor.h"

#include <algorithm>
#include <optional>
#include <utility>

namespace zen
{
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

            length = std::min(sizeof(ZenSensorDesc::name) - 1, length);
            desc.name[length] = '\0';

            length = sizeof(ZenSensorDesc::serialNumber);
            if (auto error = m_sensor->properties()->getString(ZenSensorProperty_SerialNumber, desc.serialNumber, &length))
                return std::make_pair(error, ZenSensorDesc{}); // [TODO] Specific error Sensor_SerialNumberTooLong

            length = std::min(sizeof(ZenSensorDesc::serialNumber) - 1, length);
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
