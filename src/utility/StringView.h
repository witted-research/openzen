//===========================================================================//
//
// Copyright (C) 2020 LP-Research Inc.
//
// This file is part of OpenZen, under the MIT License.
// See https://bitbucket.org/lpresearch/openzen/src/master/LICENSE for details
// SPDX-License-Identifier: MIT
//
//===========================================================================//

#ifndef ZEN_UTILITY_STRING_H_
#define ZEN_UTILITY_STRING_H_

#include <algorithm>
#include <string_view>
#include <vector>
#include <sstream>
#include <gsl/span>

namespace util
{
    inline std::vector<std::string_view> split(std::string_view s, std::string_view delim = " ")
    {
        std::vector<std::string_view> output;

        for (auto first = s.data(), second = s.data(), last = first + s.size(); second != last && first != last; first = second + 1) {
            second = std::find_first_of(first, last, std::cbegin(delim), std::cend(delim));

            if (first != second)
                output.emplace_back(first, second - first);
        }

        return output;
    }

    /**
    Caller must ensure that buffer has the size of str.size() + 1
    */
    inline void stringToSpan(std::string const& str, gsl::span<std::byte> buffer) {
        for (size_t i = 0; i < str.size(); i++) {
            buffer[i] = std::byte(str[i]);
        }
        buffer[str.size()] = std::byte(0);
    }

    inline std::vector<std::byte> stringToBuffer(std::string const& str) {
      std::vector<std::byte> byteVector;
      byteVector.resize(str.size());
      for (size_t i = 0; i < str.size() ; i++) {
        byteVector[i] = std::byte( str[i] );
      }
      return byteVector;
    }

    inline std::string right_trim(std::string & strIn, char trimChar = '\0')
    {
        if (strIn.size() == 0)
            return strIn;
        auto endString = strIn.find_last_not_of(trimChar);
        return strIn.substr(0, endString + 1);
    }

    inline std::string spanToString(gsl::span<const std::byte> const& data) {
        std::stringstream rawOutput;
        for (auto c : data)
            rawOutput << std::to_integer<unsigned>(c) << ",";
        return rawOutput.str();
    }

    inline bool endsWith(std::string const &fullString, std::string const &ending)
    {
        if (fullString.length() >= ending.length())
        {
            return (0 == fullString.compare(fullString.length() - ending.length(), ending.length(), ending));
        }
        else
        {
            return false;
        }
    }

    inline bool startsWith(std::string const &fullString, std::string const &start)
    {
        if (fullString.length() >= start.length())
        {
            return (0 == fullString.compare(0, start.length(), start));
        }
        else
        {
            return false;
        }
    }
}

#endif
