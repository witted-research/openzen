//===========================================================================//
//
// Copyright (C) 2020 LP-Research Inc.
//
// This file is part of OpenZen, under the MIT License.
// See https://bitbucket.org/lpresearch/openzen/src/master/LICENSE for details
// SPDX-License-Identifier: MIT
//
//===========================================================================//

#ifndef ZEN_UTILITY_REFERENCE_CMP_H_
#define ZEN_UTILITY_REFERENCE_CMP_H_

#include <functional>

template <typename T>
struct ReferenceWrapperCmp
{
    using type = std::reference_wrapper<T>;

    bool operator()(const type& lhs, const type& rhs) const noexcept
    {
        return std::less<T*>()(&lhs.get(), &rhs.get());
    }
};

#endif
