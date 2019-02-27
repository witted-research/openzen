#ifndef ZEN_API_ZENTYPES_H_
#define ZEN_API_ZENTYPES_H_

#include <cstdint>

#ifdef _WIN32

#if defined(ZEN_API_STATIC) || !defined(__cplusplus)
#define ZEN_API
#elif defined(ZEN_API_EXPORT)
#define ZEN_API extern "C" __declspec(dllexport)
#else
#define ZEN_API extern "C" __declspec(dllimport)
#endif

#elif defined(__GNUC__) || defined(COMPILER_GCC) || defined(__APPLE__)

#ifdef ZEN_API_STATIC
#define ZEN_API
#elif defined(ZEN_API_EXPORT)
#define ZEN_API extern "C" __attribute__((visibility("default")))
#else
#define ZEN_API extern "C"
#endif

#else

#error "This platform is not supported, yet."

#endif

#ifdef _WIN32
#define ZEN_CALLTYPE __cdecl
#else
#define ZEN_CALLTYPE
#endif

typedef struct ZenClientHandle { uintptr_t handle; } ZenClientHandle_t;
typedef struct ZenSensorHandle { uintptr_t handle; } ZenSensorHandle_t;
typedef struct ZenComponentHandle { uintptr_t handle; } ZenComponentHandle_t;

typedef uint32_t ZenError_t;

typedef enum ZenError : ZenError_t
{
    ZenError_None = 0,                       // None
    ZenError_Unknown = 1,                    // Unknown

    ZenError_IsNull = 10,                    // Pointer is invalid (null)
    ZenError_NotNull = 11,                   // Expected a null pointer
    ZenError_WrongDataType = 12,             // Wrong data type
    ZenError_BufferTooSmall = 13,            // Provided buffer is too small for the return data
    ZenError_InvalidArgument = 14,           // An invalid argument was provided

    ZenError_AlreadyInitialized = 20,        // Already initialized
    ZenError_NotInitialized = 21,            // Not initialized

    ZenError_Device_IoTypeInvalid = 30,         // Invalid IO type
    ZenError_Sensor_VersionNotSupported = 31,   // Sensor version is not supported
    ZenError_Device_ListingFailed = 32,         // An error occured while listing devices
    ZenError_Device_Listing = 35,               // Busy listing devices

    ZenError_WrongSensorType = 40,           // Wrong sensor type
    ZenError_WrongIoType = 41,               // Wrong IO type
    ZenError_UnknownDeviceId = 42,           // Unknown device ID

    ZenError_Io_AlreadyInitialized = 800,    // IO interface was already initialized
    ZenError_Io_NotInitialized = 801,        // IO interface is not initialized
    ZenError_Io_InitFailed = 802,            // Failed to open IO interface
    ZenError_Io_DeinitFailed = 803,          // Failed to deinitialize IO interface
    ZenError_Io_ReadFailed = 804,            // Failed to read from IO interface
    ZenError_Io_SendFailed = 805,            // Failed to send to IO interface
    ZenError_Io_GetFailed = 806,             // Failed to get value from IO interface
    ZenError_Io_SetFailed = 807,             // Failed to set value on IO interface
    ZenError_Io_Busy = 811,                  // IO Interface is busy sending/receiving another message. Try again later
    ZenError_Io_Timeout = 812,               // IO Interface timed out. Try again.
    ZenError_Io_UnexpectedFunction = 813,    // IO Interface received an unexpected function
    ZenError_Io_UnsupportedFunction = 814,   // IO Interface received an unsupported function
    ZenError_Io_MsgCorrupt = 815,            // Received message is corrupt
    ZenError_Io_MsgTooBig = 816,             // Trying to send too much data over CAN interface
    ZenError_Io_ExpectedAck = 820,           // IO state manager did not receive an ACK or NACK message
    ZenError_Io_BaudratesUnknown = 821,      // IO Interface does not know supported baudrates values

    ZenError_UnknownProperty = 850,          // Sensor does not support the property
    ZenError_UnknownCommandMode = 851,       // Sensor does not support the command mode
    ZenError_UnsupportedEvent = 852,        // Host does not support the event type

    // [XXX] What to do with other errors in the hardware?
    ZenError_FW_FunctionFailed = 900,        // Firmware failed to execute the requested function

    ZenError_Can_BusError = 1001,            // Can interface is in an error state
    ZenError_Can_OutOfAddresses = 1002,      // Can interface cannot support any more devices on the channel. Close another device first
    ZenError_Can_ResetFailed = 1006,         // Failed to reset queues of CAN interface
    ZenError_Can_AddressOutOfRange = 1009,   // Trying to send message to an address that is too big (max: 255)

    ZenError_InvalidClientHandle = 2000,    // Invalid client handle
    ZenError_InvalidSensorHandle = 2001,    // Invalid sensor handle
    ZenError_InvalidComponentHandle = 2002, // Invalid component handle

    ZenError_Max
} ZenError;

