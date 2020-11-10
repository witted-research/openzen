//===========================================================================//
//
// Copyright (C) 2020 LP-Research Inc.
//
// This file is part of OpenZen, under the MIT License.
// See https://bitbucket.org/lpresearch/openzen/src/master/LICENSE for details
// SPDX-License-Identifier: MIT
//
//===========================================================================//

#ifndef ZEN_SENSOR_PROPERTIES_BASESENSORPROPERTIESV0_H_
#define ZEN_SENSOR_PROPERTIES_BASESENSORPROPERTIESV0_H_

#include "InternalTypes.h"
#include "ZenTypes.h"

#include <array>
#include <optional>

namespace zen
{
    namespace base::v0
    {
        namespace internal
        {
            constexpr std::optional<EDevicePropertyInternal> map(uint8_t function)
            {
                if (function <= static_cast<uint8_t>(EDevicePropertyInternal::Nack))
                    return static_cast<EDevicePropertyInternal>(function);
                else if (function == static_cast<uint8_t>(EDevicePropertyInternal::ConfigImuOutputDataBitset))
                    return EDevicePropertyInternal::ConfigImuOutputDataBitset;

                return {};
            }
        }

        constexpr EDevicePropertyV0 mapCommand(ZenProperty_t command)
        {
            switch (command)
            {
            case ZenSensorProperty_StoreSettingsInFlash:
                return EDevicePropertyV0::WriteRegisters;

            case ZenSensorProperty_RestoreFactorySettings:
                return EDevicePropertyV0::RestoreFactorySettings;

            default:
                return EDevicePropertyV0::Ack;
            }
        }

        constexpr EDevicePropertyV0 map(ZenProperty_t property, bool isGetter)
        {
            const auto get_or = [isGetter](EDevicePropertyV0 prop) {
                return isGetter ? prop : EDevicePropertyV0::Ack;
            };
            const auto set_or = [isGetter](EDevicePropertyV0 prop) {
                return isGetter ? EDevicePropertyV0::Ack : prop;
            };
            const auto get_set = [isGetter] (EDevicePropertyV0 x, EDevicePropertyV0 y) {
                return isGetter ? x : y;
            };

            switch (property)
            {
            case ZenSensorProperty_DeviceName:
                return get_or(EDevicePropertyV0::GetDeviceName);

            case ZenSensorProperty_FirmwareInfo:
                return get_or(EDevicePropertyV0::GetFirmwareInfo);

            case ZenSensorProperty_FirmwareVersion:
                return get_or(EDevicePropertyV0::GetFirmwareVersion);

            case ZenSensorProperty_SerialNumber:
                return get_or(EDevicePropertyV0::GetSerialNumber);

            case ZenSensorProperty_BatteryCharging:
                return get_or(EDevicePropertyV0::GetBatteryCharging);

            case ZenSensorProperty_BatteryLevel:
                return get_or(EDevicePropertyV0::GetBatteryLevel);

            case ZenSensorProperty_BatteryVoltage:
                return get_or(EDevicePropertyV0::GetBatteryVoltage);

            case ZenSensorProperty_DataMode:
                return set_or(EDevicePropertyV0::SetDataMode);

            case ZenSensorProperty_TimeOffset:
                return get_set(EDevicePropertyV0::GetPing, EDevicePropertyV0::SetTimestamp);

            default:
                return EDevicePropertyV0::Ack;
            }
        }
    }
}

#endif
