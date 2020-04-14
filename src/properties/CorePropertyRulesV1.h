//===========================================================================//
//
// Copyright (C) 2020 LP-Research Inc.
//
// This file is part of OpenZen, under the MIT License.
// See https://bitbucket.org/lpresearch/openzen/src/master/LICENSE for details
// SPDX-License-Identifier: MIT
//
//===========================================================================//

#ifndef ZEN_PROPERTIES_COREPROPERTYRULESV1_H_
#define ZEN_PROPERTIES_COREPROPERTYRULESV1_H_

#include "ZenTypes.h"

namespace zen
{
    class CorePropertyRulesV1
    {
    public:
        constexpr bool isArray(ZenProperty_t property) const
        {
            switch (property)
            {
            case ZenSensorProperty_DeviceName:
            case ZenSensorProperty_FirmwareInfo:
            case ZenSensorProperty_FirmwareVersion:
            case ZenSensorProperty_SerialNumber:
            case ZenSensorProperty_SensorModel:
                return true;

            default:
                return false;
            }
        }

        constexpr bool isConstant(ZenProperty_t property) const
        {
            switch (property)
            {
            case ZenSensorProperty_DeviceName:
            case ZenSensorProperty_FirmwareInfo:
            case ZenSensorProperty_FirmwareVersion:
            case ZenSensorProperty_SerialNumber:
            case ZenSensorProperty_DataMode:
            case ZenSensorProperty_TimeOffset:
                return false;

            default:
                return true;
            }
        }

        constexpr bool isExecutable(ZenProperty_t property) const
        {
            switch (property)
            {
            case ZenSensorProperty_RestoreFactorySettings:
            case ZenSensorProperty_StoreSettingsInFlash:
                return true;

            default:
                return false;
            }
        }

        constexpr ZenPropertyType type(ZenProperty_t property) const
        {
            switch (property)
            {
            case ZenSensorProperty_DeviceName:
            case ZenSensorProperty_FirmwareInfo:
            case ZenSensorProperty_SerialNumber:
            case ZenSensorProperty_SensorModel:
                return ZenPropertyType_Byte;

            case ZenSensorProperty_BatteryCharging:
                return ZenPropertyType_Bool;

            case ZenSensorProperty_BatteryLevel:
            case ZenSensorProperty_BatteryVoltage:
                return ZenPropertyType_Float;

            case ZenSensorProperty_FirmwareVersion:
            case ZenSensorProperty_DataMode:
            case ZenSensorProperty_TimeOffset:
                return ZenPropertyType_Int32;

            default:
                return ZenPropertyType_Invalid;
            }
        }
    };
}

#endif
