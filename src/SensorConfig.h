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

struct ComponentConfig
{
    uint32_t version;
    std::string id;
};

struct SensorConfig
{
    uint32_t version;
    std::vector<ComponentConfig> components;
};

#endif
