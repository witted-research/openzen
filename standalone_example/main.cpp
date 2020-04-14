//===========================================================================//
//
// Copyright (C) 2020 LP-Research Inc.
//
// This file is part of OpenZen, under the MIT License.
// See https://bitbucket.org/lpresearch/openzen/src/master/LICENSE for details
// SPDX-License-Identifier: MIT
//
//===========================================================================//


#include <iostream>

#include <OpenZen.h>

int main() {
    auto clientPair = zen::make_client();
    auto& clientError = clientPair.first;
    auto& client = clientPair.second;
    if (clientError) {
        std::cout << "OpenZen client could not be created" << std::endl;
        return clientError;
    }

    std::cout << "OpenZen client created successfully" << std::endl;
    client.close();
    std::cout << "OpenZen client closed successfully" << std::endl;

    return 0;
}
