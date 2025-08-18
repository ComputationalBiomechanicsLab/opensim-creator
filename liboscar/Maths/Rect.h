#pragma once

#include <liboscar/Maths/CommonFunctions.h>
#include <liboscar/Maths/RectCorners.h>
#include <liboscar/Maths/Vec2.h>

#include <iosfwd>
#include <utility>

namespace osc
{
    // Represents a 2D axis-aligned bounding box in a caller-defined coordinate
    // system in which X always points towards the right, but Y can point either up
    // (methods prefixed with `ypu_`) or down (methods prefixed with `ypd_`).
    //
    // The 1D equivalent to a `Rect` is a `ClosedInterval`. The 3D equivalent is an
    // `AABB`.
    class Rect final {
    public:
        // Returns a `Rect` with an `origin` of `point` and an area of zero (i.e.
        // dimensions = {0, 0}) in the coordinate system of `point`.
        static constexpr Rect from_point(const Vec2& point)
        {
            return Rect{point, Vec2{}};
        }

        // Returns a `Rect` with a centroid of `origin` and dimensions of `dimensions`.
        static constexpr Rect from_origin_and_dimensions(const Vec2& origin, const Vec2& dimensions)
        {
            return Rect{origin, dimensions};
        }

        // Returns a `Rect` constructed from two opposite corner points in the coordinate
        // system of those points.
        static Rect from_corners(const Vec2& p1, const Vec2& p2)
        {
            return Rect{0.5f * (p1 + p2), abs(p1 - p2)};
        }

        // Constructs an empty `Rect` with an `origin` of zero and `dimensions` of zero.
        Rect() = default;

        friend bool operator==(const Rect&, const Rect&) = default;

        // Returns the origin (centroid) of this `Rect` in its (caller-defined)
        // coordinate system.
        Vec2 origin() const { return origin_; }

        // Returns the dimensions of this `Rect`.
        Vec2 dimensions() const { return dimensions_; }

        // Returns the width of this `Rect`.
        float width() const { return dimensions_.x; }

        // Returns the height of this `Rect`.
        float height() const { return dimensions_.y; }

        // Returns the half extents of this `Rect`, which represents the distance from
        // the origin to the edge of the `Rect` in each dimension.
        Vec2 half_extents() const { return 0.5f * dimensions_; }

        // Returns the area of this `Rect`.
        float area() const { return dimensions_.x * dimensions_.y; }

        // Returns the X coordinate of this `Rect`'s left edge. Assumes X "points right"
        // in the coordinate system of this `Rect`.
        float left() const { return origin_.x - 0.5f*dimensions_.x; }

        // Returns the X coordinate of this `Rect`'s right edge. Assumes X "points right"
        // in the coordinate system of this `Rect`.
        float right() const { return origin_.x + 0.5f*dimensions_.x; }

        // Assuming Y "points down" in the coordinate system of this `Rect`, returns the
        // Y coordinate of this `Rect`'s top edge.
        float ypd_top() const { return origin_.y - 0.5f*dimensions_.y; }

        // Assuming Y "points down" in the coordinate system of this `Rect`, returns the
        // Y coordinate of this `Rect`'s bottom edge.
        float ypd_bottom() const { return origin_.y + 0.5f*dimensions_.y; }

        // Assuming Y "points up" in the coordinate system of this `Rect`, returns the
        // Y coordinate of this `Rect`'s top edge.
        float ypu_top() const { return origin_.y + 0.5f*dimensions_.y; }

        // Assuming Y "points up" in the coordinate system of this `Rect`, returns the
        // Y coordinate of this `Rect`'s bottom edge.
        float ypu_bottom() const { return origin_.y - 0.5f*dimensions_.y; }

        // Returns the minimum and maximum opposite corner points of this `Rect`.
        RectCorners corners() const
        {
            const Vec2 half_extentz = half_extents();
            return {.min = origin_ - half_extentz, .max = origin_ + half_extentz};
        }

        // Returns the minimum corner point of this `Rect`, which is the point in this
        // `Rect`'s coordinate system that has the smallest X and Y within the `Rect`'s
        // bounds.
        //
        // What minimum "means" depends on the coordinate system of this `Rect`:
        //
        // - If the `Rect`'s data is in a coordinate system where Y points down (e.g.
        //   the UI coordinate system), then it means "top left".
        // - If the `Rect`'s data is in a coordinate system where Y points up (e.g.
        //   viewport coordinate system), then it means "bottom left".
        Vec2 min_corner() const { return origin_ - half_extents(); }

