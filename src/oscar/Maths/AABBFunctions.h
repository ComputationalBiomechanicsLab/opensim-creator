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
#include <iterator>
#include <optional>
#include <ranges>
#include <span>

namespace osc
{
    // returns the average centroid of the `AABB`
    constexpr Vec3 centroid(AABB const& aabb)
    {
        return 0.5f * (aabb.min + aabb.max);
    }

    // returns the widths of the edges of the `AABB`
    constexpr Vec3 dimensions(AABB const& aabb)
    {
        return aabb.max - aabb.min;
    }

    // returns the half-widths of the edges of the `AABB`
    constexpr Vec3 half_widths(AABB const& aabb)
    {
        return 0.5f * dimensions(aabb);
    }

    // returns the volume of the `AABB`
    constexpr float volume(AABB const& aabb)
    {
        Vec3 const dims = dimensions(aabb);
        return dims.x * dims.y * dims.z;
    }

    // returns `true` if the `AABB` has zero width along all of its edges (i.e. min == max)
    constexpr bool is_point(AABB const& aabb)
    {
        return aabb.min == aabb.max;
    }

    // returns `true` if the `AABB` has zero width along any of its edges
    constexpr bool has_zero_volume(AABB const& aabb)
    {
        return volume(aabb) == 0.0f;
    }

    // returns the eight corner points of the `AABB`'s cuboid
    std::array<Vec3, 8> cuboid_vertices(AABB const&);

    // returns an `AABB` that has been transformed by the `Mat4`
    AABB transform_aabb(AABB const&, Mat4 const&);

    // returns an `AABB` that has been transformed by the `Transform`
    AABB transform_aabb(AABB const&, Transform const&);

    // returns an `AABB` that tightly bounds the `Vec3`
    constexpr AABB aabb_of(Vec3 const& v)
    {
        return AABB{.min = v, .max = v};
    }

    // returns an `AABB` that tightly bounds the two `AABB`s (union)
    constexpr AABB aabb_of(AABB const& x, AABB const& y)
    {
        return AABB{elementwise_min(x.min, y.min), elementwise_max(x.max, y.max)};
    }

    // returns an `AABB` that tightly bounds the two `AABB`s, or only the second `AABB` if the first is `std::nullopt`
    constexpr AABB aabb_of(std::optional<AABB> const& x, AABB const& y)
    {
        return x ? aabb_of(*x, y) : y;
    }

    // returns an `AABB` that tightly bounds the `Vec3`s (projected from) the range
    template<std::ranges::input_range Range, class Proj = std::identity>
    constexpr AABB aabb_of(Range&& range, Proj proj = {})
        requires std::convertible_to<typename std::projected<std::ranges::iterator_t<Range>, Proj>::value_type, Vec3 const&>
    {
        using std::begin;
        using std::end;

        auto it = begin(range);
        auto const en = end(range);
        if (it == en) {
            return AABB{};  // empty range
        }

        AABB rv = aabb_of(static_cast<Vec3 const&>(proj(*it)));
        while (++it != en) {
            rv = aabb_of(rv, aabb_of(static_cast<Vec3 const&>(proj(*it))));
        }
        return rv;
    }

    // returns an `AABB` that tightly bounds the `AABB`s (projected from) the range
    template<std::ranges::input_range Range, class Proj = std::identity>
    constexpr AABB aabb_of(Range&& range, Proj proj = {})
        requires std::convertible_to<typename std::projected<std::ranges::iterator_t<Range>, Proj>::value_type, AABB const&>
    {
        using std::begin;
        using std::end;

        auto it = begin(range);
        auto const en = end(range);
        if (it == en) {
            return AABB{};  // empty range
        }

        AABB rv = proj(*it);
        while (++it != en) {
            rv = aabb_of(rv, static_cast<AABB const&>(proj(*it)));
        }
        return rv;
    }

    // returns an `AABB` that tightly bounds any non-`std::nullopt` `AABB`s in `x` or `y`
    //
    // returns `std::nullopt` if both `x` and `y` are `std::nullopt`
    constexpr std::optional<AABB> maybe_aabb_of(std::optional<AABB> x, std::optional<AABB> y)
    {
        if (x && y) {
            return aabb_of(*x, *y);
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

    // returns an `AABB` that tightly bounds any non-`std::nullopt` `AABB`s (projected from) the range
    //
    // returns `std::nullopt` if no element in the range returns an `AABB`
    template<std::ranges::input_range Range, class Proj = std::identity>
    constexpr std::optional<AABB> maybe_aabb_of(Range&& range, Proj proj = {})
        requires std::convertible_to<typename std::projected<std::ranges::iterator_t<Range>, Proj>::value_type, std::optional<AABB> const&>
    {
        using std::begin;
        using std::end;

        auto it = begin(range);
        auto const en = end(range);

        // find first non-nullopt AABB (or the end)
        std::optional<AABB> rv;
        while (!rv && it != en) {
            rv = proj(*it++);
        }

        // combine with remainder of range
        for (; it != en; ++it) {
            rv = aabb_of(proj(*it), *rv);
        }

        return rv;
    }

    // returns a `Rect` in normalized device coordinate (NDC) space that loosely
    // bounds the given worldspace `AABB`
    //
    // - returns `std::nullopt` if the AABB does not lie within the NDC clipping
    //   bounds (i.e. between (-1, -1) to (1, 1)
    // - if it does return a rectangle, the AABB is somewhere within the rectangle,
    //   but it isn't guaranteed to be a good fit (loose bounds)
    // - this function is mostly useful for figuring out what part of a screen is
    //   affected by the AABB (e.g. to minimze/skip rendering)
    std::optional<Rect> loosely_project_into_ndc(
        AABB const&,
        Mat4 const& viewMat,
        Mat4 const& projMat,
        float znear,
        float zfar
    );
}
