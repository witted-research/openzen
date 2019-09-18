#ifndef ZEN_UTILITY_STRING_H_
#define ZEN_UTILITY_STRING_H_

#include <algorithm>
#include <string_view>
#include <vector>
#include <sstream>

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
}

#endif
