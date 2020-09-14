//===========================================================================//
//
// Copyright (C) 2020 LP-Research Inc.
//
// This file is part of OpenZen, under the MIT License.
// See https://bitbucket.org/lpresearch/openzen/src/master/LICENSE for details
// SPDX-License-Identifier: MIT
//
//===========================================================================//

#include <gtest/gtest.h>

#include "ZenTypes.h"

#include "streaming/ZenTypesSerialization.h"

#include <cereal/cereal.hpp>
#include <cereal/archives/binary.hpp>
#include <sstream>

template <class TArray>
void checkArray3(TArray& a1, TArray& a2) {
    ASSERT_EQ(a1[0], a2[0]);
    ASSERT_EQ(a1[1], a2[1]);
    ASSERT_EQ(a1[2], a2[2]);
}

TEST(Serialization, serializeAndDeserializeImu) {
    std::stringstream buffer;

    ZenImuData imuData;

    imuData.a[0] = 23.0f;
    imuData.a[1] = 24.0f;
    imuData.a[2] = 25.0f;

    imuData.g[0] = 0.004f;
    imuData.g[1] = 0.005f;
    imuData.g[2] = 0.006f;

    imuData.altitude = 55.44f;

    imuData.w[0] = 1.0f;
    imuData.w[1] = 2.0f;
    imuData.w[2] = 3.0f;

    imuData.heaveMotion = 55.1f;

    {
        cereal::BinaryOutputArchive imu_archive(buffer);
        imu_archive(imuData);
    }

    cereal::BinaryInputArchive imu_archive_input(buffer);

    ZenImuData imuDataLoaded;

    imu_archive_input(imuDataLoaded);

    checkArray3(imuData.a, imuDataLoaded.a);
    ASSERT_EQ(imuData.altitude, imuDataLoaded.altitude);
    checkArray3(imuData.g, imuDataLoaded.g);
    checkArray3(imuData.w, imuDataLoaded.w);
    ASSERT_EQ(imuData.heaveMotion, imuDataLoaded.heaveMotion);
}

TEST(Serialization, serializeAndDeserializeGps) {
    std::stringstream buffer;

    ZenGnssData gnssData;
    // this is the precision the IG1 hardware provides
    // make sure that is retained in a serialization
    gnssData.latitude = 35.6635894;
    gnssData.longitude = 139.7242735;
    gnssData.fixType = ZenGnssFixType::ZenGnssFixType_3dFix;
    gnssData.carrierPhaseSolution = ZenGnssFixCarrierPhaseSolution::ZenGnssFixCarrierPhaseSolution_FixedAmbiguities;

    // creal archive must be destroyed to guarantee the complete serialization
    {
        cereal::BinaryOutputArchive imu_archive(buffer);
        imu_archive(gnssData);
    }

    ZenGnssData gnssDataLoaded;
    {
        cereal::BinaryInputArchive imu_archive_input(buffer);
        imu_archive_input(gnssDataLoaded);
    }

    ASSERT_EQ(gnssData.latitude, gnssDataLoaded.latitude);
    ASSERT_EQ(gnssData.longitude, gnssDataLoaded.longitude);
    ASSERT_EQ(gnssData.fixType, gnssDataLoaded.fixType);
    ASSERT_EQ(gnssData.carrierPhaseSolution, gnssDataLoaded.carrierPhaseSolution);
}