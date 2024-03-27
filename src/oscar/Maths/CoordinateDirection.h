#pragma once

#include <oscar/Maths/CoordinateAxis.h>
#include <oscar/Maths/Negative.h>
#include <oscar/Maths/Vec3.h>

#include <cstdint>
#include <optional>
#include <iosfwd>
#include <string_view>
#include <type_traits>

namespace osc
{
    // a `CoordinateAxis` plus a direction along that axis
    //
    // inspired by simbody's `SimTK::CoordinateDirection` class
    class CoordinateDirection final {
    public:
        // returns a `CoordinateDirection` parsed from a `std::string_view`, the format should be [direction]axis, e.g.:
        //
        //     -x, +x, x, -X, +X, X, -y,...
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
        constexpr CoordinateDirection(CoordinateAxis axis) : m_Axis{axis} {}

        // constructs a `CoordinateDirection` that points negatively along `axis`
        constexpr CoordinateDirection(CoordinateAxis axis, Negative) : m_Axis{axis}, m_Direction{-1} {}

        // `CoordinateDirection`s are equality comparable and totally ordered as -X < +X < -Y < +Y < -Z < +Z
        friend constexpr auto operator<=>(CoordinateDirection const&, CoordinateDirection const&) = default;

        // returns the `CoordinateAxis` that this `CoordinateDirection` points along
        constexpr CoordinateAxis axis() const { return m_Axis; }

        // tests if the `CoordinateDirection` is pointing negatively along its axis
        constexpr bool is_negated() const { return m_Direction == -1; }

        // returns T{-1} if this `CoordinateDirection` points negatively along its axis; otherwise, returns `T{1}`
        template<typename T = float>
        requires std::is_arithmetic_v<T> and std::is_signed_v<T>
        constexpr T direction() const
        {
            return static_cast<T>(m_Direction);
        }

        // returns a direction that points in the direction stored by this `CoordinateDirection`
        template<typename T = float>
        requires std::is_arithmetic_v<T>
        constexpr Vec<3, T> vec() const
        {
            return Vec<3, T>{}.with_element(axis().index(), direction<T>());
        }

        // returns a `CoordinateDirection` that points along the same `CoordinateAxis`, but with its direction negated
        constexpr CoordinateDirection operator-() const
        {
            return CoordinateDirection{m_Axis, static_cast<int8_t>(-1 * m_Direction)};
        }
    private:
        friend constexpr CoordinateDirection cross(CoordinateDirection, CoordinateDirection);

        explicit constexpr CoordinateDirection(CoordinateAxis axis, int8_t direction) :
            m_Axis{axis},
            m_Direction{direction}
        {}

        CoordinateAxis m_Axis;
        int8_t m_Direction = 1;
    };

    // writes the `CoordinateDirection` to the ouptut stream in a human-readable form (e.g. "x", "-x")
    std::ostream& operator<<(std::ostream&, CoordinateDirection);

    // returns the equivalent `CoordinateDirection` that `cross(UnitVec3{x}, UnitVec3{y})` would point along,
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
