#pragma once

#include <cstdint>
#include <type_traits>

namespace osc
{
    enum class ModelEditorViewerPanelLayerFlags : uint32_t {
        None                = 0u,
        CapturesMouseInputs = 1u<<0u,

        Default = CapturesMouseInputs,
    };

    constexpr bool operator&(ModelEditorViewerPanelLayerFlags a, ModelEditorViewerPanelLayerFlags b) noexcept
    {
        using T = std::underlying_type_t<ModelEditorViewerPanelLayerFlags>;
        return static_cast<T>(a) & static_cast<T>(b);
    }

    constexpr ModelEditorViewerPanelLayerFlags operator|(ModelEditorViewerPanelLayerFlags a, ModelEditorViewerPanelLayerFlags b) noexcept
    {
        using T = std::underlying_type_t<ModelEditorViewerPanelLayerFlags>;
        return static_cast<ModelEditorViewerPanelLayerFlags>(static_cast<T>(a) | static_cast<T>(b));
    }

    constexpr ModelEditorViewerPanelLayerFlags& operator|=(ModelEditorViewerPanelLayerFlags& a, ModelEditorViewerPanelLayerFlags b) noexcept
    {
        return a = a | b;
    }
}
