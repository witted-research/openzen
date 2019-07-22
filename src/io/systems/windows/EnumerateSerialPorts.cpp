#define NOMINMAX
#include "windows.h"

#include "EnumerateSerialPorts.h"

// This is derived from CEnumerateSerial found here: http://www.naughter.com/enumser.html
// referenced here https://stackoverflow.com/a/1394301/8680401.
//
// Copyright header on the original source:
/*
Copyright(c) 1998 - 2019 by PJ Naughter(Web: www.naughter.com, Email : pjna@naughter.com)

All rights reserved.

Copyright / Usage Details :

You are allowed to include the source code in any product (commercial, shareware, freeware or otherwise) 
when your product is released in binary form. You are allowed to modify the source code in any way you want 
except you cannot modify the copyright details at the top of each module. If you want to distribute source 
code with your application, then you are only allowed to distribute versions released by the author. This is 
to maintain a single distribution point for the source code. 
*/
// I don't think this qualifies as a version of the original code in the sense described above, as it
// cannot be used in the original's stead, this code isn't a drop-in replacement even for
// CEnumerateSerial::UsingRegistry().
// It has at least the following behavioral differences:
//  1) no inconsistent trailing NUL-characters in output
//  2) output is sorted
//  3) restricted to one-byte characters
//  4) doesn't call SetLastError()
// Also there are a number of implementation differences:
//  1) no _in_, _out_ etc annotations, no warning suppressions
//  2) no ATL used
//  3) automatic registry handle cleanup via gsl::finally
//  4) registry type check already during length check
//  5) main loop structure simplified
//  6) indentation, function names, "::" prefixes, etc. in line with OpenZen habits

#include <algorithm>
#include <string>
#include <vector>

#include "gsl/gsl"

namespace {
    // Gets the string valued registry key sans trailing NULs.
    bool GetRegistryString(HKEY key, LPCSTR lpValueName, std::string& sValue)
    {
        // Query for the size of the registry value
        ULONG nBytes = 0;
        DWORD dwType = 0;
        LSTATUS nStatus = ::RegQueryValueExA(key, lpValueName, nullptr, &dwType, nullptr, &nBytes);
        if (nStatus != ERROR_SUCCESS)
            return false;
        if ((dwType != REG_SZ) && (dwType != REG_EXPAND_SZ))
            return false;

        // Allocate enough bytes for the return value
        sValue.resize(static_cast<size_t>(nBytes));

        // Call again, now actually loading the string.
        nStatus = ::RegQueryValueExA(key, lpValueName, nullptr, &dwType,
            reinterpret_cast<LPBYTE>(sValue.data()), &nBytes);
        if (nStatus != ERROR_SUCCESS)
            return false;

        // COM1 at least has a NUL in the end, clean up.
        sValue.erase(std::find(sValue.begin(), sValue.end(), '\0'), sValue.end());
        return true;
    }
}

bool zen::EnumerateSerialPorts(std::vector<std::string>& ports)
{
    ports = {};

    HKEY hKey = 0;
    auto closeReg = gsl::finally([&hKey]() { if (hKey) RegCloseKey(hKey); });
    LSTATUS nStatus = ::RegOpenKeyExA(HKEY_LOCAL_MACHINE, R"(HARDWARE\DEVICEMAP\SERIALCOMM)", 0,
        KEY_QUERY_VALUE, &hKey);
    if (nStatus != ERROR_SUCCESS)
        return false;

    // Get the max value name and max value lengths
    DWORD dwMaxValueNameLen = 0;
    nStatus = ::RegQueryInfoKeyA(hKey, nullptr, nullptr, nullptr, nullptr, nullptr,
        nullptr, nullptr, &dwMaxValueNameLen, nullptr, nullptr, nullptr);
    if (nStatus != ERROR_SUCCESS)
        return false;

    const DWORD dwMaxValueNameSizeInChars = dwMaxValueNameLen + 1; //Include space for the null terminator

    //Allocate some space for the value name

    // Enumerate all the values underneath HKEY_LOCAL_MACHINE\HARDWARE\DEVICEMAP\SERIALCOMM
    DWORD dwIndex = 0;
    while (true) {
        std::vector<char> valueName(dwMaxValueNameSizeInChars, 0);
        DWORD dwValueNameSize = dwMaxValueNameSizeInChars;
        if (::RegEnumValueA(hKey, dwIndex++, valueName.data(), &dwValueNameSize,
                nullptr, nullptr, nullptr, nullptr) != ERROR_SUCCESS)
            break;

        std::string sPortName;
        if (::GetRegistryString(hKey, valueName.data(), sPortName))
            ports.emplace_back(std::move(sPortName));
    }

    // Sort the output.
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
