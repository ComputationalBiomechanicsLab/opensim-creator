#pragma once

#include <algorithm>
#include <iterator>
#include <ranges>
#include <utility>

namespace osc::cpp23
{
    // see: std::ranges::contains
    template<
        std::input_iterator I,
        std::sentinel_for<I> S,
        class T,
        class Proj = std::identity
    >
    requires std::indirect_binary_predicate<std::ranges::equal_to, std::projected<I, Proj>, const T*>
    constexpr bool contains(I first, S last, const T& value, Proj proj = {})
    {
        return std::ranges::find(first, last, value, proj) != last;
    }

    // see: std::ranges::contains
    template<
        std::ranges::forward_range R,
        class T,
        class Proj = std::identity
    >
    requires std::indirect_binary_predicate<std::ranges::equal_to, std::projected<std::ranges::iterator_t<R>, Proj>, const T*>
    constexpr bool contains(R&& r, const T& value, Proj proj = {})
    {
        return contains(std::ranges::begin(r), std::ranges::end(r), value, std::move(proj));
    }
}
