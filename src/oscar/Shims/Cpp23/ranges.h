#pragma once

#include <iterator>
#include <ranges>

namespace osc::cpp23
{
    // see: std::ranges::contains
    template<
        std::input_iterator I,
        std::sentinel_for<I> S,
        class T
    >
    constexpr bool contains(I first, S last, const T& value)
    {
        return std::ranges::find(first, last, value) != last;
    }

    // see: std::ranges::contains
    template<
        std::ranges::forward_range R,
        class T
    >
    requires std::indirect_binary_predicate<std::ranges::equal_to, std::ranges::iterator_t<R>, const T*>
    constexpr bool contains(R&& r, const T& value)
    {
        return contains(std::ranges::begin(r), std::ranges::end(r), value);
    }
}
