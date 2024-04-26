#pragma once

#include <oscar/Utils/Concepts.h>

#include <algorithm>
#include <cstddef>
#include <functional>
#include <iterator>
#include <memory>
#include <optional>
#include <ranges>
#include <stdexcept>
#include <utility>

namespace osc
{
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

    // see: std::ranges::max
    template<
        class T,
        class Proj = std::identity,
        std::indirect_strict_weak_order<std::projected<const T*, Proj>> Comp = std::ranges::less
    >
    constexpr const T& max(const T& a, const T& b, Comp comp = {}, Proj proj = {})
    {
        return std::invoke(comp, std::invoke(proj, a), std::invoke(proj, b)) ? b : a;
    }

    // see: std::ranges::max
    template<
        std::copyable T,
        class Proj = std::identity,
        std::indirect_strict_weak_order<std::projected<const T*, Proj>> Comp = std::ranges::less
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
    requires std::indirectly_copyable_storable<std::ranges::iterator_t<R>, std::ranges::range_value_t<R>*>
    constexpr std::ranges::range_value_t<R> max(R&& r, Comp comp = {}, Proj proj = {})
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

    // see: std::ranges::min
    template<
        class T,
        class Proj = std::identity,
        std::indirect_strict_weak_order<std::projected<const T*, Proj>> Comp = std::ranges::less
    >
    constexpr const T& min(const T& a, const T& b, Comp comp = {}, Proj proj = {})
    {
        return std::invoke(comp, std::invoke(proj, b), std::invoke(proj, a)) ? b : a;
    }

    // see: std::ranges::min
    template<
        std::copyable T,
        class Proj = std::identity,
        std::indirect_strict_weak_order<std::projected<const T*, Proj>> Comp = std::ranges::less
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
    requires std::indirectly_copyable_storable<std::ranges::iterator_t<R>, std::ranges::range_value_t<R>*>
    constexpr std::ranges::range_value_t<R> min(R&& r, Comp comp = {}, Proj proj = {})
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

    // returned by min_max algs (see: `std::ranges:min_max_result`)
    template<typename T>
    struct min_max_result final {
        [[no_unique_address]] T min;
        [[no_unique_address]] T max;

        template<typename T2>
        requires std::convertible_to<const T&, T2>
        constexpr operator min_max_result<T2>() const &
        {
            return {min, max};
        }

        template<typename T2>
        requires std::convertible_to<T, T2>
        constexpr operator min_max_result<T2>() &&
        {
            return {std::move(min), std::move(max)};
        }
    };

    template<typename T>
    using minmax_result = min_max_result<T>;

    template<typename I>
    using minmax_element_result = min_max_result<I>;

    // see: std::ranges::minmax_element
    template<
        std::forward_iterator I,
        std::sentinel_for<I> S,
        typename Proj = std::identity,
        std::indirect_strict_weak_order<std::projected<I, Proj>> Comp = std::ranges::less
    >
    constexpr minmax_element_result<I> minmax_element(
        I first,
        S last,
        Comp comp = {},
        Proj proj = {})
    {
        auto min = first;
        auto max = first;

        // handle no range or singular edge-case
        if (first == last or ++first == last) {
            return {min, max};
        }

        // create minmax invariant from first two elements
        if (std::invoke(comp, std::invoke(proj, *first), std::invoke(proj, *min))) {
            min = first;
        }
        else {
            max = first;
        }

        // loop over each element and reestablish invariant
        while (++first != last) {

            // C++ spec: if several elements are equivalent to the smallest, then the iterator to
            //           the first such element is returned. If several elements are equivalent to
            //           the largest element, the iterator to the last such element is returned
            //
            // (which is why there's this unusual-looking double-looping going on)

            auto i = first;
            if (++first == last) {
                if (std::invoke(comp, std::invoke(proj, *i), std::invoke(proj, *min))) {
                    min = i;
                }
                else if (not std::invoke(comp, std::invoke(proj, *i), std::invoke(proj, *max))) {
                    max = i;
                }
                break;
            }
            else {
                if (std::invoke(comp, std::invoke(proj, *first), std::invoke(proj, *i))) {
                    if (std::invoke(comp, std::invoke(proj, *first), std::invoke(proj, *min))) {
                        min = first;
                    }
                    if (not std::invoke(comp, std::invoke(proj, *i), std::invoke(proj, *max))) {
                        max = i;
                    }
                }
                else {
                    if (std::invoke(comp, std::invoke(proj, *i), std::invoke(proj, *min))) {
                        min = i;
                    }
                    if (not std::invoke(comp, std::invoke(proj, *first), std::invoke(proj, *max))) {
                        max = first;
                    }
                }
            }
        }

        return {min, max};
    }

    // see: std::ranges::minmax_element
    template<
        std::ranges::forward_range R,
        typename Proj = std::identity,
        std::indirect_strict_weak_order<std::projected<std::ranges::iterator_t<R>, Proj>> Comp = std::ranges::less
    >
    constexpr minmax_element_result<std::ranges::borrowed_iterator_t<R>> minmax_element(R&& r, Comp comp = {}, Proj proj = {})
    {
        return osc::minmax_element(std::ranges::begin(r), std::ranges::end(r), std::ref(comp), std::ref(proj));
    }

