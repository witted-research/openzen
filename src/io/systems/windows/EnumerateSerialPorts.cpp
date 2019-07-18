#include "EnumerateSerialPorts.h"

// This is derived from CEnumerateSerial found here: http://www.naughter.com/enumser.html
// referenced here https://stackoverflow.com/a/1394301/8680401.
//
// Copyright header on the original source:
/*
Copyright(c) 1998 - 2019 by PJ Naughter(Web: www.naughter.com, Email : pjna@naughter.com)

All rights reserved.

Copyright / Usage Details :

You are allowed to include the source code in any product(commercial, shareware, freeware or otherwise)
when your product is released in binary form.You are allowed to modify the source code in any way you want
except you cannot modify the copyright details at the top of each module.If you want to distribute source
code with your application, then you are only allowed to distribute versions released by the author.This is
to maintain a single distribution point for the source code.
*/
// I don't think this qualifies as a version of the original code as it no longer serves the same purpose,
// so the described risk of confusion is not present.

#include <algorithm>
#include <string>
#include <vector>

// Pull in ATL support, only sane way to use the registry.
#include <atlbase.h>

namespace {
    // Gets the string valued registry key sans trailing NULs.
    bool RegQueryValueString(ATL::CRegKey& key, LPCSTR lpValueName, std::string& sValue)
    {
        //First query for the size of the registry value
        ULONG nChars = 0;
        LSTATUS nStatus = key.QueryStringValue(lpValueName, nullptr, &nChars);
        if (nStatus != ERROR_SUCCESS)
        {
            SetLastError(nStatus);
            return false;
        }

        //Allocate enough bytes for the return value
        sValue.resize(static_cast<size_t>(nChars));

        // We will use RegQueryValueEx directly here because ATL::CRegKey::QueryStringValue does not
        // handle non-null terminated data
        DWORD dwType = 0;
        ULONG nBytes = static_cast<ULONG>(sValue.size());
        nStatus = RegQueryValueExA(key, lpValueName, nullptr, &dwType,
            reinterpret_cast<LPBYTE>(sValue.data()), &nBytes);
        if (nStatus != ERROR_SUCCESS)
        {
            SetLastError(nStatus);
            return false;
        }
        if ((dwType != REG_SZ) && (dwType != REG_EXPAND_SZ))
        {
            SetLastError(ERROR_INVALID_DATA);
            return false;
        }

        // COM1 at least has a NUL in the end, clean up.
        sValue.erase(std::find(sValue.begin(), sValue.end(), '\0'), sValue.end());
        return true;
    }
}

bool EnumerateSerialPorts(std::vector<std::string>& ports)
{
    ports = {};

    ATL::CRegKey serialCommKey;
    LSTATUS nStatus = serialCommKey.Open(HKEY_LOCAL_MACHINE, R"(HARDWARE\DEVICEMAP\SERIALCOMM)", KEY_QUERY_VALUE);
    if (nStatus != ERROR_SUCCESS)
    {
        SetLastError(nStatus);
        return false;
    }

    //Get the max value name and max value lengths
    DWORD dwMaxValueNameLen = 0;
    nStatus = RegQueryInfoKeyA(serialCommKey, nullptr, nullptr, nullptr, nullptr,
        nullptr, nullptr, nullptr, &dwMaxValueNameLen, nullptr, nullptr, nullptr);
    if (nStatus != ERROR_SUCCESS)
    {
        SetLastError(nStatus);
        return false;
    }

    const DWORD dwMaxValueNameSizeInChars = dwMaxValueNameLen + 1; //Include space for the null terminator

    //Allocate some space for the value name
    std::vector<char> valueName;
    valueName.resize(dwMaxValueNameSizeInChars);

    //Enumerate all the values underneath HKEY_LOCAL_MACHINE\HARDWARE\DEVICEMAP\SERIALCOMM
    bool bContinueEnumeration = true;
    DWORD dwIndex = 0;
    while (bContinueEnumeration)
    {
        DWORD dwValueNameSize = dwMaxValueNameSizeInChars;
        valueName[0] = '\0';
        bContinueEnumeration = (RegEnumValue(serialCommKey, dwIndex, valueName.data(),
            &dwValueNameSize, nullptr, nullptr, nullptr, nullptr) == ERROR_SUCCESS);
        if (bContinueEnumeration)
        {
            std::string sPortName;
            if (RegQueryValueString(serialCommKey, valueName.data(), sPortName))
                ports.emplace_back(std::move(sPortName));

            //Prepare for the next loop
            ++dwIndex;
        }
    }

    std::sort(ports.begin(), ports.end(),
        [](std::string_view left, std::string_view right) -> bool {
            // Strings are either COMx, COMxx or COMxxx with no leading zeros.
            // COMx comes before COMxx, COMxxx etc.
            if (left.size() != right.size())
                return left.size() < right.size();
            // Compare the digit strings which are guaranteed to be of the same length.
            return left.substr(3) < right.substr(3);
        });
    return true;
}
