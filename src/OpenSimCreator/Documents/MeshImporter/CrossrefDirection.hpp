#pragma once

#include <oscar/Shims/Cpp23/utility.hpp>

namespace osc::mi
{
    // returns the "direction" of a cross reference
    //
    // most of the time, the direction is towards whatever's being connected to,
    // but sometimes it can be the opposite, depending on how the datastructure
    // is ultimately used
    enum class CrossrefDirection {
        None     = 0,
        ToParent = 1<<0,
        ToChild  = 1<<1,

        Both = ToChild | ToParent
    };

    constexpr bool operator&(CrossrefDirection lhs, CrossrefDirection rhs)
    {
        return (osc::to_underlying(lhs) & osc::to_underlying(rhs)) != 0;
    }
}
