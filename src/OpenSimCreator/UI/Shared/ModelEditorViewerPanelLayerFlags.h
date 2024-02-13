#pragma once

#include <oscar/Shims/Cpp23/utility.h>

#include <cstdint>

namespace osc
{
    enum class ModelEditorViewerPanelLayerFlags : uint32_t {
        None                = 0,
        CapturesMouseInputs = 1<<0,

        Default = CapturesMouseInputs,
    };

    constexpr bool operator&(ModelEditorViewerPanelLayerFlags lhs, ModelEditorViewerPanelLayerFlags rhs)
    {
        return cpp23::to_underlying(lhs) & cpp23::to_underlying(rhs);
    }

    constexpr ModelEditorViewerPanelLayerFlags operator|(ModelEditorViewerPanelLayerFlags lhs, ModelEditorViewerPanelLayerFlags rhs)
    {
        return static_cast<ModelEditorViewerPanelLayerFlags>(cpp23::to_underlying(lhs) | cpp23::to_underlying(rhs));
    }

    constexpr ModelEditorViewerPanelLayerFlags& operator|=(ModelEditorViewerPanelLayerFlags& lhs, ModelEditorViewerPanelLayerFlags rhs)
    {
        return lhs = lhs | rhs;
    }
}
