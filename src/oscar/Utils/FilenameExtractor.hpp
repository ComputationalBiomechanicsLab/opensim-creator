#pragma once

#include <cstddef>
#include <string_view>

namespace osc
{
    template<size_t N>
    consteval std::string_view ExtractFilename(char const(&p)[N])
    {
        std::string_view sv{p};
        for (auto it = sv.rbegin(); it != sv.rend(); ++it)
        {
            if (*it == '/' || *it == '\\')
            {
                return std::string_view{it.base(), sv.end()};
            }
        }
        return sv;
    }
}
