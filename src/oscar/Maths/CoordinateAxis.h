#pragma once

#include <oscar/Utils/Assertions.h>

#include <cstdint>
#include <compare>
#include <optional>
#include <iosfwd>
#include <stdexcept>
#include <string_view>

namespace osc
{
    // provides convenient manipulation of the three coordinate axes (X, Y, Z)
    //
    // inspired by simbody's `SimTK::CoordinateAxis` class
    class CoordinateAxis final {
    public:
        // returns a `CoordinateAxis` parsed from a `std::string_view`, the format can only be one of:
        //
        //     "x", "X", "y", "Y", "z", "Z"
        //
        // returns `std::nullopt` if the input string is incorrect
        static std::optional<CoordinateAxis> try_parse(std::string_view);

        // returns a `CoordinateAxis` that represents the X axis
        static constexpr CoordinateAxis x()
        {
            return CoordinateAxis{0};
        }

        // returns a `CoordinateAxis` that represents the Y axis
        static constexpr CoordinateAxis y()
        {
            return CoordinateAxis{1};
        }

        // returns a `CoordinateAxis` that represents the Z axis
        static constexpr CoordinateAxis z()
        {
            return CoordinateAxis{2};
        }

        // default-constructs a `CoordinateAxis` that represents the X axis
        constexpr CoordinateAxis() = default;

        // constructs a `CoordinateAxis` from a runtime integer that must be `0`, `1`, or `2`, representing X, Y, or Z axis
        explicit constexpr CoordinateAxis(int axis_index) :
            axis_index_{static_cast<uint8_t>(axis_index)}
        {
            OSC_ASSERT(0 <= axis_index && axis_index <= 2 && "out-of-range index given to a CoordinateAxis");
        }

        // `CoordinateAxis`es are equality-comparable and totally ordered as X < Y < Z
        friend constexpr auto operator<=>(const CoordinateAxis&, const CoordinateAxis&) = default;

        // returns the index of the axis (i.e. X == 0, Y == 1, Z == 2)
        constexpr size_t index() const
        {
            return static_cast<size_t>(axis_index_);
        }

        // returns the previous axis in the ring sequence X -> Y -> Z -> X...
        constexpr CoordinateAxis previous() const
        {
            return CoordinateAxis{static_cast<uint8_t>((axis_index_ + 2) % 3), SkipRangeCheck{}};
        }

        // returns the next axis in the ring sequence X -> Y -> Z -> X...
        constexpr CoordinateAxis next() const
        {
            return CoordinateAxis{static_cast<uint8_t>((axis_index_ + 1) % 3), SkipRangeCheck{}};
        }
    private:
        struct SkipRangeCheck {};
        explicit constexpr CoordinateAxis(uint8_t v, SkipRangeCheck) :
            axis_index_{v}
        {}

        uint8_t axis_index_ = 0;
    };

    std::ostream& operator<<(std::ostream&, CoordinateAxis);
}
