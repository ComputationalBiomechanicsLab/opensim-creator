#pragma once

#include <concepts>
#include <iosfwd>
#include <string_view>

namespace osc
{
    // Satisfied if `T` is convertible to `std::istream&` and `T`
    // has a `name()` member method that returns something that is
    // convertible to `std::string_view` (i.e. it's a stream with
    // a name).
    template<typename T>
    concept NamedInputStream = requires(T v) {
        { v } -> std::convertible_to<std::istream&>;
        { v.name() } -> std::convertible_to<std::string_view>;
    };
}
