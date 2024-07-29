#pragma once

#include <cstddef>
#include <string_view>

namespace osc
{
    template<size_t N>
    consteval std::string_view extract_filename(const char(&p)[N])
    {
        // note: C++20's `std::source_location` makes this _a lot_ less useful

        std::string_view sv{p};
        for (auto it = sv.rbegin(); it != sv.rend(); ++it) {
            if (*it == '/' or *it == '\\') {
                return std::string_view{it.base(), sv.end()};
            }
        }
        return sv;
    }
}
