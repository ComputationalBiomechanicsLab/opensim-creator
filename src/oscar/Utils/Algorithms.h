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

    // see: std::ranges::contains
    template<
        std::input_iterator I,
        std::sentinel_for<I> S,
        class T
    >
    constexpr bool contains(I first, S last, T const& value)
    {
        return find(first, last, value) != last;
    }

    // see: std::ranges::contains
    template<
        std::ranges::forward_range R,
        class T
    >
    constexpr bool contains(R&& r, T const& value)
        requires std::indirect_binary_predicate<std::ranges::equal_to, std::ranges::iterator_t<R>, T const*>
    {
        return contains(std::ranges::begin(r), std::ranges::end(r), value);
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

    // see: std::ranges::fill
    //
    // NOTE: return value differs from C++20's std::ranges:fill (fix when MacOS supports std::ranges)
    template<
        typename T,
        std::ranges::output_range<T const&> R
    >
    constexpr void fill(R&& r, T const& value)
    {
        std::fill(std::ranges::begin(r), std::ranges::end(r), value);
    }

    // see: std::ranges::reverse
    template<std::ranges::bidirectional_range R>
    constexpr std::ranges::borrowed_iterator_t<R> reverse(R&& r)
        requires std::permutable<std::ranges::iterator_t<R>>
    {
        auto last = std::ranges::end(r);
        std::reverse(std::ranges::begin(r), last);
        return last;
    }

    // see: std::ranges::sample
    template<
        std::ranges::input_range R,
        std::weakly_incrementable O,
        typename Gen
    >
    O sample(R&& r, O out, std::ranges::range_difference_t<R> n, Gen&& gen)
        requires
            (std::ranges::forward_range<R> || std::random_access_iterator<O>) &&
             std::indirectly_copyable<std::ranges::iterator_t<R>, O> &&
             std::uniform_random_bit_generator<std::remove_reference_t<Gen>>
    {
        return std::sample(std::ranges::begin(r), std::ranges::end(r), std::move(out), n, std::forward<Gen>(gen));
    }

    // see: std::ranges::max
    template<
        class T,
        class Proj = std::identity,
        std::indirect_strict_weak_order<std::projected<T const*, Proj>> Comp = std::ranges::less
    >
    constexpr T const& max(T const& a, T const& b, Comp comp = {}, Proj proj = {})
    {
        return std::invoke(comp, std::invoke(proj, a), std::invoke(proj, b)) ? b : a;
    }

    // see: std::ranges::max
    template<
        std::copyable T,
        class Proj = std::identity,
        std::indirect_strict_weak_order<std::projected<T const*, Proj>> Comp = std::ranges::less
    >
    constexpr T max(std::initializer_list<T> r, Comp comp = {}, Proj proj = {})
    {
        return *max_element(r, std::ref(comp), std::ref(proj));
    }

    // see: std::ranges::max
    template<
        std::ranges::input_range R,
        class Proj = std::identity,
        std::indirect_strict_weak_order<std::projected<std::ranges::iterator_t<R>, Proj>> Comp = std::ranges::less
    >
    constexpr std::ranges::range_value_t<R> max(R&& r, Comp comp = {}, Proj proj = {})
        requires std::indirectly_copyable_storable<std::ranges::iterator_t<R>, std::ranges::range_value_t<R>*>
    {
        using V = std::ranges::range_value_t<R>;
        if constexpr (std::ranges::forward_range<R>) {
            return static_cast<V>(*max_element(r, std::ref(comp), std::ref(proj)));
        }
        else {
            auto i = std::ranges::begin(r);
            auto s = std::ranges::end(r);
            V biggest(*i);
            while (++i != s) {
                if (std::invoke(comp, std::invoke(proj, biggest), std::invoke(proj, *i))) {
                    biggest = *i;
                }
            }
            return biggest;
        }
    }

    // see: std::ranges::max_element
    template<
        std::forward_iterator I,
        std::sentinel_for<I> S,
        class Proj = std::identity,
        std::indirect_strict_weak_order<std::projected<I, Proj>> Comp = std::ranges::less
    >
    constexpr I max_element(I first, S last, Comp comp = {}, Proj proj = {})
    {
        if (first == last) {
            return last;
        }

        auto largest = first;
        while (++first != last) {
            if (std::invoke(comp, std::invoke(proj, *largest), std::invoke(proj, *first))) {
                largest = first;
            }
        }
        return largest;
    }

    // see: std::ranges::max_element
    template<
        std::ranges::forward_range R,
        class Proj = std::identity,
        std::indirect_strict_weak_order<std::projected<std::ranges::iterator_t<R>, Proj>> Comp = std::ranges::less
    >
    constexpr std::ranges::borrowed_iterator_t<R> max_element(R&& r, Comp comp = {}, Proj proj = {})
    {
        return max_element(std::ranges::begin(r), std::ranges::end(r), std::ref(comp), std::ref(proj));
    }

    // see: std::ranges::min
    template<
        class T,
        class Proj = std::identity,
        std::indirect_strict_weak_order<std::projected<T const*, Proj>> Comp = std::ranges::less
    >
    constexpr const T& min(const T& a, const T& b, Comp comp = {}, Proj proj = {})
    {
        return std::invoke(comp, std::invoke(proj, b), std::invoke(proj, a)) ? b : a;
    }

    // see: std::ranges::min
    template<
        std::copyable T,
        class Proj = std::identity,
        std::indirect_strict_weak_order<std::projected<T const*, Proj>> Comp = std::ranges::less
    >
    constexpr T min(std::initializer_list<T> r, Comp comp = {}, Proj proj = {})
    {
        return *min_element(r, std::ref(comp), std::ref(proj));
    }

    // see: std::ranges::min
    template<
        std::ranges::input_range R,
        class Proj = std::identity,
        std::indirect_strict_weak_order<std::projected<std::ranges::iterator_t<R>, Proj>> Comp = std::ranges::less
    >
    constexpr std::ranges::range_value_t<R> min(R&& r, Comp comp = {}, Proj proj = {})
        requires std::indirectly_copyable_storable<std::ranges::iterator_t<R>, std::ranges::range_value_t<R>*>
    {
        using V = std::ranges::range_value_t<R>;
        if constexpr (std::ranges::forward_range<R>) {
            return static_cast<V>(*min_element(r, std::ref(comp), std::ref(proj)));
        }
        else {
            auto i = std::ranges::begin(r);
            auto s = std::ranges::end(r);
            V m(*i);
            while (++i != s) {
                if (std::invoke(comp, std::invoke(proj, *i), std::invoke(proj, m))) {
                    m = *i;
                }
            }
            return m;
        }
    }

    // see: std::ranges::min_element
    template<
        std::forward_iterator I,
        std::sentinel_for<I> S,
        class Proj = std::identity,
        std::indirect_strict_weak_order<std::projected<I, Proj>> Comp = std::ranges::less
    >
    constexpr I min_element(I first, S last, Comp comp = {}, Proj proj = {})
    {
        if (first == last) {
            return last;
        }
        auto smallest = first;
        while (++first != last) {
            if (std::invoke(comp, std::invoke(proj, *first), std::invoke(proj, *smallest))) {
                smallest = first;
            }
        }
        return smallest;
    }

    // see: std::ranges::min_element
    template<
        std::ranges::forward_range R,
        class Proj = std::identity,
        std::indirect_strict_weak_order<std::projected<std::ranges::iterator_t<R>, Proj>> Comp = std::ranges::less
    >
    constexpr std::ranges::borrowed_iterator_t<R> min_element(R&& r, Comp comp = {}, Proj proj = {})
    {
        return min_element(std::ranges::begin(r), std::ranges::end(r), std::ref(comp), std::ref(proj));
    }

    // osc algorithm: returns the index of the largest element in the range
    template<
        std::ranges::random_access_range R,
        class Proj = std::identity,
        std::indirect_strict_weak_order<std::projected<std::ranges::iterator_t<R>, Proj>> Comp = std::ranges::less
    >
    constexpr typename std::ranges::range_size_t<R> max_element_index(R&& r, Comp comp = {}, Proj proj = {})
    {
        auto const first = std::ranges::begin(r);
        return std::distance(first, max_element(first, std::ranges::end(r), std::ref(comp), std::ref(proj)));
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
