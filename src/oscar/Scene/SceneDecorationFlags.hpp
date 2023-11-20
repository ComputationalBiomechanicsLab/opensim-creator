#pragma once

#include <oscar/Shims/Cpp23/utility.hpp>

namespace osc
{
    enum class SceneDecorationFlags {
        None                 = 0,
        IsSelected           = 1u<<0u,
        IsChildOfSelected    = 1u<<1u,
        IsHovered            = 1u<<2u,
        IsChildOfHovered     = 1u<<3u,
        IsShowingOnly        = 1u<<4u,
        IsChildOfShowingOnly = 1u<<5u,
        CastsShadows         = 1u<<6u,
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
