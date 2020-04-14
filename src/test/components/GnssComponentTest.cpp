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

#include "components/SensorParsingUtil.h"

#include <vector>
#include <iomanip>
#include <iostream>

TEST(GnssComponent, parseCoordinateResolution) {
    // check if our conversion from hardware int32 can resolve 1 centimeter
    // in the converted WGS84 coordinates
    int32_t latHw = 356635894;
    int32_t lonHw = 1397242735;
    
    const auto latFloat = zen::sensor_parsing_util::integerToScaledDouble(latHw, -7);
    const auto lonFloat = zen::sensor_parsing_util::integerToScaledDouble(lonHw, -7);

    int32_t latHw_move1 = 356635894 + 1; // adding 1 shifts the position by around 1 cm
    int32_t lonHw_move1 = 1397242735 + 1;

    const auto latFloat_move1 = zen::sensor_parsing_util::integerToScaledDouble(latHw_move1, -7);
    const auto lonFloat_move1 = zen::sensor_parsing_util::integerToScaledDouble(lonHw_move1, -7);

    // cannot be the same if the last digit of the integer was honored in the
    // conversion
    ASSERT_FALSE(latFloat_move1 == latFloat);
    ASSERT_FALSE(lonFloat_move1 == lonFloat);

    // check if the relative distance was conserved
    ASSERT_NEAR(0.0000001, latFloat_move1 - latFloat, 0.00000001);
    ASSERT_NEAR(0.0000001, lonFloat_move1 - lonFloat, 0.00000001);
}