#pragma once

#include <liboscar/Maths/CoordinateAxis.h>
#include <liboscar/Maths/Negative.h>
#include <liboscar/Maths/Vector3.h>

#include <cstdint>
#include <optional>
#include <iosfwd>
#include <string_view>
#include <type_traits>

namespace osc
{
    // A `CoordinateAxis` plus a direction along that axis.
    class CoordinateDirection final {
    public:
        // returns a `CoordinateDirection` parsed from a `std::string_view`, the format should be [direction]axis, e.g.:
        //
        //     "-x", "+x", "x", "-X", "+X", "X", "-y", etc...
        //
        // returns `std::nullopt` if the input string is incorrect
        static std::optional<CoordinateDirection> try_parse(std::string_view);

        // returns a `CoordinateDirection` that represents the positive X direction
        static constexpr CoordinateDirection x()
        {
            return CoordinateDirection{CoordinateAxis::x()};
        }

        // returns a `CoordinateDirection` that represents the negative X direction
        static constexpr CoordinateDirection minus_x()
        {
            return CoordinateDirection{CoordinateAxis::x(), Negative{}};
        }

        // returns a `CoordinateDirection` that represents the positive Y direction
        static constexpr CoordinateDirection y()
        {
            return CoordinateDirection{CoordinateAxis::y()};
        }

        // returns a `CoordinateDirection` that represents the negative Y direction
        static constexpr CoordinateDirection minus_y()
        {
            return CoordinateDirection{CoordinateAxis::y(), Negative{}};
        }

        // returns a `CoordinateDirection` that represents the positive Z direction
        static constexpr CoordinateDirection z()
        {
            return CoordinateDirection{CoordinateAxis::z()};
        }

        // returns a `CoordinateDirection` that represents the negative Z direction
        static constexpr CoordinateDirection minus_z()
        {
            return CoordinateDirection{CoordinateAxis::z(), Negative{}};
        }

        // default-constructs a `CoordinateDirection` that points in the positive X direction
        constexpr CoordinateDirection() = default;

        // constructs a `CoordinateDirection` that points positively along `axis`
        constexpr CoordinateDirection(CoordinateAxis axis) : axis_{axis} {}

        // constructs a `CoordinateDirection` that points negatively along `axis`
        constexpr CoordinateDirection(CoordinateAxis axis, Negative) : axis_{axis}, direction_{-1} {}

        // `CoordinateDirection`s are equality comparable and totally ordered as -X < +X < -Y < +Y < -Z < +Z
        friend constexpr auto operator<=>(const CoordinateDirection&, const CoordinateDirection&) = default;

        // returns the `CoordinateAxis` that this `CoordinateDirection` points along
        constexpr CoordinateAxis axis() const { return axis_; }

        // tests if the `CoordinateDirection` is pointing negatively along its axis
        constexpr bool is_negated() const { return direction_ == -1; }

        // returns T{-1} if this `CoordinateDirection` points negatively along its axis; otherwise, returns `T{1}`
        template<typename T = float>
        requires std::is_arithmetic_v<T> and std::is_signed_v<T>
        constexpr T direction() const
        {
            return static_cast<T>(direction_);
        }

        // returns a unit-length direction vector that points in the direction stored by this `CoordinateDirection`
        template<typename T = float>
        requires std::is_arithmetic_v<T>
        constexpr Vector<3, T> direction_vector() const
        {
            return Vector<3, T>{}.with_element(axis().index(), direction<T>());
        }

        // returns a `CoordinateDirection` that points along the same `CoordinateAxis`, but with its direction negated
        constexpr CoordinateDirection operator-() const
        {
            return CoordinateDirection{axis_, static_cast<int8_t>(-1 * direction_)};
        }
    private:
        friend constexpr CoordinateDirection cross(CoordinateDirection, CoordinateDirection);

        explicit constexpr CoordinateDirection(CoordinateAxis axis, int8_t direction) :
            axis_{axis},
            direction_{direction}
        {}

        CoordinateAxis axis_;
        int8_t direction_ = 1;
    };

    // writes the `CoordinateDirection` to the ouptut stream in a human-readable form (e.g. "x", "-x")
    std::ostream& operator<<(std::ostream&, CoordinateDirection);

    // returns the equivalent `CoordinateDirection` that `cross(normalize(Vector3{x}), normalize(Vector3{y}))` would point along,
    // or `x` if both `x` and `y` point along the same axis (i.e. have a zero cross product)
    constexpr CoordinateDirection cross(CoordinateDirection x, CoordinateDirection y)
    {
        if (x.axis() == y.axis()) {
            return x;
        }
        else if (x.axis().next() == y.axis()) {
            return CoordinateDirection{y.axis().next(), static_cast<int8_t>(x.direction<int>()*y.direction<int>())};
        }
        else {
            return CoordinateDirection{x.axis().next(), static_cast<int8_t>(-1*x.direction<int>()*y.direction<int>())};
        }
    }
}
