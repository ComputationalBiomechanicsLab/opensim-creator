#pragma once

#include <oscar/Shims/Cpp23/utility.hpp>

namespace osc
{
    enum class SceneDecorationFlags {
        None                 = 0,
        IsSelected           = 1<<0,
        IsChildOfSelected    = 1<<1,
        IsHovered            = 1<<2,
        IsChildOfHovered     = 1<<3,
        IsShowingOnly        = 1<<4,
        IsChildOfShowingOnly = 1<<5,
        CastsShadows         = 1<<6,
    };

    constexpr SceneDecorationFlags operator|(SceneDecorationFlags lhs, SceneDecorationFlags rhs)
    {
        return static_cast<SceneDecorationFlags>(osc::to_underlying(lhs) | osc::to_underlying(rhs));
    }

    constexpr SceneDecorationFlags& operator|=(SceneDecorationFlags& lhs, SceneDecorationFlags rhs)
    {
        return lhs = lhs | rhs;
    }

    constexpr bool operator&(SceneDecorationFlags lhs, SceneDecorationFlags rhs)
    {
        return osc::to_underlying(lhs) & osc::to_underlying(rhs);
    }
}
