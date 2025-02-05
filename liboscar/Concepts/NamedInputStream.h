#pragma once

#include <concepts>
#include <iosfwd>
#include <string_view>

namespace osc
{
    template<typename T>
    concept NamedInputStream = requires(T v) {
        { v } -> std::convertible_to<std::istream&>;
        { v.name() } -> std::convertible_to<std::string_view>;
    };
}
