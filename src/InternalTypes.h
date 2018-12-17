#ifndef ZEN_INTERNALTYPES_H_
#define ZEN_INTERNALTYPES_H_

#include <cstdint>
#include <optional>

using DeviceProperty_t = uint8_t;

enum class EDevicePropertyInternal : DeviceProperty_t
{
    Ack = 0,
    Nack = 1,
    UpdateFirmware = 2,
    UpdateIAP = 3,
    Config = 4,

    Max
};

enum class EDevicePropertyV0 : DeviceProperty_t
{
    Ack = 0,
    Nack = 1,

    UpdateFirmware = 2,
    UpdateIAP = 3,

    GetConfig = 4,                      // uint32_t (bitset)
    GetStatus = 5,                      // uint32_t (bitset)

    SetCommandMode = 6,                 // uint32_t
    SetStreamMode = 7,                  // uint32_t

    GetRawSensorData = 9,               // uint8[]
    SetTransmitData = 10,               // uint32_t (bitset)
    SetSamplingRate = 11,               // uint32_t

    WriteRegisters = 15,                // void
    RestoryFactorySettings = 16,        // void

    SetOrientationOffsetMode = 18,      // uint32_t

    StartGyroCalibration = 22,          // void
    SetGyrUseAutoCalibration = 23,      // bool
    SetGyrUseThreshold = 24,            // uint32?
    SetGyrRange = 25,                   // uint32
    GetGyrRange = 26,                   // uint32
    SetAccBias = 27,                    // float[3]
    GetAccBias = 28,                    // float[3]
    SetAccAlignment = 29,               // float[9]
    GetAccAlignment = 30,               // float[9]
    SetAccRange = 31,                   // uint32
    GetAccRange = 32,                   // uint32
    SetMagRange = 33,                   // uint32
    GetMagRange = 34,                   // uint32
    SetMagHardIronOffset = 35,          // float[3]
    GetMagHardIronOffset = 36,          // float[3]
    SetMagSoftIronMatrix = 37,          // float[9]
    GetMagSoftIronMatrix = 38,          // float[9]
    SetFieldRadius = 39,                // float
    GetFieldRadius = 40,                // float
    SetFilterMode = 41,                 // uint32
    GetFilterMode = 42,                 // uint32
    SetFilterPreset = 43,               // uint32
    GetFilterPreset = 44,               // uint32
    SetStreamFormat = 45,

    GetFirmwareVersion = 47,            // int[3]
    SetGyrBias = 48,                    // float[3]
    GetGyrBias = 49,                    // float[3]
    SetGyrAlignment = 50,               // float[9]
    GetGyrAlignment = 51,               // float[9]

    SetTimestamp = 66,                  // uint32
    SetLinearCompensationRate = 67,     // uint32
    GetLinearCompensationRate = 68,     // uint32
    SetCentricCompensationRate = 69,    // float
    GetCentricCompensationRate = 70,    // float

    SetDataMode = 75,
    SetMagAlignment = 76,               // float[9]
    SetMagBias = 77,                    // float[3]
    SetMagReference = 78,               // float[3]
    GetMagAlignment = 79,               // float[9]
    GetMagBias = 80,                    // float[3]
    GetMagReference = 81,               // float[3]
    ResetOrientationOffset = 82,        // void

    SetBaudRate = 84,
    GetBaudRate = 85,

    GetBatteryLevel = 87,               // float
    GetBatteryVoltage = 88,             // float
    GetBatteryCharging = 89,            // uint32
    GetSerialNumber = 90,               // char[24]
    GetDeviceName = 91,                 // char[16]
    GetFirmwareInfo = 92,               // char[16]
    
    GetPing = 98                        // uint32
};

#endif