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
    // returns the greater of the given projected values
    template<
        class T,
        class Proj = std::identity,
        std::indirect_strict_weak_order<std::projected<const T*, Proj>> Comp = std::ranges::less
    >
    constexpr const T& max(const T& a, const T& b, Comp comp = {}, Proj proj = {})
    {
        return std::ranges::max(a, b, std::ref(comp), std::ref(proj));
    }

    // returns the smaller of the given projected elements
    template<
        class T,
        class Proj = std::identity,
        std::indirect_strict_weak_order<std::projected<const T*, Proj>> Comp = std::ranges::less
    >
    constexpr const T& min(const T& a, const T& b, Comp comp = {}, Proj proj = {})
    {
        return std::ranges::min(a, b, std::ref(comp), std::ref(proj));
    }

    // if `v` compares less than `lo`, returns `lo`; otherwise, if `hi` compares less than `v`, returns `hi`; otherwise, returns `v`
    template<
        class T,
        class Proj = std::identity,
        std::indirect_strict_weak_order<std::projected<const T*, Proj>> Comp = std::ranges::less
    >
    constexpr const T& clamp(const T& v, const T& lo, const T& hi, Comp comp = {}, Proj proj = {})
    {
        return std::ranges::clamp(v, lo, hi, std::ref(comp), std::ref(proj));
    }

    // returns the index of the largest element in the range
    template<
        std::ranges::random_access_range R,
        class Proj = std::identity,
        std::indirect_strict_weak_order<std::projected<std::ranges::iterator_t<R>, Proj>> Comp = std::ranges::less
    >
    constexpr typename std::ranges::range_size_t<R> max_element_index(R&& r, Comp comp = {}, Proj proj = {})
    {
        const auto first = std::ranges::begin(r);
        return std::distance(first, std::ranges::max_element(first, std::ranges::end(r), std::ref(comp), std::ref(proj)));
    }

    // returns a reference to the element at specified location `pos`, with bounds checking
    template<std::ranges::random_access_range Range>
    constexpr auto at(const Range& range, typename Range::size_type pos) -> decltype(range[pos])
    {
        if (pos < std::ranges::size(range)) {
            return range[pos];
        }
        else {
            throw std::out_of_range{"out of bounds index given to a container"};
        }
    }

    // returns the element that has a projection that compares equivalent to `value`, or `std::nullopt` if not found
    template<
        std::ranges::input_range R,
        typename T,
        typename Proj = std::identity
    >
    requires
        std::indirect_binary_predicate<std::ranges::equal_to, std::projected<std::ranges::iterator_t<R>, Proj>, const T*> and
        (not AssociativeContainer<R>)
    std::optional<typename R::value_type> find_or_optional(R&& r, const T& value, Proj proj = {})
    {
        const auto it = std::ranges::find(r, value, std::ref(proj));
        return it != std::ranges::end(r) ? std::optional<typename R::value_type>{*it} : std::nullopt;
    }

    // returns the element with key equivalent to `key`, or `std::nullopt` if no such element exists in `container`
    template<AssociativeContainer T, typename Key>
    std::optional<typename T::mapped_type> find_or_optional(const T& container, const Key& key)
    {
        if (auto it = container.find(key); it != container.end()) {
            return it->second;
        }
        return std::nullopt;
    }

    // returns a pointer to the element with a projection that compares equal to `value`, or `nullptr` if no such element exists in `r`
    template<
        std::ranges::input_range R,
        typename T,
        typename Proj = std::identity
    >
    auto find_or_nullptr(R&& r, const T& value, Proj proj = {})
    {
        const auto it = std::ranges::find(r, value, std::move(proj));
        return it != std::ranges::end(r) ? std::addressof(*it) : nullptr;
    }

    // returns a pointer to the element with key equivalent to `key`, or `nullptr` if no such element exists in `container`
    template<AssociativeContainer T, typename Key>
    const typename T::mapped_type* find_or_nullptr(const T& container, const Key& key)
    {
        if (auto it = container.find(key); it != container.end()) {
            return &it->second;
        }
        return nullptr;
    }

    // returns a mutable pointer to the element with key equivalent to `key`, or `nullptr` if no such element exists in `container`
    template<AssociativeContainer T, typename Key>
    typename T::mapped_type* find_or_nullptr(T& container, const Key& key)
    {
        if (auto it = container.find(key); it != container.end()) {
            return &it->second;
        }
        return nullptr;
    }

    // returns `true` if both `lhs` and `rhs` can be sucessfully `dynamic_cast`ed to `Downcasted` and compare equal
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
