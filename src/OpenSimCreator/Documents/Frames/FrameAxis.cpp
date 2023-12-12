#include "FrameAxis.hpp"

#include <oscar/Shims/Cpp23/utility.hpp>
#include <oscar/Utils/EnumHelpers.hpp>

#include <iostream>
#include <optional>
#include <string_view>

using osc::frames::FrameAxis;
using osc::NumOptions;

namespace
{
    std::string_view ToStringView(FrameAxis fa)
    {
        static_assert(NumOptions<FrameAxis>() == 6);

        switch (fa)
        {
        case FrameAxis::PlusX: return "x";
        case FrameAxis::PlusY: return "y";
        case FrameAxis::PlusZ: return "z";
        case FrameAxis::MinusX: return "-x";
        case FrameAxis::MinusY: return "-y";
        case FrameAxis::MinusZ: return "-z";
        default:
            return "unknown";
        }
    }
}

std::optional<FrameAxis> osc::frames::TryParseAsFrameAxis(std::string_view s)
{
    if (s.empty() || s.size() > 2)
    {
        return std::nullopt;
    }
    bool const negated = s.front() == '-';
    if (negated || s.front() == '+')
    {
        s = s.substr(1);
    }
    if (s.empty())
    {
        return std::nullopt;
    }
    switch (s.front())
    {
    case 'x':
    case 'X':
        return negated ? FrameAxis::MinusX : FrameAxis::PlusX;
    case 'y':
    case 'Y':
        return negated ? FrameAxis::MinusY : FrameAxis::PlusY;
    case 'z':
    case 'Z':
        return negated ? FrameAxis::MinusZ : FrameAxis::PlusZ;
    default:
        return std::nullopt;  // invalid input
    }
}

bool osc::frames::AreOrthogonal(FrameAxis a, FrameAxis b)
{
    static_assert(osc::to_underlying(FrameAxis::PlusX) == 0);
    static_assert(osc::to_underlying(FrameAxis::MinusX) == 3);
    static_assert(NumOptions<FrameAxis>() == 6);
    return osc::to_underlying(a) % 3 != osc::to_underlying(b) % 3;
}

std::ostream& osc::frames::operator<<(std::ostream& o, FrameAxis f)
{
    return o << ToStringView(f);
}

