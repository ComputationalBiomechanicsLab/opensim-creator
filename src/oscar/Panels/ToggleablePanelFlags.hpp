#pragma once

#include <cstdint>
#include <type_traits>

namespace osc
{
    // flags for the panel (affects things like when it is initialized, whether the user
    // will be presented with the option to toggle it, etc.)
    enum class ToggleablePanelFlags : uint32_t {
        None = 0u,
        IsEnabledByDefault = 1u<<0u,
        IsSpawnable = 1u<<1u,  // ignores IsEnabledByDefault

        Default = IsEnabledByDefault,
    };

    constexpr ToggleablePanelFlags operator-(ToggleablePanelFlags a, ToggleablePanelFlags b) noexcept
    {
        using T = std::underlying_type_t<ToggleablePanelFlags>;
        return static_cast<ToggleablePanelFlags>(static_cast<T>(a) & ~static_cast<T>(b));
    }

    constexpr bool operator&(ToggleablePanelFlags a, ToggleablePanelFlags b) noexcept
    {
        using T = std::underlying_type_t<ToggleablePanelFlags>;
        return static_cast<T>(a) & static_cast<T>(b);
    }
}
