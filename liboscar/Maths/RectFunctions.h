#pragma once

#include <liboscar/Maths/CommonFunctions.h>
#include <liboscar/Maths/Rect.h>
#include <liboscar/Maths/Vec2.h>

#include <concepts>
#include <functional>
#include <ranges>

namespace osc { struct Circle; }

namespace osc
{
    // returns the aspect ratio of `rect`
    inline float aspect_ratio_of(const Rect& rect)
    {
        const auto [x, y] = rect.dimensions();
        return x / y;
    }

    // returns a `Rect` that tightly bounds `x` (i.e. a `Rect` with an area of zero)
    constexpr Rect bounding_rect_of(const Vec2& x)
    {
        return Rect::from_point(x);
    }

    // returns a `Rect` that tightly bounds `x` and `y`
    inline Rect bounding_rect_of(const Rect& x, const Vec2& y)
    {
        const auto corners = x.corners();
        return Rect::from_corners(elementwise_min(corners.min, y), elementwise_max(corners.max, y));
    }

    // returns a `Rect` that tightly bounds `x` and `y`
    inline Rect bounding_rect_of(const Rect& x, const Rect& y)
    {
        const auto x_corners = x.corners();
        const auto y_corners = y.corners();
        return Rect::from_corners(
            elementwise_min(x_corners.min, y_corners.min),
            elementwise_max(x_corners.max, y_corners.max)
        );
    }

    // returns a `Rect` that tightly bounds the `Vec2`s projected from `r`
    template<
        std::ranges::input_range R,
        typename Proj = std::identity
    >
    requires std::convertible_to<typename std::projected<std::ranges::iterator_t<R>, Proj>::value_type, const Vec2&>
    constexpr Rect bounding_rect_of(R&& r, Proj proj = {})
    {
        auto it = std::ranges::begin(r);
        const auto last = std::ranges::end(r);
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
    Rect bounding_rect_of(const Circle& circle);

    // returns a `Rect` that has its bounds clamped between `min` and `max` (inclusive)
    Rect clamp(const Rect&, const Vec2& min, const Vec2& max);
}
