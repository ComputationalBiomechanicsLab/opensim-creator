#pragma once

#include <oscar/Maths/AABB.h>
#include <oscar/Maths/CommonFunctions.h>
#include <oscar/Maths/Mat4.h>
#include <oscar/Maths/Rect.h>
#include <oscar/Maths/Transform.h>
#include <oscar/Maths/Triangle.h>
#include <oscar/Maths/Vec3.h>

#include <array>
#include <cstdint>
#include <concepts>
#include <functional>
#include <optional>
#include <ranges>

namespace osc
{
    // returns the average centroid of `aabb`
    constexpr Vec3 centroid_of(AABB const& aabb)
    {
        return 0.5f * (aabb.min + aabb.max);
    }

    // returns the widths of the edges of `aabb`
    constexpr Vec3 dimensions_of(AABB const& aabb)
    {
        return aabb.max - aabb.min;
    }

    // returns the half-widths of the edges of `aabb`
    constexpr Vec3 half_widths(AABB const& aabb)
    {
        return 0.5f * dimensions_of(aabb);
    }

    // returns the volume of `aabb`
    constexpr float volume_of(AABB const& aabb)
    {
        Vec3 const dims = dimensions_of(aabb);
        return dims.x * dims.y * dims.z;
    }

    // tests if `aabb` has zero width along all of its edges (i.e. min == max)
    constexpr bool is_point(AABB const& aabb)
    {
        return aabb.min == aabb.max;
    }

    // tests if `aabb` has zero width along any of its edges
    constexpr bool has_zero_volume(AABB const& aabb)
    {
        return volume_of(aabb) == 0.0f;
    }

    // returns the eight corner vertices of `aabb`
    std::array<Vec3, 8> corner_vertices(AABB const& aabb);

    // returns an `AABB` computed by transforming `aabb` with `m`
    AABB transform_aabb(Mat4 const& m, AABB const& aabb);

    // returns an `AABB` computed by transforming `aabb` with `t`
    AABB transform_aabb(Transform const& t, AABB const& aabb);

    // returns an `AABB` that tightly bounds `x`
    constexpr AABB bounding_aabb_of(Vec3 const& x)
    {
        return AABB{.min = x, .max = x};
    }

    // returns an `AABB` that tightly bounds both `x` and `y`
    constexpr AABB bounding_aabb_of(AABB const& x, Vec3 const& y)
    {
        return AABB{elementwise_min(x.min, y), elementwise_max(x.max, y)};
    }

    // returns an `AABB` that tightly bounds both `x` and `y`
    constexpr AABB bounding_aabb_of(AABB const& x, AABB const& y)
    {
        return AABB{elementwise_min(x.min, y.min), elementwise_max(x.max, y.max)};
    }

    // returns an `AABB` that tightly bounds the union of `x` and `y`, or only `y` if `x` is `std::nullopt`
    constexpr AABB bounding_aabb_of(std::optional<AABB> const& x, AABB const& y)
    {
        return x ? bounding_aabb_of(*x, y) : y;
    }

    // returns an `AABB` that tightly bounds the `Vec3`s projected from `r`
    template<std::ranges::input_range R, class Proj = std::identity>
    requires std::convertible_to<typename std::projected<std::ranges::iterator_t<R>, Proj>::value_type, Vec3 const&>
    constexpr AABB bounding_aabb_of(R&& r, Proj proj = {})
    {
        auto it = std::ranges::begin(r);
        auto const last = std::ranges::end(r);
        if (it == last) {
            return AABB{};  // empty range
        }

        AABB rv = bounding_aabb_of(std::invoke(proj, *it));
        while (++it != last) {
            rv = bounding_aabb_of(rv, std::invoke(proj, *it));
        }
        return rv;
    }

    // returns an `AABB` that tightly bounds the `AABB`s projected from `r`
    template<std::ranges::input_range Range, class Proj = std::identity>
    requires std::convertible_to<typename std::projected<std::ranges::iterator_t<Range>, Proj>::value_type, AABB const&>
    constexpr AABB bounding_aabb_of(Range&& r, Proj proj = {})
    {
        auto it = std::ranges::begin(r);
        auto const last = std::ranges::end(r);
        if (it == last) {
            return AABB{};  // empty range
        }

        AABB rv = std::invoke(proj, *it);
        while (++it != last) {
            rv = bounding_aabb_of(rv, std::invoke(proj, *it));
        }
        return rv;
    }

    // returns an `AABB` that tightly bounds any non-`std::nullopt` `AABB`s in `x` or `y`
    //
    // if both `x` and `y` are `std::nullopt`, returns `std::nullopt`
    constexpr std::optional<AABB> maybe_bounding_aabb_of(std::optional<AABB> x, std::optional<AABB> y)
    {
        if (x && y) {
            return bounding_aabb_of(*x, *y);
        }
        else if (x) {
            return *x;
        }
        else if (y) {
            return *y;
        }
        else {
            return std::nullopt;
        }
    }

    // returns an `AABB` that tightly bounds any non-`std::nullopt` `std::optional<AABB>`s projected from `r`
    //
    // if no element in `r` projects an `AABB`, returns `std::nullopt`
    template<std::ranges::input_range Range, class Proj = std::identity>
    requires std::convertible_to<typename std::projected<std::ranges::iterator_t<Range>, Proj>::value_type, std::optional<AABB> const&>
    constexpr std::optional<AABB> maybe_bounding_aabb_of(Range&& r, Proj proj = {})
    {
        auto it = std::ranges::begin(r);
        auto const last = std::ranges::end(r);

        // find first non-nullopt AABB (or the end)
        std::optional<AABB> rv;
        while (!rv && it != last) {
            rv = std::invoke(proj, *it++);
        }

        // combine with remainder of range
        for (; it != last; ++it) {
            rv = bounding_aabb_of(std::invoke(proj, *it), *rv);
        }

        return rv;
    }

    // returns a `Rect` in normalized device coordinate (NDC) space that loosely
    // bounds the worldspace-located `aabb`
    //
    // if the AABB does not lie within the NDC clipping bounds (i.e. between (-1, -1)
    // and (1, 1)), returns `std::nullopt`
    std::optional<Rect> loosely_project_into_ndc(
        AABB const& aabb,
        Mat4 const& viewMat,
        Mat4 const& projMat,
        float znear,
        float zfar
    );
}
