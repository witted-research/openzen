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
    // this id is used for the OutputDataBitset when loading the initial
    // sensor configuration
    Config = 4,
    // this id is used for the GPS OutputDataBitset
    ConfigGpsOutputDataBitset = 5,

    Max
};

// Legacy device commands, for example LPMS-CU2, LPMS-B2
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
    RestoreFactorySettings = 16,        // void

    SetOrientationOffsetMode = 18,      // uint32_t

    StartGyroCalibration = 22,          // void
    SetGyrUseAutoCalibration = 23,      // bool
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
    
    GetPing = 98,                       // uint32

    SetCanBaudrate = 46,				// uint32
    SetCanMapping = 62,					// uint32
    GetCanMapping = 63,					// uint32
    SetCanHeartbeat = 64,				// uint32
    GetCanHeartbeat = 65,				// uint32
    GetCanConfiguration = 71,			// uint32
    SetCanChannelMode = 72,				// uint32
    SetCanPointMode = 73,				// uint32
    SetCanStartId = 74,					// uint32

    SetUartBaudrate = 84,				// uint32
    GetUartBaudrate = 85,				// uint32
    SetUartFormat = 86					// uint32
};

// IG1 device commands
// commands referenced in this document
// https://lp-research.com/wp-content/uploads/2019/09/LpmsIG1-seriesReferenceManual20190828.pdf
enum class EDevicePropertyV1 : DeviceProperty_t
{
    Ack = 0,
    Nack = 1,

    UpdateFirmware = 2,
    UpdateIAP = 3,

    WriteRegisters = 4,                // void
    RestoreFactorySettings = 5,        // void

    GotoCommandMode = 6,                // none
    GotoStreamMode = 7,                 // none

    GetSensorStatus = 8,                // uint32

    GetRawImuSensorData = 9,            // uint8[]
    GetRawGpsSensorData = 10,           // uint8[]

    GetSensorModel = 20,                // char[24]
    GetFirmwareInfo = 21,               // char[24]
    GetSerialNumber = 22,               // char[24]

    GetFilterVersion = 23,              // char[24]

    SetImuTransmitData = 30,            // uint32
    GetImuTransmitData = 31,            // uint32

    SetImuId = 32,                      // uint32
    GetImuId = 33,                      // uint32

    SetStreamFreq = 34,                 // uint32
    GetStreamFreq = 35,                 // uint32

    SetDegGradOutput = 36,              // uint32
    GetDegGradOutput = 37,              // uint32

    SetOrientationOffsetMode = 38,      // uint32
    ResetOrientationOffset = 39,        // void

    SetAccRange = 50,                   // uint32
    GetAccRange = 51,                   // uint32

    SetGyrRange = 60,                   // uint32
    GetGyrRange = 61,                   // uint32

    StartGyroCalibration = 62,          // void
    SetEnableGyrAutoCalibration = 64,   // bool
    GetEnableGyrAutoCalibration = 65,   // bool

    SetGyrThreshold = 66,               // float
    GetGyrThreshold = 67,               // float

    SetMagRange = 70,                   // uint32
    GetMagRange = 71,                   // uint32

    StartMagCalibration = 84,           // void
    StopMagCalibration = 85,            // void
    SetMagCalibrationTimeout = 86,      // uint32
    GetMagCalibrationTimeout = 87,      // uint32

    SetFilterMode = 90,                 // uint32
    GetFilterMode = 91,                 // uint32

    SetCanStartId = 110,                 // uint32
    GetCanStartId = 111,                 // uint32
    SetCanBaudRate = 112,               // uint32
    GetCanBaudRate = 113,               // uint32
    SetCanDataPrecision = 114,          // uint32
    GetCanDataPrecision = 115,          // uint32
    GetCanMode = 117,                   // uint32
    SetCanMapping = 118,                // uint32
    GetCanMapping = 119,                // uint32
    SetCanHeartbeat = 120,              // uint32
    GetCanHeartbeat = 121,              // uint32

    SetUartBaudrate = 130,              // uint32
    GetUartBaudrate = 131,              // uint32
    SetUartFormat = 132,                // uint32
    GetUartFormat = 133,                // uint32

    SetUartAsciiCharacter = 134,        // uint8
    GetUartAsciiCharacter = 135,        // uint8

    SetLpBusDataPrecision = 136,        // uint32
    GetLpBusDataPrecision = 137,        // uint32

    SetTimestamp = 152,                 // uint32

    SetGpsTransmitData = 160,           // uint32
    GetGpsTransmitData = 161,           // uint32

    SaveGpsState = 162,                 // void
    ClearGpsState = 163,                // void

    SetRtkCorrection = 166,                // void
};

#endif
