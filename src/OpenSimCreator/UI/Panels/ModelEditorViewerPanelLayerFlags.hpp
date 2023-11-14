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

    constexpr bool operator&(ModelEditorViewerPanelLayerFlags a, ModelEditorViewerPanelLayerFlags b) noexcept
    {
        return osc::to_underlying(a) & osc::to_underlying(b);
    }

    constexpr ModelEditorViewerPanelLayerFlags operator|(ModelEditorViewerPanelLayerFlags a, ModelEditorViewerPanelLayerFlags b) noexcept
    {
        return static_cast<ModelEditorViewerPanelLayerFlags>(osc::to_underlying(a) | osc::to_underlying(b));
    }

    constexpr ModelEditorViewerPanelLayerFlags& operator|=(ModelEditorViewerPanelLayerFlags& a, ModelEditorViewerPanelLayerFlags b) noexcept
    {
        return a = a | b;
    }
}