typedef enum ZenSensorInitError : ZenError_t
{
    ZenSensorInitError_None = 0,

    ZenSensorInitError_InvalidHandle,           // Provided client handle is invalid
    ZenSensorInitError_IsNull,                  // Provided pointer is null
    ZenSensorInitError_UnsupportedComponent,    // At least one of the sensor's component types is not supported by the host
    ZenSensorInitError_UnsupportedDataFormat,   // Provided Modbus Format is not supported
    ZenSensorInitError_UnsupportedIoType,       // Provided IO type is not supported
    ZenSensorInitError_UnsupportedProtocol,     // The sensor's protocol version is not supported by the host

    ZenSensorInitError_ConnectFailed,           // Failed to establish a connection with the sensor
    ZenSensorInitError_IoFailed,                // Low-level IO API returned an error
    ZenSensorInitError_RetrieveFailed,          // Failed to retrieve a property from the sensor
    ZenSensorInitError_SetBaudRateFailed,       // Failed to change the BaudRate
    ZenSensorInitError_SendFailed,              // Failed to send data
    ZenSensorInitError_IncompatibleBaudRates,   // Unable to find a compatible BaudRate
    ZenSensorInitError_InvalidAddress,          // Provided remote address is invalid
    ZenSensorInitError_InvalidConfig,           // The configuration file is invalid

    ZenSensorInitError_Max
} ZenSensorInitError;

typedef enum ZenAsyncStatus
{
    ZenAsync_Finished,

    ZenAsync_ThreadBusy,
    ZenAsync_InvalidArgument,
    ZenAsync_Updating,
    ZenAsync_Failed,

    ZenAsync_Max
} ZenAsyncStatus;

typedef struct ZenHeaveMotionData
{
    float yHeave;
} ZenHeaveMotionData;

typedef struct ZenImuData
{
    /// Calibrated accelerometer sensor data.
    float a[3];

    /// Calibrated gyroscope sensor data.
    float g[3];

    /// Calibrated magnetometer sensor data.
    float b[3];

    /// Raw accelerometer sensor data.
    float aRaw[3];

    /// Raw gyroscope sensor data.
    float gRaw[3];

    /// Raw magnetometer sensor data.
    float bRaw[3];

    /// Angular velocity data.
    float w[3];

    /// Euler angle data.
    float r[3];

    /// Quaternion orientation data.
    float q[4];

    /// Orientation data as rotation matrix without offset.
    float rotationM[9];

    /// Orientation data as rotation matrix after zeroing.
    float rotOffsetM[9];

    /// Barometric pressure.
    float pressure;

    /// Index of the data frame.
    int frameCount;

    /// Linear acceleration x, y and z.
    float linAcc[3];

    /// Gyroscope temperature.
    float gTemp;

    /// Altitude.
    float altitude;

    /// Temperature.
    float temperature;

    /// Sampling time of the data.
    float timestamp;

    ZenHeaveMotionData hm;
} ZenImuData;

typedef ZenImuData ZenEventData_Imu;

typedef struct ZenSensorDesc
{
    char name[256];
    char serialNumber[64];
    char ioType[64];
    union
    {
        uint32_t handle32;
        uint64_t handle64;
    };
} ZenSensorDesc;

typedef ZenSensorDesc ZenEventData_SensorFound;

typedef struct ZenEventData_SensorListingProgress
{
    float progress;
} ZenEventData_SensorListingProgress;

typedef union
{
    ZenEventData_Imu imuData;
    ZenEventData_SensorFound sensorFound;
    ZenEventData_SensorListingProgress sensorListingProgress;
} ZenEventData;

typedef uint32_t ZenEvent_t;

typedef struct ZenEvent
{
    ZenEvent_t eventType;
    ZenSensorHandle_t sensor;
    ZenComponentHandle_t component;
    ZenEventData data;
} ZenEvent;

typedef enum ZenSensorEvent : ZenEvent_t
{
    ZenSensorEvent_None = 0,

    ZenSensorEvent_SensorFound = 1,
    ZenSensorEvent_SensorListingProgress = 2,

    // Sensors are free to expose private events in this reserved region
    ZenSensorEvent_SensorSpecific_Start = 10000,
    ZenSensorEvent_SensorSpecific_End = 19999,

    ZenSensorEvent_Max
} ZenSensorEvent;

typedef enum ZenImuEvent : ZenEvent_t
{
    ZenImuEvent_None = 0,

    ZenImuEvent_Sample = 1,

    // Components are free to expose private events in this reserved region
    ZenImuEvent_ComponentSpecific_Start = 10000,
    ZenImuEvent_ComponentSpecific_End = 19999,

    ZenImuEvent_Max
} ZenImuEvent;

typedef uint32_t ZenProperty_t;

