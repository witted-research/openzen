#include "utility/linux/LinuxDll.h"

#include <array>
#include <experimental/filesystem>
#include <new>
#include <string>
#include <type_traits>

#include <dlfcn.h>

namespace fs = std::experimental::filesystem;

namespace zen
{
    namespace
    {
        static unsigned int g_niftyCounter = 0;
        static std::aligned_storage_t<sizeof(LinuxDll), alignof(LinuxDll)> g_singletonBuffer;
        LinuxDll& g_singleton = reinterpret_cast<LinuxDll&>(g_singletonBuffer);
    }

    PlatformDllInitializer::PlatformDllInitializer()
    {
        if (g_niftyCounter++ == 0)
            new (&g_singleton) LinuxDll();
    }

    PlatformDllInitializer::~PlatformDllInitializer()
    {
        if (--g_niftyCounter == 0)
            (&g_singleton)->~LinuxDll();
    }

    IPlatformDll& IPlatformDll::get() noexcept
    {
        return g_singleton;
    }

    void* LinuxDll::load(std::string_view filename)
    {
        const auto filePath = fs::current_path().append(filename);
        return ::dlopen(filePath.c_str(), RTLD_LAZY);
    }

    void LinuxDll::unload(void* handle)
    {
        if (handle)
            ::dlclose(handle);
    }

    void* LinuxDll::procedure(void* handle, std::string_view procName)
    {
        if (handle == nullptr)
            return nullptr;

        return ::dlsym(handle, procName.data());
    }
}