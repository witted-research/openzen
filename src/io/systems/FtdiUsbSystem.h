//===========================================================================//
//
// Copyright (C) 2020 LP-Research Inc.
//
// This file is part of OpenZen, under the MIT License.
// See https://bitbucket.org/lpresearch/openzen/src/master/LICENSE for details
// SPDX-License-Identifier: MIT
//
//===========================================================================//

#ifndef ZEN_IO_SYSTEMS_FTDIUSBSYSTEM_H_
#define ZEN_IO_SYSTEMS_FTDIUSBSYSTEM_H_

#include "io/IIoSystem.h"

#include "ftd2xx.h"

namespace zen
{
    struct FtdiFnTable
    {
        using CloseFn = FT_STATUS (WINAPI *)(FT_HANDLE);
        using GetDeviceInfoFn = FT_STATUS (WINAPI *)(FT_HANDLE, FT_DEVICE*, LPDWORD, PCHAR, PCHAR, LPVOID);
        using GetStatusFn = FT_STATUS (WINAPI *)(FT_HANDLE, DWORD*, DWORD*, DWORD*);
        using ListDevicesFn = FT_STATUS (WINAPI *)(PVOID, PVOID, DWORD);
        using OpenExFn = FT_STATUS(WINAPI *)(PVOID, DWORD, FT_HANDLE*);
        using ReadFn = FT_STATUS (WINAPI*)(FT_HANDLE, LPVOID, DWORD, LPDWORD);
        using SetBaudrateFn = FT_STATUS (WINAPI *)(FT_HANDLE, ULONG);
        using SetDataCharacteristicsFn = FT_STATUS (WINAPI *)(FT_HANDLE, UCHAR, UCHAR, UCHAR);
        using SetFlowControlFn = FT_STATUS (WINAPI *)(FT_HANDLE, USHORT, UCHAR, UCHAR);
        using SetLatencyTimerFn = FT_STATUS (WINAPI *)(FT_HANDLE, UCHAR);
        using SetUsbParametersFn = FT_STATUS (WINAPI *)(FT_HANDLE, ULONG, ULONG);
        using WriteFn = FT_STATUS (WINAPI *)(FT_HANDLE, LPVOID, DWORD, LPDWORD);

        CloseFn close;
        GetDeviceInfoFn getDeviceInfo;
        GetStatusFn getStatus;
        ListDevicesFn listDevices;
        OpenExFn openEx;
        ReadFn read;
        SetBaudrateFn setBaudrate;
        SetDataCharacteristicsFn setDataCharacteristics;
        SetFlowControlFn setFlowControl;
        SetLatencyTimerFn setLatencyTimer;
        SetUsbParametersFn setUsbParameters;
        WriteFn write;
    };

    class FtdiUsbSystem : public IIoSystem
    {
        constexpr static unsigned int DEFAULT_BAUDRATE = FT_BAUD_921600;

    public:
        constexpr static const char KEY[] = "Ftdi";

        ~FtdiUsbSystem();

        bool available() override;

        ZenError listDevices(std::vector<ZenSensorDesc>& outDevices) override;

        nonstd::expected<std::unique_ptr<IIoInterface>, ZenSensorInitError> obtain(const ZenSensorDesc& desc, IIoDataSubscriber& subscriber) noexcept override;

        static FtdiFnTable fnTable;

    private:
        void* m_handle;
    };
}

#endif
