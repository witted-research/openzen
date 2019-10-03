#ifndef ZEN_PROPERTIES_IMUPROPERTYRULESV1_H_
#define ZEN_PROPERTIES_IMUPROPERTYRULESV1_H_

#include "ZenTypes.h"

namespace zen
{
    class GnssPropertyRulesV1
    {
    public:
        constexpr bool isArray(ZenProperty_t) const
        {
            return false;
        }

        constexpr bool isConstant(ZenProperty_t) const
        {
            return false;
        }

        constexpr bool isExecutable(ZenProperty_t) const
        {
            return false;
        }

        constexpr ZenPropertyType type(ZenProperty_t property) const
        {
            switch (property)
            {
            case ZenImuProperty_StreamData:

            case ZenImuProperty_OutputRawAcc:
            case ZenImuProperty_OutputRawGyr:
            case ZenImuProperty_OutputRawMag:
            case ZenImuProperty_OutputEuler:
            case ZenImuProperty_OutputQuat:
            case ZenImuProperty_OutputAngularVel:
            case ZenImuProperty_OutputLinearAcc:
            case ZenImuProperty_OutputHeaveMotion:
            // [TODO] Should these go here?
            case ZenImuProperty_OutputAltitude:
            case ZenImuProperty_OutputPressure:
            case ZenImuProperty_OutputTemperature:
                return ZenPropertyType_Bool;

            default:
                return ZenPropertyType_Invalid;
            }
        }
    };
}

#endif
