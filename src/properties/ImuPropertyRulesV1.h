//===========================================================================//
//
// Copyright (C) 2020 LP-Research Inc.
//
// This file is part of OpenZen, under the MIT License.
// See https://bitbucket.org/lpresearch/openzen/src/master/LICENSE for details
// SPDX-License-Identifier: MIT
//
//===========================================================================//

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
            case ZenImuProperty_AccAlignment:
            case ZenImuProperty_AccBias:
            case ZenImuProperty_AccSupportedRanges:
            case ZenImuProperty_GyrAlignment:
            case ZenImuProperty_GyrBias:
            case ZenImuProperty_GyrSupportedRanges:
            case ZenImuProperty_MagAlignment:
            case ZenImuProperty_MagBias:
            case ZenImuProperty_MagSupportedRanges:
            case ZenImuProperty_MagReference:
            case ZenImuProperty_MagHardIronOffset:
            case ZenImuProperty_MagSoftIronMatrix:
            case ZenImuProperty_CanMapping:
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

        constexpr bool isExecutable(ZenProperty_t property) const
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

        constexpr ZenPropertyType type(ZenProperty_t property) const
        {
            switch (property)
            {
            case ZenImuProperty_SupportedFilterModes:
                return ZenPropertyType_Byte;

            case ZenImuProperty_StreamData:
            case ZenImuProperty_GyrUseAutoCalibration:
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
            case ZenImuProperty_FilterPreset: // Note: property name is misleading, this sets the magnetometer covariance
            case ZenImuProperty_FieldRadius:
            case ZenImuProperty_AccAlignment:
            case ZenImuProperty_AccBias:
            case ZenImuProperty_GyrAlignment:
            case ZenImuProperty_GyrBias:
            case ZenImuProperty_MagAlignment:
            case ZenImuProperty_MagBias:
            case ZenImuProperty_MagReference:
            case ZenImuProperty_MagHardIronOffset:
            case ZenImuProperty_MagSoftIronMatrix:
                return ZenPropertyType_Float;

            case ZenImuProperty_FilterMode:
            case ZenImuProperty_OrientationOffsetMode:
            case ZenImuProperty_AccRange:
            case ZenImuProperty_AccSupportedRanges:
            case ZenImuProperty_GyrRange:
            case ZenImuProperty_GyrSupportedRanges:
            case ZenImuProperty_MagRange:
            case ZenImuProperty_MagSupportedRanges:

            case ZenImuProperty_CanChannelMode:
            case ZenImuProperty_CanPointMode:
            case ZenImuProperty_CanStartId:
            case ZenImuProperty_CanBaudrate:
            case ZenImuProperty_CanMapping:
            case ZenImuProperty_CanHeartbeat:
            case ZenImuProperty_UartBaudRate:
            case ZenImuProperty_UartFormat:
                return ZenPropertyType_Int32;

            default:
                return ZenPropertyType_Invalid;
            }
        }
    };
}

#endif
