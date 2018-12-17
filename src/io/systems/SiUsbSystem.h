#ifndef ZEN_IO_SYSTEMS_SIUSBSYSTEM_H_
#define ZEN_IO_SYSTEMS_SIUSBSYSTEM_H_

#define NOMINMAX
#include <Windows.h>
#undef NOMINMAX

#include "SiUSBXp.h"
#include "io/IIoSystem.h"

namespace zen
{
    struct SiUsbFnTable
    {
        using CheckRxQueueFn = SI_STATUS (WINAPI *)(const HANDLE, LPDWORD, LPDWORD);
        using CloseFn = SI_STATUS (WINAPI *)(const HANDLE);
        using GetDeviceProductStringFn = SI_STATUS (WINAPI *)(const HANDLE, LPVOID, LPBYTE);
        using GetNumDevicesFn = SI_STATUS (WINAPI *)(LPDWORD);
        using GetProductStringSafeFn = SI_STATUS (WINAPI *)(const DWORD, LPVOID, const size_t, const DWORD);
        using OpenFn = SI_STATUS (WINAPI *)(const DWORD, HANDLE*);
        using ReadFn = SI_STATUS (WINAPI *)(const HANDLE, LPVOID, const DWORD, LPDWORD, LPOVERLAPPED);
        using SetBaudrateFn = SI_STATUS (WINAPI *)(const HANDLE, const DWORD);
        using SetFlowControlFn = SI_STATUS (WINAPI *)(const HANDLE, const BYTE, const BYTE, const BYTE, const BYTE, const BYTE, const BOOL);
        using WriteFn = SI_STATUS (WINAPI *)(const HANDLE, LPVOID, const DWORD, LPDWORD, LPOVERLAPPED);

        CheckRxQueueFn checkRxQueue;
        CloseFn close;
        GetDeviceProductStringFn getDeviceProductString;
        GetNumDevicesFn getNumDevices;
        GetProductStringSafeFn getProductStringSafe;
        OpenFn open;
        ReadFn read;
        SetBaudrateFn setBaudrate;
        SetFlowControlFn setFlowControl;
        WriteFn write;
    };

    class SiUsbSystem : public IIoSystem
    {
    public:
        constexpr static const char KEY[] = "SiUsb";

        ~SiUsbSystem();

        bool available() override;

        ZenError listDevices(std::vector<ZenSensorDesc>& outDevices) override;

        std::unique_ptr<BaseIoInterface> obtain(const ZenSensorDesc& desc, ZenError& outError) override;

        static SiUsbFnTable fnTable;

    private:
        void* m_handle;
    };
}

#endif