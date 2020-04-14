//===========================================================================//
//
// Copyright (C) 2020 LP-Research Inc.
//
// This file is part of OpenZen, under the MIT License.
// See https://bitbucket.org/lpresearch/openzen/src/master/LICENSE for details
// SPDX-License-Identifier: MIT
//
//===========================================================================//

#include "ISensorProperties.h"

namespace zen
{
    void ISensorProperties::subscribeToPropertyChanges(ZenProperty_t property, SensorPropertyChangeCallback callback) noexcept
    {
        auto it = m_subscriberCallbacks.find(property);
        if (it == m_subscriberCallbacks.end())
            it = m_subscriberCallbacks.emplace(property, std::vector<SensorPropertyChangeCallback>()).first;

        it->second.emplace_back(std::move(callback));
    }

    void ISensorProperties::notifyPropertyChange(ZenProperty_t property, SensorPropertyValue value) const noexcept
    {
        auto it = m_subscriberCallbacks.find(property);
        if (it != m_subscriberCallbacks.end())
            for (const auto& callback : it->second)
                callback(value);
    }
}