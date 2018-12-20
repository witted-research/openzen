#ifndef ZEN_API_PROTOCOL_H_
#define ZEN_API_PROTOCOL_H_

#include <cstdint>

typedef enum EZenProtocolFunction : uint8_t
{
    ZenProtocolFunction_Invalid = 0,

    ZenProtocolFunction_Execute = 1,
    ZenProtocolFunction_Get = 2,
    ZenProtocolFunction_Set = 3,
    ZenProtocolFunction_Ack = 4,
    ZenProtocolFunction_Result = 5,

    ZenProtocolFunction_Max
} EZenProtocolFunction;

#endif