typedef enum EZenSensorProperty : ZenProperty_t
{
    ZenSensorProperty_Invalid = 0,

    ZenSensorProperty_DeviceName = 1000,         // char[16]
    ZenSensorProperty_FirmwareInfo,              // char[16]
    ZenSensorProperty_FirmwareVersion,           // int[3]
    ZenSensorProperty_RestoreFactorySettings,    // void
    ZenSensorProperty_SerialNumber,              // char[24]
    ZenSensorProperty_StoreSettingsInFlash,      // void

    ZenSensorProperty_BatteryCharging,           // bool
    ZenSensorProperty_BatteryLevel,              // float
    ZenSensorProperty_BatteryVoltage,            // float

    ZenSensorProperty_BaudRate,                  // int
    ZenSensorProperty_SupportedBaudRates,        // int[]

    ZenSensorProperty_DataMode,                  // int (0: 32-bit float, 1: 16-bit fixed)
    ZenSensorProperty_TimeOffset,                // int

    // Sensors are free to expose private properties in this reserved region
    ZenSensorProperty_SensorSpecific_Start = 10000,
    ZenSensorProperty_SensorSpecific_End = 19999,

    ZenSensorProperty_Max
} EZenSensorProperty;

typedef enum EZenImuProperty : ZenProperty_t
{
    ZenImuProperty_Invalid = 0,

    ZenImuProperty_StreamData = 1000,           // bool
    ZenImuProperty_SamplingRate,                // int
    ZenImuProperty_SupportedSamplingRates,      // int[]

    ZenImuProperty_PollSensorData,               // void - Manually request sensor data (when not streaming)
    ZenImuProperty_CalibrateGyro,                // void - Start gyro calibration
    ZenImuProperty_ResetOrientationOffset,       // void - Resets the orientation's offset

    ZenImuProperty_CentricCompensationRate,      // float
    ZenImuProperty_LinearCompensationRate,       // float

    ZenImuProperty_FieldRadius,                  // float
    ZenImuProperty_FilterMode,                   // int
    ZenImuProperty_SupportedFilterModes,         // char[]
    ZenImuProperty_FilterPreset,                 // int (future: float acc_covar, mag_covar)

    ZenImuProperty_OrientationOffsetMode,        // int

    ZenImuProperty_AccAlignment,                 // matrix
    ZenImuProperty_AccBias,                      // float[3]
    ZenImuProperty_AccRange,                     // int
    ZenImuProperty_AccSupportedRanges,           // int[]

    ZenImuProperty_GyrAlignment,                 // matrix
    ZenImuProperty_GyrBias,                      // float[3]
    ZenImuProperty_GyrRange,                     // int
    ZenImuProperty_GyrSupportedRanges,           // int[]
    ZenImuProperty_GyrUseAutoCalibration,        // bool
    ZenImuProperty_GyrUseThreshold,              // bool

    ZenImuProperty_MagAlignment,                 // matrix
    ZenImuProperty_MagBias,                      // float[3]
    ZenImuProperty_MagRange,                     // int
    ZenImuProperty_MagSupportedRanges,           // int[]
    ZenImuProperty_MagReference,                 // float[3]
    ZenImuProperty_MagHardIronOffset,            // float[3]
    ZenImuProperty_MagSoftIronMatrix,            // matrix

    ZenImuProperty_OutputLowPrecision,           // bool
    ZenImuProperty_OutputRawAcc,                 // bool
    ZenImuProperty_OutputRawGyr,                 // bool
    ZenImuProperty_OutputRawMag,                 // bool
    ZenImuProperty_OutputEuler,                  // bool
    ZenImuProperty_OutputQuat,                   // bool
    ZenImuProperty_OutputAngularVel,             // bool
    ZenImuProperty_OutputLinearAcc,              // bool
    ZenImuProperty_OutputHeaveMotion,            // bool
    ZenImuProperty_OutputAltitude,               // bool
    ZenImuProperty_OutputPressure,               // bool
    ZenImuProperty_OutputTemperature,            // bool

    ZenImuProperty_Max
} EZenImuProperty;

typedef enum ZenOrientationOffsetMode
{
    ZenOrientationOffsetMode_Object = 0,     // Object mode
    ZenOrientationOffsetMode_Heading = 1,    // Heading mode
    ZenOrientationOffsetMode_Alignment = 2,  // Alignment mode

    ZenOrientationOffsetMode_Max
} ZenOrientationOffsetMode;

typedef enum ZenPropertyType
{
    ZenPropertyType_Invalid = 0,

    ZenPropertyType_Bool = 1,
    ZenPropertyType_Float = 2,
    ZenPropertyType_Int32 = 3,
    ZenPropertyType_UInt64 = 4,
    ZenPropertyType_String = 5,

    ZenPropertyType_Matrix = 10,

    ZenPropertyType_Json = 40,


    ZenPropertyType_Max
} ZenPropertyType;

typedef struct ZenMatrix3x3f
{
    float data[9];
} ZenMatrix3x3f;

static const char g_zenSensorType_Imu[] = "imu";

#endif
