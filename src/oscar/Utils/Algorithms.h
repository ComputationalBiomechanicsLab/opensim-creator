#pragma once

#include <oscar/Utils/Concepts.h>

#include <algorithm>
#include <cstddef>
#include <optional>
#include <ranges>
#include <stdexcept>

namespace osc
{
    template<
        std::ranges::input_range R,
        std::indirect_unary_predicate<std::ranges::iterator_t<R>> Pred
    >
    constexpr bool all_of(R&& r, Pred pred)
    {
        return std::all_of(std::ranges::begin(r), std::ranges::end(r), pred);
    }

    template<
        std::ranges::input_range R,
        std::indirect_unary_predicate<std::ranges::iterator_t<R>> Pred
    >
    constexpr bool any_of(R&& r, Pred pred)
    {
        return std::any_of(std::ranges::begin(r), std::ranges::end(r), pred);
    }

    template<
        std::ranges::input_range R,
        std::indirect_unary_predicate<std::ranges::iterator_t<R>> Pred
    >
    constexpr bool none_of(R&& r, Pred pred)
    {
        return std::any_of(std::ranges::begin(r), std::ranges::end(r), pred);
    }

    template<
        std::ranges::input_range R1,
        std::ranges::input_range R2,
        class Pred = std::ranges::equal_to
    >
    constexpr bool equal(R1&& r1, R2&& r2, Pred pred = {})
        requires std::indirectly_comparable<std::ranges::iterator_t<R1>, std::ranges::iterator_t<R2>, Pred>
    {
        return std::equal(std::ranges::begin(r1), std::ranges::end(r1), std::ranges::begin(r2), std::ranges::end(r2), pred);
    }

    template<
        std::ranges::input_range R,
        std::indirect_unary_predicate<std::ranges::iterator_t<R>> Pred
    >
    constexpr std::ranges::borrowed_iterator_t<R> find_if(R&& r, Pred pred)
    {
        return std::find_if(std::ranges::begin(r), std::ranges::end(r), pred);
    }

    template<
        std::ranges::input_range R,
        class T
    >
    constexpr std::ranges::borrowed_iterator_t<R> find(R&& r, T const& value)
        requires std::indirect_binary_predicate<std::ranges::equal_to, std::ranges::iterator_t<R>, T const*>
    {
        return std::find(std::ranges::begin(r), std::ranges::end(r), value);
    }

    template<
        std::ranges::input_range R,
        std::weakly_incrementable O
    >
    constexpr void copy(R&& r, O result)  // NOTE: return value differs from C++20's std::ranges::copy
        requires std::indirectly_copyable<std::ranges::iterator_t<R>, O>
    {
        std::copy(std::ranges::begin(r), std::ranges::end(r), result);
    }

    template<
        std::ranges::input_range R,
        typename T
    >
    constexpr typename std::ranges::range_difference_t<R> count(R&& r, T const& value)
        requires std::indirect_binary_predicate<std::ranges::equal_to, std::ranges::iterator_t<R>, T const*>
    {
        return std::count(std::ranges::begin(r), std::ranges::end(r), value);
    }

    template<
        std::ranges::input_range R,
        std::indirect_unary_predicate<std::ranges::iterator_t<R>> Pred
    >
    constexpr typename std::ranges::range_difference_t<R> count_if(R&& r, Pred pred)
    {
        return std::count_if(std::ranges::begin(r), std::ranges::end(r), pred);
    }

    template<std::ranges::random_access_range Range>
    constexpr auto at(Range const& range, typename Range::size_type i) -> decltype(range[i])
    {
        if (i < std::ranges::size(range))
        {
            return range[i];
        }
        else
        {
            throw std::out_of_range{"out of bounds index given to a container"};
        }
    }

    template<AssociativeContainer T, typename Key>
    std::optional<typename T::mapped_type> find_or_optional(T const& container, Key const& key)
    {
        if (auto it = container.find(key); it != container.end()) {
            return it->second;
        }
        return std::nullopt;
    }

    template<AssociativeContainer T, typename Key>
    typename T::mapped_type const* try_find(T const& container, Key const& key)
    {
        if (auto it = container.find(key); it != container.end()) {
            return &it->second;
        }
        return nullptr;
    }

    template<AssociativeContainer T, typename Key>
    typename T::mapped_type* try_find(T& container, Key const& key)
    {
        if (auto it = container.find(key); it != container.end()) {
            return &it->second;
        }
        return nullptr;
    }
}
