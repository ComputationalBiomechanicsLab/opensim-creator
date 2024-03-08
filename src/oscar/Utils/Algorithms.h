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
