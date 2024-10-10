#pragma once

#include <oscar/Shims/Cpp23/utility.h>

#include <cstdint>

namespace osc
{
    enum class ModelViewerPanelLayerFlags : uint32_t {
        None                = 0,
        CapturesMouseInputs = 1<<0,

        Default = CapturesMouseInputs,
    };

    constexpr bool operator&(ModelViewerPanelLayerFlags lhs, ModelViewerPanelLayerFlags rhs)
    {
        return cpp23::to_underlying(lhs) & cpp23::to_underlying(rhs);
    }

    constexpr ModelViewerPanelLayerFlags operator|(ModelViewerPanelLayerFlags lhs, ModelViewerPanelLayerFlags rhs)
    {
        return static_cast<ModelViewerPanelLayerFlags>(cpp23::to_underlying(lhs) | cpp23::to_underlying(rhs));
    }

    constexpr ModelViewerPanelLayerFlags& operator|=(ModelViewerPanelLayerFlags& lhs, ModelViewerPanelLayerFlags rhs)
    {
        return lhs = lhs | rhs;
    }
}
