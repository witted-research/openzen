//===========================================================================//
//
// Copyright (C) 2020 LP-Research Inc.
//
// This file is part of OpenZen, under the MIT License.
// See https://bitbucket.org/lpresearch/openzen/src/master/LICENSE for details
// SPDX-License-Identifier: MIT
//
//===========================================================================//

#ifndef ZEN_IO_CAN_CANMANAGER_H_
#define ZEN_IO_CAN_CANMANAGER_H_

#include <atomic>
#include <set>

#include "ZenTypes.h"
#include "io/can/ICanChannel.h"

namespace zen
{
    class CanManager
    {
    public:
        static CanManager& get();

        bool registerChannel(ICanChannel& channel);
        void unregisterChannel(ICanChannel& channel);

        bool available();

        ZenError poll();

        // [XXX] Should not be able to change after init (config?) Requires restart
        uint32_t id() const { return m_id; }
        void setId(uint32_t id) { m_id = id; }

    private:
        std::set<ICanChannel*> m_channels;
        std::atomic_uint32_t m_id;
    };

    static struct CanManagerInitializer
    {
        CanManagerInitializer();
        ~CanManagerInitializer();
    } canManagerInitializer;
}
#endif