    // see: std::ranges::minmax
    template<
        typename T,
        typename Proj = std::identity,
        std::indirect_strict_weak_order<std::projected<const T*, Proj>> Comp = std::ranges::less
    >
    constexpr minmax_result<const T&> minmax(const T& a, const T& b, Comp comp = {}, Proj proj = {})
    {
        if (std::invoke(comp, std::invoke(proj, b), std::invoke(proj, a))) {
            return {b, a};
        }

        return {a, b};
    }

    // see: std::ranges::minmax
    template<
        std::copyable T,
        typename Proj = std::identity,
        std::indirect_strict_weak_order<std::projected<const T*, Proj>> Comp = std::ranges::less
    >
    constexpr minmax_result<T> minmax(std::initializer_list<T> r, Comp comp = {}, Proj proj = {})
    {
        auto result = minmax_element(r, std::ref(comp), std::ref(proj));
        return {*result.min, *result.max};
    }

    // see: std::ranges::minmax
    template<
        std::ranges::input_range R,
        typename Proj = std::identity,
        std::indirect_strict_weak_order<std::projected<std::ranges::iterator_t<R>, Proj>> Comp = std::ranges::less
    >
    requires std::indirectly_copyable_storable<std::ranges::iterator_t<R>, std::ranges::range_value_t<R>*>
    constexpr minmax_result<std::ranges::range_value_t<R>> minmax(R&& r, Comp comp = {}, Proj proj = {})
    {
        auto result = minmax_element(r, std::ref(comp), std::ref(proj));
        return {std::move(*result.min), std::move(*result.max)};
    }

    // see: std::ranges::clamp
    template<
        class T,
        class Proj = std::identity,
        std::indirect_strict_weak_order<std::projected<const T*, Proj>> Comp = std::ranges::less
    >
    constexpr const T& clamp(const T& v, const T& lo, const T& hi, Comp comp = {}, Proj proj = {})
    {
        auto&& pv = std::invoke(proj, v);

        if (std::invoke(comp, std::forward<decltype(pv)>(pv), std::invoke(proj, lo))) {
            return lo;
        }
        if (std::invoke(comp, std::invoke(proj, hi), std::forward<decltype(pv)>(pv))) {
            return hi;
        }
        return v;
    }

    // osc algorithm: returns the index of the largest element in the range
    template<
        std::ranges::random_access_range R,
        class Proj = std::identity,
        std::indirect_strict_weak_order<std::projected<std::ranges::iterator_t<R>, Proj>> Comp = std::ranges::less
    >
    constexpr typename std::ranges::range_size_t<R> max_element_index(R&& r, Comp comp = {}, Proj proj = {})
    {
        const auto first = std::ranges::begin(r);
        return std::distance(first, max_element(first, std::ranges::end(r), std::ref(comp), std::ref(proj)));
    }

    // osc algorithm: perform bounds-checked indexed access
    template<std::ranges::random_access_range Range>
    constexpr auto at(const Range& range, typename Range::size_type i) -> decltype(range[i])
    {
        if (i < std::ranges::size(range)) {
            return range[i];
        }
        else {
            throw std::out_of_range{"out of bounds index given to a container"};
        }
    }

    // osc algorithm: returns a `std::optional<T>` containing the value located at `key`, or `std::nullopt` if no such element exists in `container`
    template<AssociativeContainer T, typename Key>
    std::optional<typename T::mapped_type> find_or_optional(const T& container, const Key& key)
    {
        if (auto it = container.find(key); it != container.end()) {
            return it->second;
        }
        return std::nullopt;
    }

    // osc algorithm: returns a pointer to the element at `key`, or `nullptr` if no such element exists in `container`
    template<AssociativeContainer T, typename Key>
    const typename T::mapped_type* try_find(const T& container, const Key& key)
    {
        if (auto it = container.find(key); it != container.end()) {
            return &it->second;
        }
        return nullptr;
    }

    // osc algorithm: returns a mutable pointer to the element at `key`, or `nullptr` if no such element exists in `container`
    template<AssociativeContainer T, typename Key>
    typename T::mapped_type* try_find(T& container, const Key& key)
    {
        if (auto it = container.find(key); it != container.end()) {
            return &it->second;
        }
        return nullptr;
    }

    template<typename Downcasted, typename T1, typename T2>
    requires
        std::equality_comparable<Downcasted> and
        std::is_polymorphic_v<Downcasted> and
        std::derived_from<Downcasted, T1> and
        std::derived_from<Downcasted, T2>
    constexpr bool is_eq_downcasted(const T1& lhs, const T2& rhs)
    {
        const auto* lhs_casted = dynamic_cast<const Downcasted*>(std::addressof(lhs));
        const auto* rhs_casted = dynamic_cast<const Downcasted*>(std::addressof(rhs));
        if (lhs_casted and rhs_casted) {
            return *lhs_casted == *rhs_casted;
        }
        return false;
    }
}
