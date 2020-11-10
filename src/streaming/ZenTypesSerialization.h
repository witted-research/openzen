//===========================================================================//
//
// Copyright (C) 2020 LP-Research Inc.
//
// This file is part of OpenZen, under the MIT License.
// See https://bitbucket.org/lpresearch/openzen/src/master/LICENSE for details
// SPDX-License-Identifier: MIT
//
//===========================================================================//

#ifndef ZEN_TYPES_SERIALIZATION_H_
#define ZEN_TYPES_SERIALIZATION_H_

#include "ZenTypes.h"

namespace zen {
    namespace Serialization {
        /**
        Cannot use unions in serialization, therefore dedicated wrapper classes
        for the serialization
        */
        class ZenEventSerializationBase {
        public:
            uint64_t sensor;
            uint64_t component;
        };

        class ZenEventImuSerialization : public ZenEventSerializationBase {
        public:
            ZenImuData data;
        };

        class ZenEventGnssSerialization : public ZenEventSerializationBase {
        public:
            ZenGnssData data;
        };

        template <class Archive>
        void serialize(Archive& ar, ZenEventImuSerialization& wrapper) {
            ar(wrapper.sensor);
            ar(wrapper.component);
            ar(wrapper.data);
        }

        template <class Archive>
        void serialize(Archive& ar, zen::Serialization::ZenEventGnssSerialization& wrapper) {
            ar(wrapper.sensor);
            ar(wrapper.component);
            ar(wrapper.data);
        }
    }
}

template <class Archive>
void serialize(Archive& ar, ZenSensorHandle_t& sensorHandle)
{
    ar(sensorHandle.handle);
}

template <class Archive>
void serialize(Archive& ar, ZenComponentHandle_t& componentHandle)
{
    ar(componentHandle.handle);
}

template <class Archive>
void serialize(Archive& ar, ZenImuData & imuData)
{
    ar(imuData.frameCount,
        imuData.timestamp,
        imuData.a,
        imuData.g,
        imuData.b,
        imuData.aRaw,
        imuData.gRaw,
        imuData.bRaw,
        imuData.w,
        imuData.r,
        imuData.q,
        imuData.rotationM,
        imuData.rotOffsetM,
        imuData.pressure,
        imuData.linAcc,
        imuData.gTemp,
        imuData.altitude,
        imuData.temperature,
        imuData.heaveMotion);
}

template <class Archive>
void serialize(Archive& ar, ZenGnssData& gnssData)
{
    ar(gnssData.frameCount,
        gnssData.timestamp,
        gnssData.latitude,
        gnssData.horizontalAccuracy,
        gnssData.longitude,
        gnssData.verticalAccuracy,
        gnssData.height,
        gnssData.headingOfMotion,
        gnssData.headingOfVehicle,
        gnssData.headingAccuracy,
        gnssData.velocity,
        gnssData.velocityAccuracy,
        gnssData.fixType,
        gnssData.carrierPhaseSolution,
        gnssData.numberSatellitesUsed,
        gnssData.year,
        gnssData.month,
        gnssData.day,
        gnssData.hour,
        gnssData.minute,
        gnssData.second,
        gnssData.nanoSecondCorrection);
}

#endif