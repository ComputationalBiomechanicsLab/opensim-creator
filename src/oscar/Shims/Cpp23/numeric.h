#pragma once

#include <oscar/Shims/Cpp23/algorithm.h>

#include <iterator>
#include <ranges>
#include <utility>

namespace osc::cpp23
{
    template<typename O, typename T>
    using iota_result = cpp23::out_value_result<O, T>;

    template<
        std::input_or_output_iterator O,
        std::sentinel_for<O> S,
        std::weakly_incrementable T
    >
    requires std::indirectly_writable<O, const T&>
    constexpr iota_result<O, T> iota(O first, S last, T value)
    {
        while (first != last) {
            *first = std::as_const(value);
            ++first;
            ++value;
        }
        return {std::move(first), std::move(value)};
    }

    template<std::weakly_incrementable T, std::ranges::output_range<const T&> R>
    constexpr iota_result<std::ranges::borrowed_iterator_t<R>, T> iota(R&& r, T value)
    {
        return cpp23::iota(std::ranges::begin(r), std::ranges::end(r), std::move(value));
    }
}
