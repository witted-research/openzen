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
            case ZenSensorProperty_SerialNumber:
            case ZenSensorProperty_FirmwareVersion:
                return true;

            default:
                return false;
            }
        }

        constexpr bool isConstant(ZenProperty_t property) const
        {
            switch (property)
            {
            case ZenSensorProperty_SerialNumber:
            case ZenSensorProperty_DeviceName:
            case ZenSensorProperty_FirmwareInfo:
            case ZenSensorProperty_FirmwareVersion:
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
            case ZenSensorProperty_BatteryCharging:
                return ZenPropertyType_Bool;

            case ZenSensorProperty_BatteryLevel:
            case ZenSensorProperty_BatteryVoltage:
                return ZenPropertyType_Float;

            case ZenSensorProperty_FirmwareVersion:
            case ZenSensorProperty_DataMode:
            case ZenSensorProperty_TimeOffset:
                return ZenPropertyType_Int32;

            case ZenSensorProperty_DeviceName:
            case ZenSensorProperty_FirmwareInfo:
            case ZenSensorProperty_SerialNumber:
                return ZenPropertyType_String;

            default:
                return ZenPropertyType_Invalid;
            }
        }
    };
}

#endif
