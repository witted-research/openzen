#include "utility/windows/WindowsDll.h"

#include <array>
#include <filesystem>
#include <new>
#include <string>
#include <type_traits>

#include <Windows.h>

namespace fs = std::filesystem;

namespace zen
{
    namespace
    {
        static unsigned int g_niftyCounter = 0;
        static typename std::aligned_storage_t<sizeof(WindowsDll), alignof(WindowsDll)> g_singletonBuffer;
        WindowsDll& g_singleton = reinterpret_cast<WindowsDll&>(g_singletonBuffer);
    }

    PlatformDllInitializer::PlatformDllInitializer()
    {
        if (g_niftyCounter++ == 0)
            new (&g_singleton) WindowsDll();
    }

    PlatformDllInitializer::~PlatformDllInitializer()
    {
        if (--g_niftyCounter == 0)
            (&g_singleton)->~WindowsDll();
    }

    IPlatformDll& IPlatformDll::get() noexcept
    {
        return g_singleton;
    }

    void* WindowsDll::load(std::string_view filename)
    {
        const auto filePath = fs::current_path().append(filename);

        // If already loaded
        if (auto handle = ::GetModuleHandleW(filePath.c_str()))
            return handle;

        if (auto handle = ::LoadLibraryW(filePath.c_str()))
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
