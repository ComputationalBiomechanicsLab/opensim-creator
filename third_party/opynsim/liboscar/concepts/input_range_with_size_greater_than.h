#pragma once

#include <liboscar/concepts/range_with_size_greater_than.h>

#include <ranges>

namespace osc
{
    // Satisfied if `T` is a `std::ranges::input_range<T>` AND has a size that's
    // known at compile-time to be non-zero.
    template<typename T, size_t N>
    concept InputRangeWithSizeGreaterThan = std::ranges::input_range<T> and RangeWithSizeGreaterThan<T, N>;
}
