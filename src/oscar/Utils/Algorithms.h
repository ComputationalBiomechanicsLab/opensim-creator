#pragma once

#include <oscar/Utils/Concepts.h>

#include <algorithm>
#include <cstddef>
#include <iterator>
#include <optional>
#include <ranges>
#include <stdexcept>
#include <utility>

namespace osc
{
    // see: std::ranges::all_of
    template<
        std::ranges::input_range R,
        std::indirect_unary_predicate<std::ranges::iterator_t<R>> Pred
    >
    constexpr bool all_of(R&& r, Pred pred)
    {
        return std::all_of(std::ranges::begin(r), std::ranges::end(r), pred);
    }

    // see: std::ranges::all_of
    template<
        std::input_iterator I,
        std::sentinel_for<I> S,
        std::indirect_unary_predicate<I> Pred
    >
    constexpr bool all_of(I first, S last, Pred pred)
    {
        return std::all_of(first, last, pred);
    }

    // see: std::ranges::any_of
    template<
        std::ranges::input_range R,
        std::indirect_unary_predicate<std::ranges::iterator_t<R>> Pred
    >
    constexpr bool any_of(R&& r, Pred pred)
    {
        return std::any_of(std::ranges::begin(r), std::ranges::end(r), pred);
    }

    // see: std::ranges::none_of
    template<
        std::ranges::input_range R,
        std::indirect_unary_predicate<std::ranges::iterator_t<R>> Pred
    >
    constexpr bool none_of(R&& r, Pred pred)
    {
        return std::any_of(std::ranges::begin(r), std::ranges::end(r), pred);
    }

    // see: std::ranges::mismatch
    template<
        std::ranges::input_range R1,
        std::ranges::input_range R2,
        typename Pred = std::ranges::equal_to
    >
    constexpr auto mismatch(R1&& r1, R2&& r2, Pred pred = {})
        requires std::indirectly_comparable<std::ranges::iterator_t<R1>, std::ranges::iterator_t<R2>, Pred>
    {
        return std::mismatch(
            std::ranges::begin(r1),
            std::ranges::end(r1),
            std::ranges::begin(r2),
            std::ranges::end(r2),
            pred
        );
    }

    // see: std::ranges::equal
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

    // see: std::ranges::lexicographical_compare
    template<
        std::ranges::input_range R1,
        std::ranges::input_range R2,
        std::indirect_strict_weak_order<
            std::ranges::iterator_t<R1>,
            std::ranges::iterator_t<R2>
        > Comp = std::ranges::less
    >
    constexpr bool lexicographical_compare(R1&& r1, R2&& r2, Comp comp = {})
    {
        return std::lexicographical_compare(
            std::ranges::begin(r1),
            std::ranges::end(r1),
            std::ranges::begin(r2),
            std::ranges::end(r2),
            comp
        );
    }

    // see: std::ranges::find_if
    template<
        std::input_iterator I,
        std::sentinel_for<I> S,
        std::indirect_unary_predicate<I> Pred
    >
    constexpr bool find_if(I first, S last, Pred pred)
    {
        return std::find_if(first, last, pred);
    }

    // see: std::ranges::find_if
    template<
        std::ranges::input_range R,
        std::indirect_unary_predicate<std::ranges::iterator_t<R>> Pred
    >
    constexpr std::ranges::borrowed_iterator_t<R> find_if(R&& r, Pred pred)
    {
        return std::find_if(std::ranges::begin(r), std::ranges::end(r), pred);
    }

    // see: std::ranges::find_if_not
    template<
        std::input_iterator I,
        std::sentinel_for<I> S,
        std::indirect_unary_predicate<I> Pred
    >
    constexpr bool find_if_not(I first, S last, Pred pred)
    {
        return std::find_if_not(first, last, pred);
    }

    // see: std::ranges::find_if_not
    template<
        std::ranges::input_range R,
        std::indirect_unary_predicate<std::ranges::iterator_t<R>> Pred
    >
    constexpr std::ranges::borrowed_iterator_t<R> find_if_not(R&& r, Pred pred)
    {
        return std::find_if_not(std::ranges::begin(r), std::ranges::end(r), pred);
    }

    // see: std::ranges::find
    template<
        std::input_iterator I,
        std::sentinel_for<I> S,
        typename T
    >
    constexpr I find(I first, S last, T const& value)
        requires std::indirect_binary_predicate<std::ranges::equal_to, I, T const*>
    {
        return std::find(first, last, value);
    }

    // see: std::ranges::find
    template<
        std::ranges::input_range R,
        class T
    >
    constexpr std::ranges::borrowed_iterator_t<R> find(R&& r, T const& value)
        requires std::indirect_binary_predicate<std::ranges::equal_to, std::ranges::iterator_t<R>, T const*>
    {
        return std::find(std::ranges::begin(r), std::ranges::end(r), value);
    }

    // see: std::ranges::copy
    //
    // NOTE: return value differs from C++20's std::ranges::copy (fix when MacOS supports std::ranges)
    template<
        std::input_iterator I,
        std::sentinel_for<I> S,
        std::weakly_incrementable O
    >
    constexpr void copy(I first, S last, O result)
        requires std::indirectly_copyable<I, O>
    {
        std::copy(first, last, result);
    }

    // see: std::ranges::copy
    //
    // NOTE: return value differs from C++20's std::ranges::copy (fix when MacOS supports std::ranges)
    template<
        std::ranges::input_range R,
        std::weakly_incrementable O
    >
    constexpr void copy(R&& r, O result)
        requires std::indirectly_copyable<std::ranges::iterator_t<R>, O>
    {
        std::copy(std::ranges::begin(r), std::ranges::end(r), result);
    }

    // see: std::ranges::count
    template<
        std::ranges::input_range R,
        typename T
    >
    constexpr typename std::ranges::range_difference_t<R> count(R&& r, T const& value)
        requires std::indirect_binary_predicate<std::ranges::equal_to, std::ranges::iterator_t<R>, T const*>
    {
        return std::count(std::ranges::begin(r), std::ranges::end(r), value);
    }

    // see: std::ranges::count_if
    template<
        std::ranges::input_range R,
        std::indirect_unary_predicate<std::ranges::iterator_t<R>> Pred
    >
    constexpr typename std::ranges::range_difference_t<R> count_if(R&& r, Pred pred)
    {
        return std::count_if(std::ranges::begin(r), std::ranges::end(r), pred);
    }

    // osc algorithm: perform bounds-checked indexed access
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

    // osc algorithm: returns a `std::optional<T>` containing the value located at `key`, or `std::nullopt` if no such element exists in `container`
    template<AssociativeContainer T, typename Key>
    std::optional<typename T::mapped_type> find_or_optional(T const& container, Key const& key)
    {
        if (auto it = container.find(key); it != container.end()) {
            return it->second;
        }
        return std::nullopt;
    }

    // osc algorithm: returns a pointer to the element at `key`, or `nullptr` if no such element exists in `container`
    template<AssociativeContainer T, typename Key>
    typename T::mapped_type const* try_find(T const& container, Key const& key)
    {
        if (auto it = container.find(key); it != container.end()) {
            return &it->second;
        }
        return nullptr;
    }

    // osc algorithm: returns a mutable pointer to the element at `key`, or `nullptr` if no such element exists in `container`
    template<AssociativeContainer T, typename Key>
    typename T::mapped_type* try_find(T& container, Key const& key)
    {
        if (auto it = container.find(key); it != container.end()) {
            return &it->second;
        }
        return nullptr;
    }
}
