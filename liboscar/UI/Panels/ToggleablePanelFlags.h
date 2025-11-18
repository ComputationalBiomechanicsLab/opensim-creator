#pragma once

#include <cstdint>
#include <utility>

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
        return static_cast<ToggleablePanelFlags>(std::to_underlying(lhs) & ~std::to_underlying(rhs));
    }

    constexpr bool operator&(ToggleablePanelFlags lhs, ToggleablePanelFlags rhs)
    {
        return std::to_underlying(lhs) & std::to_underlying(rhs);
    }
}
