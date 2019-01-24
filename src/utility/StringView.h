#ifndef ZEN_UTILITY_STRING_H_
#define ZEN_UTILITY_STRING_H_

#include <algorithm>
#include <string_view>
#include <vector>

namespace util
{
    std::vector<std::string_view> split(std::string_view s, std::string_view delim = " ")
    {
        std::vector<std::string_view> output;

        for (auto first = s.data(), second = s.data(), last = first + s.size(); second != last && first != last; first = second + 1) {
            second = std::find_first_of(first, last, std::cbegin(delim), std::cend(delim));

            if (first != second)
                output.emplace_back(first, second - first);
        }

        return output;
    }
}

#endif
