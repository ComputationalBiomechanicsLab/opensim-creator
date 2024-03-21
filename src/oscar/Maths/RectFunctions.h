#pragma once

#include <oscar/Maths/CommonFunctions.h>
#include <oscar/Maths/Rect.h>
#include <oscar/Maths/Vec2.h>

#include <span>

namespace osc { struct Circle; }

namespace osc
{
    // returns the dimensions of `rect`
    inline Vec2 dimensions(Rect const& rect)
    {
        return abs(rect.p2 - rect.p1);
    }

    // returns the area of `rect`
    inline float area(Rect const& rect)
    {
        auto const [x, y] = dimensions(rect);
        return x * y;
    }

    // returns the aspect ratio of `rect`
    inline float aspect_ratio(Rect const& rect)
    {
        auto const [x, y] = dimensions(rect);
        return x / y;
    }

    // returns the middle point of `rect`
    constexpr Vec2 centroid(Rect const& rect)
    {
        return 0.5f * (rect.p1 + rect.p2);
    }

    // returns bottom-left point of the rectangle, assuming y points down (e.g. in 2D UIs)
    Vec2 bottom_left_lh(Rect const&);

    // returns the smallest rectangle that bounds the provided points
    //
    // note: no points --> zero-sized rectangle at the origin
    Rect bounding_rect_of(std::span<Vec2 const>);

    // returns the smallest rectangle that bounds the provided circle
    Rect bounding_rect_of(Circle const&);

    // returns a rectangle that has been expanded along each edge by the given amount
    //
    // (e.g. expand 1.0f adds 1.0f to both the left edge and the right edge)
    Rect expand(Rect const&, float);
    Rect expand(Rect const&, Vec2);

    // returns a rectangle where both p1 and p2 are clamped between min and max (inclusive)
    Rect Clamp(Rect const&, Vec2 const& min, Vec2 const& max);
}
