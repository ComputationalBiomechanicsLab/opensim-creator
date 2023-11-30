#pragma once

#include <string_view>

namespace osc
{
    // shim, because MacOS's stdlib doesn't support <=> for std::string_view at
    // time of writing
    constexpr auto ThreeWayComparison(std::string_view lhs, std::string_view rhs)
    {
        auto const v = std::string_view{lhs}.compare(std::string_view{rhs});
        return v < 0 ? std::strong_ordering::less : v == 0 ? std::strong_ordering::equal : std::strong_ordering::greater;
    }
}
