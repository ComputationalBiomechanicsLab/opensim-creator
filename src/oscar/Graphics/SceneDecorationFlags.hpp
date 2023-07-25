#pragma once

#include <cstdint>
#include <type_traits>

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

    constexpr SceneDecorationFlags operator|(SceneDecorationFlags a, SceneDecorationFlags b) noexcept
    {
        using T = std::underlying_type_t<SceneDecorationFlags>;
        return static_cast<SceneDecorationFlags>(static_cast<T>(a) | static_cast<T>(b));
    }

    constexpr SceneDecorationFlags& operator|=(SceneDecorationFlags& a, SceneDecorationFlags b) noexcept
    {
        return a = a | b;
    }

    constexpr bool operator&(SceneDecorationFlags a, SceneDecorationFlags b) noexcept
    {
        using T = std::underlying_type_t<SceneDecorationFlags>;
        return static_cast<T>(a) & static_cast<T>(b);
    }
}
