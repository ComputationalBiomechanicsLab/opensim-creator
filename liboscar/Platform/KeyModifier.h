#pragma once

#include <liboscar/Shims/Cpp23/utility.h>

namespace osc
{
    enum class KeyModifier : unsigned {
        None         = 0,
        Shift        = 1<<0,
        Ctrl         = 1<<1,
        Gui          = 1<<2,  // Windows key / MacOS command key / Ubuntu Key, etc.
        Alt          = 1<<3,
        NUM_FLAGS    =    4,

        CtrlORGui = Ctrl | Gui,
    };

    constexpr bool operator&(KeyModifier lhs, KeyModifier rhs)
    {
        return (cpp23::to_underlying(lhs) & cpp23::to_underlying(rhs)) != 0;
    }

    constexpr KeyModifier operator|(KeyModifier lhs, KeyModifier rhs)
    {
        return static_cast<KeyModifier>(cpp23::to_underlying(lhs) | cpp23::to_underlying(rhs));
    }

    constexpr KeyModifier& operator|=(KeyModifier& lhs, KeyModifier rhs)
    {
        lhs = lhs | rhs;
        return lhs;
    }
}
