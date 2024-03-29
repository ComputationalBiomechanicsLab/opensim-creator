#pragma once

#include <oscar/Maths/CommonFunctions.h>
#include <oscar/Maths/Rect.h>
#include <oscar/Maths/Vec2.h>

#include <concepts>
#include <functional>
#include <ranges>
#include <span>

namespace osc { struct Circle; }

namespace osc
{
    // returns the dimensions of `rect`
    inline Vec2 dimensions_of(Rect const& rect)
    {
        return abs(rect.p2 - rect.p1);
    }

    // returns the area of `rect`
    inline float area_of(Rect const& rect)
    {
        auto const [x, y] = dimensions_of(rect);
        return x * y;
    }

    // returns the aspect ratio of `rect`
    inline float aspect_ratio(Rect const& rect)
    {
        auto const [x, y] = dimensions_of(rect);
        return x / y;
    }

    // returns the middle point of `rect`
    constexpr Vec2 centroid_of(Rect const& rect)
    {
        return 0.5f * (rect.p1 + rect.p2);
    }

    // returns bottom-left point of the rectangle, assuming y points down (e.g. in 2D UIs)
    Vec2 bottom_left_lh(Rect const&);

    // returns a `Rect` that tightly bounds `x` (i.e. a `Rect` with an area of zero)
    constexpr Rect bounding_rect_of(Vec2 const& x)
    {
        return Rect{x, x};
    }

    // returns a `Rect` that tightly bounds `x` and `y`
    constexpr Rect bounding_rect_of(Rect const& x, Vec2 const& y)
    {
        return Rect{elementwise_min(x.p1, y), elementwise_max(x.p2, y)};
    }

    // returns a `Rect` that tightly bounds `x` and `y`
    constexpr Rect bounding_rect_of(Rect const& x, Rect const& y)
    {
        return Rect{elementwise_min(x.p1, y.p1), elementwise_max(x.p2, y.p2)};
    }

    // returns a `Rect` that tightly bounds the `Vec2`s projected from `r`
    template<
        std::ranges::input_range R,
        typename Proj = std::identity
    >
    requires std::convertible_to<typename std::projected<std::ranges::iterator_t<R>, Proj>::value_type, Vec2 const&>
    constexpr Rect bounding_rect_of(R&& r, Proj proj = {})
    {
        auto it = std::ranges::begin(r);
        auto const last = std::ranges::end(r);
        if (it == last) {
            return Rect{};  // empty range
        }

        Rect rv = bounding_rect_of(std::invoke(proj, *it));
        while (++it != last) {
            rv = bounding_rect_of(rv, std::invoke(proj, *it));
        }
        return rv;
    }

    // returns a `Rect` that tightly bounds `circle`
    Rect bounding_rect_of(Circle const& circle);

    // returns a `Rect` calculated by adding `absAmount` to each edge of `rect`
    Rect expand(Rect const& rect, float absAmount);

    // returns a `Rect` calculated by adding `absAmount` to each edge of `rect`
    Rect expand(Rect const& rect, Vec2 absAmount);

    // returns a `Rect` that has its bounds clamped between `min` and `max` (inclusive)
    Rect clamp(Rect const&, Vec2 const& min, Vec2 const& max);
}
