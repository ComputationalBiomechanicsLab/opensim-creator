#pragma once

#include <cstdint>

namespace osc
{
    // flags for the panel (affects things like when it is initialized, whether the user
    // will be presented with the option to toggle it, etc.)
    using ToggleablePanelFlags = int32_t;
    enum ToggleablePanelFlags_ {
        ToggleablePanelFlags_None = 0,
        ToggleablePanelFlags_IsEnabledByDefault = 1<<0,
        ToggleablePanelFlags_Default = ToggleablePanelFlags_IsEnabledByDefault,
    };
}