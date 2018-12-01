#ifndef ZEN_UTILITY_IPLATFORMDLL_H_
#define ZEN_UTILITY_IPLATFORMDLL_H_

#include <cstdint>
#include <string_view>

namespace zen
{
    class IPlatformDll
    {
    public:
        static IPlatformDll& get() noexcept;

        /** Load a DLL */
        virtual void* load(std::string_view filename) = 0;

        /** Free a DLL */
        virtual void unload(void* handle) = 0;

        /** Lookup the address of a DLL function */
        virtual void* procedure(void* handle, std::string_view procName) = 0;
    };

    static struct PlatformDllInitializer
    {
        PlatformDllInitializer();
        ~PlatformDllInitializer();
    } platformDllInitializer;
}
#endif
