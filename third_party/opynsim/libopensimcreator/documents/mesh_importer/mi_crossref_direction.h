#pragma once

#include <utility>

namespace osc
{
    // returns the "direction" of a cross reference
    //
    // most of the time, the direction is towards whatever's being connected to,
    // but sometimes it can be the opposite, depending on how the datastructure
    // is ultimately used
    enum class MiCrossrefDirection {
        None     = 0,
        ToParent = 1<<0,
        ToChild  = 1<<1,

        Both = ToChild | ToParent
    };

    constexpr bool operator&(MiCrossrefDirection lhs, MiCrossrefDirection rhs)
    {
        return (std::to_underlying(lhs) & std::to_underlying(rhs)) != 0;
    }
}
