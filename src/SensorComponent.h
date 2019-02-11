#ifndef ZEN_SENSORCOMPONENT_H_
#define ZEN_SENSORCOMPONENT_H_

#include <memory>

#include "IZenSensor.h"

namespace zen
{
    class SensorComponent : public IZenSensorComponent
    {
    public:
        /** Tries to initialize settings of the sensor's component that can fail.
         * After succesfully completing init, m_properties should be set.
         */
        virtual ZenSensorInitError init() = 0;

        virtual ZenError processData(uint8_t function, const unsigned char* data, size_t length) = 0;

        IZenSensorProperties* properties() override { return m_properties.get(); }

    protected:
        std::unique_ptr<IZenSensorProperties> m_properties;
    };
}

#endif
