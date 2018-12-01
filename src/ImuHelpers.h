#ifndef ZEN_API_IMUHELPERS_H_
#define ZEN_API_IMUHELPERS_H_

#include "ZenTypes.h"

namespace zen
{
    inline void quatIdentity(float q[4])
    {
        q[0] = 1.f;
        q[1] = q[2] = q[3] = 0.f;
    }

    inline void vec3Zero(float v[3])
    {
        v[0] = v[1] = v[2] = 0.f;
    }

    inline void mat3x3fIdentity(float m[9])
    {
        for (auto i = 0; i < 3; ++i)
            for (auto j = 0; j < 3; ++j)
                m[i * 3 + j] = i == j ? 1.f : 0.f;
    }

    inline void imuDataReset(ZenImuData& imuData)
    {
        vec3Zero(imuData.a);
        vec3Zero(imuData.g);
        vec3Zero(imuData.b);
        vec3Zero(imuData.aRaw);
        vec3Zero(imuData.gRaw);
        vec3Zero(imuData.bRaw);
        vec3Zero(imuData.w);
        quatIdentity(imuData.q);
        mat3x3fIdentity(imuData.rotationM);
        mat3x3fIdentity(imuData.rotOffsetM);
        imuData.pressure = 0.f;
        imuData.frameCount = 0;
        vec3Zero(imuData.linAcc);
        imuData.gTemp = 0.f;
        imuData.altitude = 0.f;
        imuData.temperature = 0.f;
        imuData.timestamp = 0.f;
        imuData.hm.yHeave = 0.f;
    }
}

#endif
