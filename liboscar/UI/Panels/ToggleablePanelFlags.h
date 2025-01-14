#pragma once

#include <liboscar/Shims/Cpp23/utility.h>

#include <cstdint>

namespace osc
{
    // flags for the panel (affects things like when it is initialized, whether the user
    // will be presented with the option to toggle it, etc.)
    enum class ToggleablePanelFlags : uint32_t {
        None               = 0u,
        IsEnabledByDefault = 1u<<0u,
        IsSpawnable        = 1u<<1u,  // ignores IsEnabledByDefault

        Default = IsEnabledByDefault,
    };

    constexpr ToggleablePanelFlags operator-(ToggleablePanelFlags lhs, ToggleablePanelFlags rhs)
    {
        return static_cast<ToggleablePanelFlags>(cpp23::to_underlying(lhs) & ~cpp23::to_underlying(rhs));
    }

    constexpr bool operator&(ToggleablePanelFlags lhs, ToggleablePanelFlags rhs)
    {
        return cpp23::to_underlying(lhs) & cpp23::to_underlying(rhs);
    }
}
