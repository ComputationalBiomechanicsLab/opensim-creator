#pragma once

#include <oscar/Shims/Cpp23/utility.h>

namespace osc
{
    enum class SceneDecorationFlags {
        None                 = 0,
        IsSelected           = 1<<0,  // is the active user selection (so highlight it)
        IsChildOfSelected    = 1<<1,  // is a child of the active user selection (so highlight it)
        IsHovered            = 1<<2,  // is currently hovered by the mouse (so highlight it, but fainter)
        IsChildOfHovered     = 1<<3,  // is a child of the mouse hover (so highlight it, but fainter)
        IsShowingOnly        = 1<<4,  // is the only thing being shown (helpful for culling)
        IsChildOfShowingOnly = 1<<5,  // is a child of the only thing being shown (helpful for culling)
        CastsShadows         = 1<<6,  // casts shadows on other elements in the scene
        WireframeOverlay     = 1<<7,  // in addition to everything else, draw the geometry as a wireframe
        NoDrawNormally       = 1<<8,  // disable drawing "normally" (i.e. in the scene) but still draw wireframes, cast shadows, rims, etc.
    };

    constexpr SceneDecorationFlags operator|(SceneDecorationFlags lhs, SceneDecorationFlags rhs)
    {
        return static_cast<SceneDecorationFlags>(cpp23::to_underlying(lhs) | cpp23::to_underlying(rhs));
    }

    constexpr SceneDecorationFlags& operator|=(SceneDecorationFlags& lhs, SceneDecorationFlags rhs)
    {
        return lhs = lhs | rhs;
    }

    constexpr bool operator&(SceneDecorationFlags lhs, SceneDecorationFlags rhs)
    {
        return cpp23::to_underlying(lhs) & cpp23::to_underlying(rhs);
    }
}
