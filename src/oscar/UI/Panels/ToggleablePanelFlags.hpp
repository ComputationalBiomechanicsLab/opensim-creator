#pragma once

#include <oscar/Shims/Cpp23/utility.hpp>

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

    constexpr ToggleablePanelFlags operator-(ToggleablePanelFlags a, ToggleablePanelFlags b)
    {
        return static_cast<ToggleablePanelFlags>(osc::to_underlying(a) & ~osc::to_underlying(b));
    }

    constexpr bool operator&(ToggleablePanelFlags a, ToggleablePanelFlags b)
    {
        return osc::to_underlying(a) & osc::to_underlying(b);
    }
}
