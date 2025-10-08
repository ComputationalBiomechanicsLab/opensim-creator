#pragma once

#include <cstddef>
#include <string_view>

namespace osc
{
    template<size_t N>
    consteval std::string_view extract_filename(const char(&p)[N])
    {
        for (auto i = static_cast<ptrdiff_t>(N) - 2; i > 0; --i) {
            if (p[i] == '/' or p[i] == '\\') {
                return std::string_view{p + i + 1, static_cast<size_t>((static_cast<ptrdiff_t>(N) - 2) - i)};
            }
        }
        return {p, N-1};  // else: no slashes, return as-is
    }
}
