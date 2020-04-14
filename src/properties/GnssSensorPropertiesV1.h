//===========================================================================//
//
// Copyright (C) 2020 LP-Research Inc.
//
// This file is part of OpenZen, under the MIT License.
// See https://bitbucket.org/lpresearch/openzen/src/master/LICENSE for details
// SPDX-License-Identifier: MIT
//
//===========================================================================//

#ifndef ZEN_PROPERTIES_GNSSSENSORPROPERTIESV1_H_
#define ZEN_PROPERTIES_GNSSSENSORPROPERTIESV1_H_

#include <array>
#include <cstring>
#include <utility>

#include "InternalTypes.h"
#include "ZenTypes.h"

#define GET_OR(x) isGetter ? (x) : EDevicePropertyV1::Ack
#define SET_OR(x) isGetter ? EDevicePropertyV1::Ack : (x)
#define GET_SET(x, y) isGetter ? (x) : (y)

namespace zen
{
    namespace imu::v1
    {
        constexpr EDevicePropertyV1 map(ZenProperty_t property, bool isGetter)
        {
            switch (property)
            {
            default:
                return EDevicePropertyV1::Ack;
            }
        }
}

#endif
