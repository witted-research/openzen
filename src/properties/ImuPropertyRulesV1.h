#ifndef ZEN_PROPERTIES_IMUPROPERTYRULESV1_H_
#define ZEN_PROPERTIES_IMUPROPERTYRULESV1_H_

#include "ZenTypes.h"

namespace zen
{
    class ImuPropertyRulesV1
    {
    public:
        constexpr bool isArray(ZenProperty_t property) const
        {
            switch (property)
            {
            case ZenImuProperty_SupportedFilterModes:
            case ZenImuProperty_AccBias:
            case ZenImuProperty_AccSupportedRanges:
            case ZenImuProperty_GyrBias:
            case ZenImuProperty_GyrSupportedRanges:
            case ZenImuProperty_MagBias:
            case ZenImuProperty_MagSupportedRanges:
            case ZenImuProperty_MagReference:
            case ZenImuProperty_MagHardIronOffset:
                return true;

            default:
                return false;
            }
        }

        constexpr bool isCommand(ZenProperty_t property) const
        {
            switch (property)
            {
            case ZenImuProperty_PollSensorData:
            case ZenImuProperty_CalibrateGyro:
            case ZenImuProperty_ResetOrientationOffset:
                return true;

            default:
                return false;
            }
        }

        constexpr bool isConstant(ZenProperty_t property) const
        {
            switch (property)
            {
            case ZenImuProperty_SupportedFilterModes:
            case ZenImuProperty_AccSupportedRanges:
            case ZenImuProperty_GyrSupportedRanges:
            case ZenImuProperty_MagSupportedRanges:
                return true;

            default:
                return false;
            }
        }

        constexpr ZenPropertyType type(ZenProperty_t property) const
        {
            switch (property)
            {
            case ZenImuProperty_StreamData:
            case ZenImuProperty_GyrUseAutoCalibration:
            case ZenImuProperty_GyrUseThreshold:
            case ZenImuProperty_OutputLowPrecision:
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

            case ZenImuProperty_CentricCompensationRate:
            case ZenImuProperty_LinearCompensationRate:
            case ZenImuProperty_FieldRadius:
            case ZenImuProperty_AccBias:
            case ZenImuProperty_GyrBias:
            case ZenImuProperty_MagBias:
            case ZenImuProperty_MagReference:
            case ZenImuProperty_MagHardIronOffset:
                return ZenPropertyType_Float;

            case ZenImuProperty_FilterMode:
            case ZenImuProperty_FilterPreset:
            case ZenImuProperty_OrientationOffsetMode:
            case ZenImuProperty_AccRange:
            case ZenImuProperty_AccSupportedRanges:
            case ZenImuProperty_GyrRange:
            case ZenImuProperty_GyrSupportedRanges:
            case ZenImuProperty_MagRange:
            case ZenImuProperty_MagSupportedRanges:
                return ZenPropertyType_Int32;

            case ZenImuProperty_AccAlignment:
            case ZenImuProperty_GyrAlignment:
            case ZenImuProperty_MagAlignment:
            case ZenImuProperty_MagSoftIronMatrix:
                return ZenPropertyType_Matrix;

            case ZenImuProperty_SupportedFilterModes:
                return ZenPropertyType_String;

            default:
                return ZenPropertyType_Invalid;
            }
        }
    };
}

#endif