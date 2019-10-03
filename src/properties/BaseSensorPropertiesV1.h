#ifndef ZEN_SENSOR_PROPERTIES_BASESENSORPROPERTIESV1_H_
#define ZEN_SENSOR_PROPERTIES_BASESENSORPROPERTIESV1_H_

#include "InternalTypes.h"
#include "ZenTypes.h"

#include <array>
#include <optional>

#define GET_OR_V1(x) isGetter ? (x) : EDevicePropertyV1::Ack
#define SET_OR_V1(x) isGetter ? EDevicePropertyV1::Ack : (x)
#define GET_SET_V1(x, y) isGetter ? (x) : (y)

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
                    return EDevicePropertyInternal::Config;
                else if (function == static_cast<uint16_t>( EDevicePropertyV1::GetGpsTransmitData))
                    return EDevicePropertyInternal::ConfigGpsOutputDataBitset;

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
            switch (property)
            {
            case ZenSensorProperty_DeviceName:
                return GET_OR_V1(EDevicePropertyV1::GetSensorModel);

            case ZenSensorProperty_FirmwareInfo:
                return GET_OR_V1(EDevicePropertyV1::GetFirmwareInfo);

            case ZenSensorProperty_FirmwareVersion:
                return GET_OR_V1(EDevicePropertyV1::GetFirmwareInfo);

            case ZenSensorProperty_SerialNumber:
                return GET_OR_V1(EDevicePropertyV1::GetSerialNumber);

            case ZenSensorProperty_SensorModel:
                return GET_OR_V1(EDevicePropertyV1::GetSensorModel);

            default:
                return EDevicePropertyV1::Ack;
            }
        }
    }
}

#endif
