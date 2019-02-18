#ifndef ZEN_SENSORCOMPONENT_H_
#define ZEN_SENSORCOMPONENT_H_

#include <memory>
#include <string_view>

#include "ISensorProperties.h"

namespace zen
{
    class SensorComponent
    {
    public:
        SensorComponent(std::unique_ptr<ISensorProperties> properties)
            : m_properties(std::move(properties))
        {}

        /** Tries to initialize settings of the sensor's component that can fail.
         * After succesfully completing init, m_properties should be set.
         */
        virtual ZenSensorInitError init() = 0;

        virtual ZenError processData(uint8_t function, const unsigned char* data, size_t length) = 0;
        virtual ZenError processEvent(ZenEvent event, const unsigned char* data, size_t length) noexcept = 0;

        virtual std::string_view type() const noexcept = 0;

        ISensorProperties* properties() noexcept { return m_properties.get(); }

    protected:
        std::unique_ptr<ISensorProperties> m_properties;
    };
}

#endif
