//===========================================================================//
//
// Copyright (C) 2020 LP-Research Inc.
//
// This file is part of OpenZen, under the MIT License.
// See https://bitbucket.org/lpresearch/openzen/src/master/LICENSE for details
// SPDX-License-Identifier: MIT
//
//===========================================================================//

#include "io/systems/FtdiUsbSystem.h"

#include <array>
#include <cstring>
#include <iostream>

#include "io/interfaces/FtdiUsbInterface.h"

#include "utility/Finally.h"
#include "utility/IPlatformDll.h"

#include "ftd2xx.h"

namespace zen
{
    FtdiFnTable FtdiUsbSystem::fnTable = {};

    FtdiUsbSystem::~FtdiUsbSystem()
    {
        IPlatformDll::get().unload(m_handle);
    }

    bool FtdiUsbSystem::available()
    {
        if (m_handle != nullptr)
            return true;

        auto& dll = IPlatformDll::get();
        if (auto handle = dll.load("ftd2xx.dll"))
        {
            m_handle = handle;

            fnTable.close = reinterpret_cast<FtdiFnTable::CloseFn>(dll.procedure(m_handle, "FT_Close"));
            fnTable.getDeviceInfo = reinterpret_cast<FtdiFnTable::GetDeviceInfoFn>(dll.procedure(m_handle, "FT_GetDeviceInfo"));
            fnTable.getStatus = reinterpret_cast<FtdiFnTable::GetStatusFn>(dll.procedure(m_handle, "FT_GetStatus"));
            fnTable.listDevices = reinterpret_cast<FtdiFnTable::ListDevicesFn>(dll.procedure(m_handle, "FT_ListDevices"));
            fnTable.openEx = reinterpret_cast<FtdiFnTable::OpenExFn>(dll.procedure(m_handle, "FT_OpenEx"));
            fnTable.read = reinterpret_cast<FtdiFnTable::ReadFn>(dll.procedure(m_handle, "FT_Read"));
            fnTable.setBaudrate = reinterpret_cast<FtdiFnTable::SetBaudrateFn>(dll.procedure(m_handle, "FT_SetBaudRate"));
            fnTable.setDataCharacteristics = reinterpret_cast<FtdiFnTable::SetDataCharacteristicsFn>(dll.procedure(m_handle, "FT_SetDataCharacteristics"));
            fnTable.setFlowControl = reinterpret_cast<FtdiFnTable::SetFlowControlFn>(dll.procedure(m_handle, "FT_SetFlowControl"));
            fnTable.setLatencyTimer = reinterpret_cast<FtdiFnTable::SetLatencyTimerFn>(dll.procedure(m_handle, "FT_SetLatencyTimer"));
            fnTable.setUsbParameters = reinterpret_cast<FtdiFnTable::SetUsbParametersFn>(dll.procedure(m_handle, "FT_SetUSBParameters"));
            fnTable.write = reinterpret_cast<FtdiFnTable::WriteFn>(dll.procedure(m_handle, "FT_Write"));
            return true;
        }

        return false;
    }

    ZenError FtdiUsbSystem::listDevices(std::vector<ZenSensorDesc>& outDevices)
    {
        DWORD nDevices;
        if (!FT_SUCCESS(fnTable.listDevices(&nDevices, nullptr, FT_LIST_NUMBER_ONLY)))
            return ZenError_Unknown;

        for (DWORD i = 0; i < nDevices; ++i)
        {
            ZenSensorDesc desc;
            if (!FT_SUCCESS(fnTable.listDevices(&i, desc.name, FT_LIST_BY_INDEX | FT_OPEN_BY_DESCRIPTION)))
                return ZenError_Unknown;

            if (!FT_SUCCESS(fnTable.listDevices(&i, desc.serialNumber, FT_LIST_BY_INDEX | FT_OPEN_BY_SERIAL_NUMBER)))
                return ZenError_Unknown;

            std::memcpy(desc.ioType, FtdiUsbSystem::KEY, sizeof(FtdiUsbSystem::KEY));
            std::memcpy(desc.identifier, desc.serialNumber, sizeof(ZenSensorDesc::serialNumber));

            desc.baudRate = 921600;
            outDevices.emplace_back(desc);
        }

        return ZenError_None;
    }

    nonstd::expected<std::unique_ptr<IIoInterface>, ZenSensorInitError> FtdiUsbSystem::obtain(const ZenSensorDesc& desc, IIoDataSubscriber& subscriber) noexcept
    {
        FT_HANDLE handle;
        // Need to cast do a non-const because the FT_OpenEx interface expects a void pointer
        if (!FT_SUCCESS(fnTable.openEx(const_cast<char*>(desc.identifier), FT_OPEN_BY_SERIAL_NUMBER, &handle)))
            return nonstd::make_unexpected(ZenSensorInitError_InvalidAddress);

        auto ioInterface = std::make_unique<FtdiUsbInterface>(subscriber, handle);
        //if (outError = ioInterface->setBaudrate(DEFAULT_BAUDRATE))
        //    return nullptr;

        if (!FT_SUCCESS(fnTable.setDataCharacteristics(handle, FT_BITS_8, FT_STOP_BITS_1, FT_PARITY_NONE)))
            return nonstd::make_unexpected(ZenSensorInitError_IoFailed);

        if (!FT_SUCCESS(fnTable.setFlowControl(handle, FT_FLOW_RTS_CTS, 0x11, 0x12)))
            return nonstd::make_unexpected(ZenSensorInitError_IoFailed);

        if (!FT_SUCCESS(fnTable.setLatencyTimer(handle, 2)))
            return nonstd::make_unexpected(ZenSensorInitError_IoFailed);

        if (!FT_SUCCESS(fnTable.setUsbParameters(handle, 64, 0)))
            return nonstd::make_unexpected(ZenSensorInitError_IoFailed);

        return std::move(ioInterface);
    }
}
