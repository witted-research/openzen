//===========================================================================//
//
// Copyright (C) 2020 LP-Research Inc.
//
// This file is part of OpenZen, under the MIT License.
// See https://bitbucket.org/lpresearch/openzen/src/master/LICENSE for details
// SPDX-License-Identifier: MIT
//
//===========================================================================//

#ifndef ZEN_UTILITY_FINALLY_H_
#define ZEN_UTILITY_FINALLY_H_

#include <utility>

template <typename Func>
class FinallyGuard
{
public:
    FinallyGuard(Func f)
        : m_func(std::move(f))
        , m_active(true)
    {}

    FinallyGuard(FinallyGuard&& other)
        : m_func(std::move(other.m_func))
        , m_active(other.m_active)
    {
        other.reset();
    }

    FinallyGuard& operator=(FinallyGuard&&) = delete;

    ~FinallyGuard()
    {
        if (m_active)
            m_func();
    }

    void reset()
    {
        m_active = false;
    }

private:
    Func m_func;
    bool m_active;
};

template <typename Func>
FinallyGuard<typename std::decay<Func>::type> finally(Func&& f)
{
    return { std::forward<Func>(f) };
}

#endif
