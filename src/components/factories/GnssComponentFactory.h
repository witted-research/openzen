//===========================================================================//
//
// Copyright (C) 2020 LP-Research Inc.
//
// This file is part of OpenZen, under the MIT License.
// See https://bitbucket.org/lpresearch/openzen/src/master/LICENSE for details
// SPDX-License-Identifier: MIT
//
//===========================================================================//

#ifndef ZEN_COMPONENTS_FACTORIES_GNSSCOMPONENTFACTORY_H_
#define ZEN_COMPONENTS_FACTORIES_GNSSCOMPONENTFACTORY_H_

#include "components/IComponentFactory.h"

namespace zen
{
    /**
    Initializes the GNSS component for an LPMS-IG1 sensor by loading the GPS output data flags
    and setting them in the newly create GnssComponent.
    */
    class GnssComponentFactory : public IComponentFactory
    {
    public:
        nonstd::expected<std::unique_ptr<SensorComponent>, ZenSensorInitError> make_component(
            unsigned int version,
            SpecialOptions options,
            uint8_t id,
            SyncedModbusCommunicator& communicator
        ) const noexcept override;
    };
}

#endif
