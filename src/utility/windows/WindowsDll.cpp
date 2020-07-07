//===========================================================================//
//
// Copyright (C) 2020 LP-Research Inc.
//
// This file is part of OpenZen, under the MIT License.
// See https://bitbucket.org/lpresearch/openzen/src/master/LICENSE for details
// SPDX-License-Identifier: MIT
//
//===========================================================================//

#include "utility/windows/WindowsDll.h"

#include <array>
#include <filesystem>
#include <new>
#include <string>
#include <type_traits>

#include <gsl/gsl_util>
#include <Windows.h>

#include "FindThisModule.h"

namespace fs = std::filesystem;

namespace zen
{
    namespace WindowsDllSingleton
    {
        static unsigned int g_niftyCounter = 0;
        static typename std::aligned_storage_t<sizeof(WindowsDll), alignof(WindowsDll)> g_singletonBuffer;
        WindowsDll& g_singleton = reinterpret_cast<WindowsDll&>(g_singletonBuffer);
    }

    PlatformDllInitializer::PlatformDllInitializer()
    {
        if (WindowsDllSingleton::g_niftyCounter++ == 0)
            new (&WindowsDllSingleton::g_singleton) WindowsDll();
    }

    PlatformDllInitializer::~PlatformDllInitializer()
    {
        if (--WindowsDllSingleton::g_niftyCounter == 0)
            (&WindowsDllSingleton::g_singleton)->~WindowsDll();
    }

    IPlatformDll& IPlatformDll::get() noexcept
    {
        return WindowsDllSingleton::g_singleton;
    }

    void* WindowsDll::load(std::string_view filename)
    {
        // Load the DLL which should be in the same directory as openzen.dll.
        // The safest way to actually loading the DLL and all its dependencies
        // appears to be using SetDllDirectory which gives us a deterministic
        // search order.  AddDllDirectory -- if I take the documentation at
        // face value, even though it is wrong in other cases that I inspected
        // -- cannot be used because then dependent Dlls might not find their
        // dependencies.
        //
        // So we remember the current DllDirectory, change it, try to load the
        // DLL and set the DllDirectory back to its original value before we
        // return.
        // FIXME this is not threadsafe.

        // Path to the openzen.dll.
        fs::path path(zen::internal::FindThisModule());
        path.remove_filename();

        wchar_t c; // Null pointer argument to GetDllDirectory isn't
                   // documented, so let's pass this as destination for
                   // the length check.
        DWORD bufLen = ::GetDllDirectoryW(1, &c);
        std::vector<wchar_t> dllDirectoryBackup(bufLen);
        ::GetDllDirectoryW(bufLen, dllDirectoryBackup.data());
        ::SetDllDirectoryW(path.c_str());
        auto resetDllDirectory = gsl::finally([&path, &dllDirectoryBackup]() {
            SetDllDirectoryW(dllDirectoryBackup.data()); });

        // If already loaded
        if (auto handle = ::GetModuleHandleA(std::string(filename).c_str()))
            return handle;

        if (auto handle = ::LoadLibraryA(std::string(filename).c_str()))
            return handle;

        return nullptr;
    }

    void WindowsDll::unload(void* handle)
    {
        // Handles nullptr
        ::FreeLibrary(reinterpret_cast<HMODULE>(handle));
    }

    void* WindowsDll::procedure(void* handle, std::string_view procName)
    {
        if (handle == nullptr)
            return nullptr;

        return ::GetProcAddress(reinterpret_cast<HMODULE>(handle), procName.data());
    }
}
