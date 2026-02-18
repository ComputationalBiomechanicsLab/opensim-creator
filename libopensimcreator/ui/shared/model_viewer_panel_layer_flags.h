#pragma once

#include <cstdint>
#include <utility>

namespace osc
{
    enum class ModelViewerPanelLayerFlags : uint32_t {
        None                = 0,
        CapturesMouseInputs = 1<<0,

        Default = CapturesMouseInputs,
    };

    constexpr bool operator&(ModelViewerPanelLayerFlags lhs, ModelViewerPanelLayerFlags rhs)
    {
        return std::to_underlying(lhs) & std::to_underlying(rhs);
    }

    constexpr ModelViewerPanelLayerFlags operator|(ModelViewerPanelLayerFlags lhs, ModelViewerPanelLayerFlags rhs)
    {
        return static_cast<ModelViewerPanelLayerFlags>(std::to_underlying(lhs) | std::to_underlying(rhs));
    }

    constexpr ModelViewerPanelLayerFlags& operator|=(ModelViewerPanelLayerFlags& lhs, ModelViewerPanelLayerFlags rhs)
    {
        return lhs = lhs | rhs;
    }
}
