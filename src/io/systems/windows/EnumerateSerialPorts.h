//===========================================================================//
//
// Copyright (C) 2020 LP-Research Inc.
//
// This file is part of OpenZen, under the MIT License.
// See https://bitbucket.org/lpresearch/openzen/src/master/LICENSE for details
// SPDX-License-Identifier: MIT
//
//===========================================================================//

#ifndef ZEN_IO_SYSTEMS_WINDOWS_ENUMERATESERIALPORTS_H_
#define ZEN_IO_SYSTEMS_WINDOWS_ENUMERATESERIALPORTS_H_

#include <string>
#include <vector>

namespace zen {
    bool EnumerateSerialPorts(std::vector<std::string>& ports);
}

#endif
