#pragma once

#include <oscar/Utils/EnumHelpers.h>

#include <cstddef>
#include <optional>

namespace osc::fd
{
    // enumeration of the possible axes a user may define
    enum class AxisIndex {
        X,
        Y,
        Z,
        NUM_OPTIONS,
    };

    // returns the next `AxisIndex` in the circular sequence X -> Y -> Z
    constexpr AxisIndex Next(AxisIndex axis)
    {
        return static_cast<AxisIndex>((static_cast<size_t>(axis) + 1) % NumOptions<AxisIndex>());
    }
    static_assert(Next(AxisIndex::X) == AxisIndex::Y);
    static_assert(Next(AxisIndex::Y) == AxisIndex::Z);
    static_assert(Next(AxisIndex::Z) == AxisIndex::X);

    // returns a char representation of the given `AxisIndex`
    constexpr char ToChar(AxisIndex axis)
    {
        static_assert(NumOptions<AxisIndex>() == 3);
        switch (axis)
        {
        case AxisIndex::X:
            return 'x';
        case AxisIndex::Y:
            return 'y';
        case AxisIndex::Z:
        default:
            return 'z';
        }
    }

    // returns `c` parsed as an `AxisIndex`, or `std::nullopt` if the given char
    // cannot be parsed as an axis index
    constexpr std::optional<AxisIndex> ParseAxisIndex(char c)
    {
        switch (c)
        {
        case 'x':
        case 'X':
            return AxisIndex::X;
        case 'y':
        case 'Y':
            return AxisIndex::Y;
        case 'z':
        case 'Z':
            return AxisIndex::Z;
        default:
            return std::nullopt;
        }
    }

    // returns the integer index equivalent of the given `AxisIndex`
    constexpr ptrdiff_t ToIndex(AxisIndex axis)
    {
        return static_cast<ptrdiff_t>(axis);
    }
}
