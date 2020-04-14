//===========================================================================//
//
// Copyright (C) 2020 LP-Research Inc.
//
// This file is part of OpenZen, under the MIT License.
// See https://bitbucket.org/lpresearch/openzen/src/master/LICENSE for details
// SPDX-License-Identifier: MIT
//
//===========================================================================//

#ifndef FINDTHISMODULE_H_a48839fa_5146_4d3b_afda_d185e792951c
#define FINDTHISMODULE_H_a48839fa_5146_4d3b_afda_d185e792951c

#include <string>

namespace zen {
    namespace internal {
        // Get the filename of the current DLL / Executable / Module.
        std::wstring FindThisModule();

        //
        // Implementation details below.
        //

        // This needs an object that is guaranteed to be in the same file as the 
        // caller, so has to live in the same executable where it's called.  This is
        // guaranteed by using a variable in an anonymous namespace.
        namespace {
            char objectInThisModule;
        }

        // The actual worker.
        std::wstring FindModuleForObject(const void *object);

        // This calls the worker, passing it an address that is guaranteed
        // to be living in the same module as the caller.
        inline std::wstring FindThisModule()
        {
            return FindModuleForObject(&objectInThisModule);
        }
    }
}

#endif
