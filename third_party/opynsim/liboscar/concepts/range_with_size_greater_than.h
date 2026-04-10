#pragma once

#include <concepts>
#include <cstddef>
#include <ranges>
#include <tuple>

namespace osc
{
    namespace detail
    {
        template<typename T> concept HasTupleSize = requires { std::tuple_size<std::remove_cvref_t<T>>{}; };
        template<typename T> concept HasExtent = requires { { std::remove_cvref_t<T>::extent } -> std::convertible_to<size_t>; };

        template<typename T, typename = void>
        struct SizeHelper { static constexpr size_t size = std::dynamic_extent; };

        template<HasTupleSize T>
        requires (not HasExtent<T>)
        struct SizeHelper<T> { static constexpr size_t size = std::tuple_size_v<std::remove_cvref_t<T>>; };

        template<HasExtent T>
        struct SizeHelper<T> { static constexpr size_t size = std::remove_cvref_t<T>::extent; };
    }

    // Satisfied if `T` has a compile-time-known size greater than `N`
    template<typename T, size_t N>
    concept RangeWithSizeGreaterThan = std::ranges::range<T> and (detail::SizeHelper<T>::size != std::dynamic_extent and detail::SizeHelper<T>::size > N);
}
