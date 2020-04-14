//===========================================================================//
//
// Copyright (C) 2020 LP-Research Inc.
//
// This file is part of OpenZen, under the MIT License.
// See https://bitbucket.org/lpresearch/openzen/src/master/LICENSE for details
// SPDX-License-Identifier: MIT
//
//===========================================================================//

#ifndef ZEN_UTILITY_WINDOWS_WINDOWSDLL_H_
#define ZEN_UTILITY_WINDOWS_WINDOWSDLL_H_

#include <string>

#include "utility/IPlatformDll.h"

namespace zen
{
    class WindowsDll final : public IPlatformDll
    {
    public:
        /** Load a DLL */
        void* load(std::string_view filename) override;

        /** Free a DLL */
        void unload(void* handle) override;

        /** Lookup the address of a DLL function */
        void* procedure(void* handle, std::string_view procName) override;

    private:
        std::string m_dllDirectory;
    };
}

#endif
