#pragma once

#include <oscar/Shims/Cpp23/utility.hpp>

#include <cstdint>

namespace osc
{
    enum class ModelEditorViewerPanelLayerFlags : uint32_t {
        None                = 0u,
        CapturesMouseInputs = 1u<<0u,

        Default = CapturesMouseInputs,
    };

    constexpr bool operator&(ModelEditorViewerPanelLayerFlags lhs, ModelEditorViewerPanelLayerFlags rhs)
    {
        return osc::to_underlying(lhs) & osc::to_underlying(rhs);
    }

    constexpr ModelEditorViewerPanelLayerFlags operator|(ModelEditorViewerPanelLayerFlags lhs, ModelEditorViewerPanelLayerFlags rhs)
    {
        return static_cast<ModelEditorViewerPanelLayerFlags>(osc::to_underlying(lhs) | osc::to_underlying(rhs));
    }

    constexpr ModelEditorViewerPanelLayerFlags& operator|=(ModelEditorViewerPanelLayerFlags& lhs, ModelEditorViewerPanelLayerFlags rhs)
    {
        return lhs = lhs | rhs;
    }
}
