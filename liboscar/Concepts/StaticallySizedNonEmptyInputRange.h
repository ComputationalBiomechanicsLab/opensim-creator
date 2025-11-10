#pragma once

#include <ranges>

namespace osc
{
    // Satisfied if `T` is a `std::ranges::input_range<T>` AND has a size that's
    // known at compile-time to be non-zero.
    template<typename T>
    concept StaticallySizedNonEmptyInputRange = std::ranges::input_range<T> and requires (const T& v) { std::ranges::size(v) > 0; };
}
