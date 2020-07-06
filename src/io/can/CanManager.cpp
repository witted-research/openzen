//===========================================================================//
//
// Copyright (C) 2020 LP-Research Inc.
//
// This file is part of OpenZen, under the MIT License.
// See https://bitbucket.org/lpresearch/openzen/src/master/LICENSE for details
// SPDX-License-Identifier: MIT
//
//===========================================================================//

#include "io/can/CanManager.h"

#include <new>
#include <type_traits>

namespace zen
{
    namespace CanManagerSingleton
    {
        static unsigned int g_niftyCounter = 0;
        static std::aligned_storage_t<sizeof(CanManager), alignof(CanManager)> g_singletonBuffer;
        CanManager& g_singleton = reinterpret_cast<CanManager&>(g_singletonBuffer);
    }

    CanManagerInitializer::CanManagerInitializer()
    {
        if (CanManagerSingleton::g_niftyCounter++ == 0)
            new (&CanManagerSingleton::g_singleton) CanManager();
    }

    CanManagerInitializer::~CanManagerInitializer()
    {
        if (--CanManagerSingleton::g_niftyCounter == 0)
            (&CanManagerSingleton::g_singleton)->~CanManager();
    }

    CanManager& CanManager::get()
    {
        return CanManagerSingleton::g_singleton;
    }

    bool CanManager::registerChannel(ICanChannel& channel)
    {
        auto result = m_channels.emplace(&channel);
        return result.second;
    }

    void CanManager::unregisterChannel(ICanChannel& channel)
    {
        m_channels.erase(&channel);
    }

    bool CanManager::available()
    {
        return !m_channels.empty();
    }

    ZenError CanManager::poll()
    {
        for (auto& channel : m_channels)
            if (auto error = channel->poll())
                return error;

        return ZenError_None;
    }
}
