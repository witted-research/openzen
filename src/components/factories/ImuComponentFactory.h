//===========================================================================//
//
// Copyright (C) 2020 LP-Research Inc.
//
// This file is part of OpenZen, under the MIT License.
// See https://bitbucket.org/lpresearch/openzen/src/master/LICENSE for details
// SPDX-License-Identifier: MIT
//
//===========================================================================//

#ifndef ZEN_COMPONENTS_FACTORIES_IMUCOMPONENTFACTORY_H_
#define ZEN_COMPONENTS_FACTORIES_IMUCOMPONENTFACTORY_H_

#include "components/IComponentFactory.h"

namespace zen
{
    class ImuComponentFactory : public IComponentFactory
    {
    public:
        nonstd::expected<std::unique_ptr<SensorComponent>, ZenSensorInitError> make_component(
            unsigned int version,
            SpecialOptions specialOptions,
            uint8_t id,
            SyncedModbusCommunicator& communicator
        ) const noexcept override;
    };
}

#endif
