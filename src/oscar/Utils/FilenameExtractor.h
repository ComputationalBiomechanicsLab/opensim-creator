#pragma once

#include <cstddef>
#include <string_view>

namespace osc
{
    template<size_t N>
    consteval std::string_view extract_filename(const char(&p)[N])
    {
        std::string_view sv{p};
        for (auto it = sv.rbegin(); it != sv.rend(); ++it) {
            if (*it == '/' or *it == '\\') {
                return std::string_view{it.base(), sv.end()};
            }
        }
        return sv;
    }
}
