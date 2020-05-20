//===========================================================================//
//
// Copyright (C) 2020 LP-Research Inc.
//
// This file is part of OpenZen, under the MIT License.
// See https://bitbucket.org/lpresearch/openzen/src/master/LICENSE for details
// SPDX-License-Identifier: MIT
//
//===========================================================================//

#ifndef ZEN_SENSORCONFIG_H_
#define ZEN_SENSORCONFIG_H_

#include <cstdint>
#include <string>

/**
 * Options for some differences in sensor component details that
 * need to be honored by the Sensor component
 */
enum SpecialOptions {
    SpecialOptions_None = 0,
    // The second Gyro value (in new IG1 protocol version) is the primary one
    // which should be reported to the user.
    // This is the case for LPMS-BE1 sensor
    SpecialOptions_SecondGyroIsPrimary = 1ul << 0
};

struct ComponentConfig
{
    uint32_t version;
    std::string id;
    SpecialOptions specialOptions = SpecialOptions_None;
};

struct SensorConfig
{
    uint32_t version;
    std::vector<ComponentConfig> components;
};

#endif
