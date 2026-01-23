#pragma once

#include <nanobind/nanobind.h>
#include <nanobind/ndarray.h>
#include <libopynsim/shims/cpp23/mdspan.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <utility>

namespace opyn
{
    // Internal implementation details for `to_mdspan`.
    namespace detail
    {
        // Converts a `nb::ssize_t`, as used by `nb::shape`, into the equivalent  `std::extent`, as
        // used by `std::mdspan`.
        template<nanobind::ssize_t NbExtent>
        constexpr size_t to_stdlib_extent() { return static_cast<size_t>(NbExtent); }
        template<>
        constexpr size_t to_stdlib_extent<nanobind::ssize_t{-1}>() { return std::dynamic_extent; }

        // Converts an `nb::shape`, as used by `nb::ndarray` into the equivalent `std::extents`, as
        // used by `std::mdspan`.
        template<nanobind::ssize_t... NbExtents, size_t... I>
        requires (sizeof...(NbExtents) == sizeof...(I))
        constexpr auto to_stdlib_extents(
            nanobind::shape<NbExtents...>,
            const int64_t* runtime_extents,
            std::index_sequence<I...>) -> opyn::cpp23::extents<size_t, to_stdlib_extent<NbExtents>()...>
        {
            return opyn::cpp23::extents<size_t, to_stdlib_extent<NbExtents>()...>{runtime_extents[I]...};
        }

        template<nanobind::ssize_t... NbExtents>
        constexpr auto to_stdlib_extents(
            nanobind::shape<NbExtents...> shape,
            const int64_t* extents) -> opyn::cpp23::extents<size_t, to_stdlib_extent<NbExtents>()...>
        {
            return to_stdlib_extents(shape, extents, std::make_index_sequence<sizeof...(NbExtents)>{});
        }

        // A type that's the equivalent `std::extents` of the given `nb::shape`.
        template<typename NbShape>
        using ToStdlibExtents = decltype(to_stdlib_extents(std::declval<NbShape>(), std::declval<const int64_t*>()));

        // Returns an array of strides that's compatible with `std::layout_stride::mapping`.
        template<typename T, nanobind::ssize_t... NbExtents>
        std::array<size_t, sizeof...(NbExtents)> to_stdlib_strides(
            const nanobind::ndarray<T, nanobind::shape<NbExtents...>,
            nanobind::device::cpu>& ndary)
        {
            std::array<size_t, sizeof...(NbExtents)> rv{};
            for (size_t i = 0; i < sizeof...(NbExtents); ++i) {
                rv[i] = static_cast<size_t>(ndary.stride(i));
            }
            return rv;
        }
    }

    // Returns a non-owning `std::mdspan` view of the given `nb::ndarray`.
    template<typename T, typename Shape>
    constexpr auto to_mdspan(nanobind::ndarray<T, Shape, nanobind::device::cpu> ndary)
        -> opyn::cpp23::mdspan<T, detail::ToStdlibExtents<Shape>, opyn::cpp23::layout_stride>
    {
        return {
            ndary.data(),
            opyn::cpp23::layout_stride::mapping{detail::to_stdlib_extents(Shape{}, ndary.shape_ptr()), detail::to_stdlib_strides(ndary)}
        };
    }
}
