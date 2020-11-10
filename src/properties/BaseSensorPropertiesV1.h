//===========================================================================//
//
// Copyright (C) 2020 LP-Research Inc.
//
// This file is part of OpenZen, under the MIT License.
// See https://bitbucket.org/lpresearch/openzen/src/master/LICENSE for details
// SPDX-License-Identifier: MIT
//
//===========================================================================//

#ifndef ZEN_SENSOR_PROPERTIES_BASESENSORPROPERTIESV1_H_
#define ZEN_SENSOR_PROPERTIES_BASESENSORPROPERTIESV1_H_

#include "InternalTypes.h"
#include "ZenTypes.h"

#include <array>
#include <optional>

namespace zen
{
    namespace base::v1
    {
        namespace internal
        {
            constexpr std::optional<EDevicePropertyInternal> map(uint16_t function)
            {
                if (function <= static_cast<uint16_t>(EDevicePropertyInternal::Nack))
                    return static_cast<EDevicePropertyInternal>(function);
                else if (function == static_cast<uint16_t>(EDevicePropertyV1::GetImuTransmitData))
                    return EDevicePropertyInternal::ConfigImuOutputDataBitset;
                else if (function == static_cast<uint16_t>( EDevicePropertyV1::GetGpsTransmitData))
                    return EDevicePropertyInternal::ConfigGpsOutputDataBitset;
                else if (function == static_cast<uint16_t>( EDevicePropertyV1::GetDegGradOutput))
                    return EDevicePropertyInternal::ConfigGetDegGradOutput;

                return {};
            }
        }

        constexpr EDevicePropertyV1 mapCommand(ZenProperty_t command)
        {
            switch (command)
            {
            case ZenSensorProperty_StoreSettingsInFlash:
                return EDevicePropertyV1::WriteRegisters;

            case ZenSensorProperty_RestoreFactorySettings:
                return EDevicePropertyV1::RestoreFactorySettings;

            default:
                return EDevicePropertyV1::Ack;
            }
        }

        constexpr EDevicePropertyV1 map(ZenProperty_t property, bool isGetter)
        {
            const auto get_or = [isGetter](EDevicePropertyV1 prop) {
                return isGetter ? prop : EDevicePropertyV1::Ack;
            };

            switch (property)
            {
            case ZenSensorProperty_DeviceName:
                return get_or(EDevicePropertyV1::GetSensorModel);

            case ZenSensorProperty_FirmwareInfo:
                return get_or(EDevicePropertyV1::GetFirmwareInfo);

            case ZenSensorProperty_FirmwareVersion:
                return get_or(EDevicePropertyV1::GetFirmwareInfo);

            case ZenSensorProperty_SerialNumber:
                return get_or(EDevicePropertyV1::GetSerialNumber);

            case ZenSensorProperty_SensorModel:
                return get_or(EDevicePropertyV1::GetSensorModel);

            default:
                return EDevicePropertyV1::Ack;
            }
        }
    }
}

#endif
