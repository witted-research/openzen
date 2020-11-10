//===========================================================================//
//
// Copyright (C) 2020 LP-Research Inc.
//
// This file is part of OpenZen, under the MIT License.
// See https://bitbucket.org/lpresearch/openzen/src/master/LICENSE for details
// SPDX-License-Identifier: MIT
//
//===========================================================================//

#ifndef ZEN_ZENTYPESHELPER_H_
#define ZEN_ZENTYPESHELPER_H_

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

    inline void radToDeg3(float * v) {
        const auto r2d = 180.0f / 3.14159265359f;
        v[0] *= r2d;
        v[1] *= r2d;
        v[2] *= r2d;
    }

    /**
    Set all data fields of the imu data class to zero or sensible values
    */
    inline void imuDataReset(ZenImuData& imuData)
    {
        imuData.frameCount = 0;
        imuData.timestamp = 0.;
        vec3Zero(imuData.a);
        vec3Zero(imuData.g);
        vec3Zero(imuData.b);
        vec3Zero(imuData.aRaw);
        vec3Zero(imuData.gRaw);
        vec3Zero(imuData.bRaw);
        vec3Zero(imuData.w);
        vec3Zero(imuData.r);
        quatIdentity(imuData.q);
        mat3x3fIdentity(imuData.rotationM);
        mat3x3fIdentity(imuData.rotOffsetM);
        imuData.pressure = 0.f;
        vec3Zero(imuData.linAcc);
        imuData.gTemp = 0.f;
        imuData.altitude = 0.f;
        imuData.temperature = 0.f;
        imuData.heaveMotion = 0.f;
    }

    /**
    Set all data fields of the gnss data class to zero or sensible values
    */
    inline void gnssDataReset(ZenGnssData& gnssData)
    {
        gnssData.frameCount = 0;
        gnssData.timestamp = 0.0f;
        gnssData.latitude = 0.0f;
        gnssData.horizontalAccuracy = 0.0f;
        gnssData.longitude = 0.0f;
        gnssData.verticalAccuracy = 0.0f;
        gnssData.height = 0.0f;
        gnssData.headingOfMotion = 0.0f;
        gnssData.headingOfVehicle = 0.0f;
        gnssData.headingAccuracy = 0.0f;
        gnssData.velocity = 0.0f;
        gnssData.velocityAccuracy = 0.0f;
        gnssData.fixType = ZenGnssFixType_NoFix;
        gnssData.carrierPhaseSolution = ZenGnssFixCarrierPhaseSolution_None;
        gnssData.numberSatellitesUsed = 0;
        gnssData.year = 0;
        gnssData.month = 0;
        gnssData.day = 0;
        gnssData.hour = 0;
        gnssData.minute = 0;
        gnssData.second = 0;
        gnssData.nanoSecondCorrection = 0;
    }
}

#endif