        // Returns the maximum corner point of this `Rect`, which is the point in this `Rect`'s
        // coordinate system that has the smallest X and Y within the `Rect`'s bounds.
        //
        // What "maximum" contextually means depends on the coordinate system of this `Rect`:
        //
        // - If the `Rect`'s data is in a coordinate system where Y points down (e.g.
        //   the UI coordinate system), then it means "bottom right".
        // - If the `Rect`'s data is in a coordinate system where Y points up (e.g.
        //   viewport coordinate system), then it means "top right".
        Vec2 max_corner() const { return origin_ + half_extents(); }

        // Assuming Y "points down" in the coordinate system of this `Rect`, returns a point
        // that represents the top-left corner of this `Rect` in that coordinate system.
        Vec2 ypd_top_left() const { return min_corner(); }

        // Assuming Y "points down" in the coordinate system of this `Rect`, returns a point
        // that represents the top-right corner of this `Rect` in that coordinate system.
        Vec2 ypd_top_right() const { const auto c = corners(); return Vec2{c.max.x, c.min.y}; }

        // Assuming Y "points down" in the coordinate system of this `Rect`, returns a point
        // that represents the bottom-left corner of this `Rect` in that coordinate system.
        Vec2 ypd_bottom_left() const { const auto c = corners(); return Vec2{c.min.x, c.max.y}; }

        // Assuming Y "points down" in the coordinate system of this `Rect`, returns a point
        // that represents the bottom-right corner of this `Rect` in that coordinate system.
        Vec2 ypd_bottom_right() const { return max_corner(); }

        // Assuming Y "points up" in the coordinate system of this `Rect`, returns a point
        // that represents the top-left corner of this `Rect` in that coordinate system.
        Vec2 ypu_top_left() const { const auto c = corners(); return Vec2{c.min.x, c.max.y}; }

        // Assuming Y "points up" in the coordinate system of this `Rect`, returns a point
        // that represents the top-right corner of this `Rect` in that coordinate system.
        Vec2 ypu_top_right() const { return max_corner(); }

        // Assuming Y "points up" in the coordinate system of this `Rect`, returns a point
        // that represents the bottom-left corner of this `Rect` in that coordinate system.
        Vec2 ypu_bottom_left() const { return min_corner(); }

        // Assuming Y "points up" in the coordinate system of this `Rect`, returns a point
        // that represents the bottom-right corner of this `Rect` in that coordinate system.
        Vec2 ypu_bottom_right() const { const auto c = corners(); return Vec2{c.max.x, c.min.y}; }

        // Assuming the Y axis in the coordinate system of this `Rect` points in one direction
        // (up or down), returns a new `Rect` in a flipped coordinate system that has the
        // same scale and x axis origin, but has a new Y axis origin that point in the opposite
        // direction.
        //
        // `distance_between_x_axes` should be the distance between the source X axis
        // line (Y=0) and the flipped Y axis line. For example, if transforming between
        // the viewport coordinate space (ypu) to the ui coordinate space (ypd), the
        // `distance_between_x_axes` is the distance between the bottom of the viewport
        // (ypu) and the top of the viewport (ypd): i.e. the height of the viewport.
        Rect with_flipped_y(float distance_between_x_axes) const
        {
            return Rect{Vec2{origin_.x, distance_between_x_axes - origin_.y}, dimensions_};
        }

        // Returns a new `Rect` with the same `origin` as this `Rect`, but with the given
        // new dimensions.
        Rect with_dimensions(const Vec2& new_dimensions) const
        {
            return Rect{origin_, new_dimensions};
        }

        // Returns a new `Rect` with the same `origin` as this `Rect`, but with its `dimensions`
        // scaled by the given `scale_factors`
        Rect with_dimensions_scaled_by(const Vec2& scale_factors) const
        {
            return Rect{origin_, scale_factors * dimensions_};
        }

        // Returns a new `Rect` with and `origin` equivalent to `scale_factor * original_origin` and
        // `dimensions` equivalent to `scale_factor * original_dimensions`.
        Rect with_origin_and_dimensions_scaled_by(float scale_factor) const
        {
            return Rect{scale_factor * origin_, scale_factor * dimensions_};
        }
    private:
        // Constructs a `Rect` from its data members.
        explicit constexpr Rect(const Vec2& origin, const Vec2& dimensions) :
            origin_{origin},
            dimensions_{dimensions}
        {}

        Vec2 origin_{};
        Vec2 dimensions_{};
    };

    std::ostream& operator<<(std::ostream&, const Rect&);
}
