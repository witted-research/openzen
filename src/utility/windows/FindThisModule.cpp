//===========================================================================//
//
// Copyright (C) 2020 LP-Research Inc.
//
// This file is part of OpenZen, under the MIT License.
// See https://bitbucket.org/lpresearch/openzen/src/master/LICENSE for details
// SPDX-License-Identifier: MIT
//
//===========================================================================//

#include "FindThisModule.h"

#include <Windows.h>

#include <vector>

std::wstring zen::internal::FindModuleForObject(const void* object)
{
    HMODULE module;
    GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS
        | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
        (LPCSTR)object, &module);
    // Get the filename of the file containing that module, making sure it fits in buffer.
    // According to the documentation, we can only tell if the buffer is long enough when
    // GetModuleFileName returns something _smaller_ than the size of the passed buffer,
    // so we're not guaranteed to be able to grow it to the correct size in one step.
    std::vector<wchar_t> fnbuf(128);
    DWORD resultLen = 0;
    do {
        fnbuf.resize(2 * fnbuf.size());
        resultLen = GetModuleFileNameW(module, fnbuf.data(), (DWORD)fnbuf.size());
    } while (resultLen == fnbuf.size());
    // Form string from buffer minus the \0 at the end.
    // (End iterator is one past the end, i.e. on the \0.)
    return std::wstring(fnbuf.begin(), fnbuf.begin() + resultLen);
}
