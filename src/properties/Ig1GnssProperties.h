//===========================================================================//
//
// Copyright (C) 2020 LP-Research Inc.
//
// This file is part of OpenZen, under the MIT License.
// See https://bitbucket.org/lpresearch/openzen/src/master/LICENSE for details
// SPDX-License-Identifier: MIT
//
//===========================================================================//

#ifndef ZEN_PROPERTIES_IG1GNSSPROPERTIES_H_
#define ZEN_PROPERTIES_IG1GNSSPROPERTIES_H_

#include <unordered_map>
#include <vector>

#include "communication/SyncedModbusCommunicator.h"
#include "components/ImuComponent.h"

namespace zen
{
    class Ig1GnssProperties : public ISensorProperties
    {
    public:
        Ig1GnssProperties(SyncedModbusCommunicator& communicator) noexcept;

        /** If successful executes the command, therwise returns an error. */
        ZenError execute(ZenProperty_t property) noexcept override;

        /** If successful fills the value with the property's boolean value, otherwise returns an error. */
        nonstd::expected<bool, ZenError> getBool(ZenProperty_t property) noexcept override;

        /** If successful sets the boolean property, otherwise returns an error. */
        ZenError setBool(ZenProperty_t property, bool value) noexcept override;

        /** Returns the type of the property */
        ZenPropertyType type(ZenProperty_t property) const noexcept override;

        /** Manually initializes the output-data bitset */
        void setGpsOutputDataBitset(uint64_t bitset) noexcept { m_cache.outputGpsDataBitset = bitset; }

    private:

        struct GnssState
        {
            std::atomic_uint64_t outputGpsDataBitset;
        } m_cache;

        SyncedModbusCommunicator& m_communicator;

        std::atomic_bool m_streaming;
    };
}

#endif
