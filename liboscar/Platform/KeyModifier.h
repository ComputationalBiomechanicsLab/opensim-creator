#pragma once

#include <liboscar/Shims/Cpp23/utility.h>

namespace osc
{
    enum class KeyModifier : unsigned {
        None         = 0,
        LeftShift    = 1<<0,
        RightShift   = 1<<1,
        LeftCtrl     = 1<<2,
        RightCtrl    = 1<<3,
        LeftGui      = 1<<4,  // Windows key / MacOS command key / Ubuntu Key, etc.
        RightGui     = 1<<5,  // Windows key / MacOS command key / Ubuntu Key, etc.
        LeftAlt      = 1<<6,
        RightAlt     = 1<<7,
        NUM_FLAGS    =    8,

        Ctrl = LeftCtrl | RightCtrl,
        Shift = LeftShift | RightShift,
        Gui = LeftGui | RightGui,
        Alt = LeftAlt | RightAlt,
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
