#pragma once

#include <oscar/Maths/AABB.hpp>

#include <nonstd/span.hpp>

#include <functional>
#include <optional>
#include <string>

namespace osc { class OpenSimDecorationOptions; }
namespace osc { class OverlayDecorationOptions; }
namespace osc { class CustomRenderingOptions; }
namespace osc { class IconCache; }
namespace osc { class MainUIStateAPI; }
namespace osc { struct ModelRendererParams; }
namespace osc { struct PolarPerspectiveCamera; }
namespace osc { class ParamBlock; }
namespace osc { struct Rect; }
namespace osc { struct SceneDecoration; }
namespace osc { class VirtualModelStatePair; }
namespace osc { class VirtualOutputExtractor; }
namespace osc { class SimulationModelStatePair; }
namespace OpenSim { class Component; }

namespace osc
{
    void DrawComponentHoverTooltip(OpenSim::Component const&);
    void DrawNothingRightClickedContextMenuHeader();
    void DrawRightClickedComponentContextMenuHeader(OpenSim::Component const&);
    void DrawContextMenuSeparator();
    void DrawSelectOwnerMenu(VirtualModelStatePair&, OpenSim::Component const&);
    bool DrawWatchOutputMenu(MainUIStateAPI&, OpenSim::Component const&);
    void DrawSimulationParams(ParamBlock const&);
    void DrawSearchBar(std::string&, int maxLen);
    void DrawOutputNameColumn(
        VirtualOutputExtractor const& output,
        bool centered = true,
        SimulationModelStatePair* maybeActiveSate = nullptr
    );

    // basic wigetized parts of the 3D viewer
    void DrawMuscleRenderingOptionsRadioButtions(OpenSimDecorationOptions&);
    void DrawMuscleSizingOptionsRadioButtons(OpenSimDecorationOptions&);
    void DrawMuscleColoringOptionsRadioButtons(OpenSimDecorationOptions&);
    void DrawMuscleDecorationOptionsEditor(OpenSimDecorationOptions&);
    void DrawRenderingOptionsEditor(CustomRenderingOptions&);
    void DrawOverlayOptionsEditor(OverlayDecorationOptions&);
    void DrawCustomDecorationOptionCheckboxes(OpenSimDecorationOptions&);
    void DrawAdvancedParamsEditor(ModelRendererParams&, nonstd::span<SceneDecoration const>);
    void DrawVisualAidsContextMenuContent(ModelRendererParams&);
    void DrawViewerTopButtonRow(
        ModelRendererParams&,
        nonstd::span<SceneDecoration const>,
        IconCache&,
        std::function<void()> const& drawExtraElements = []() {}
    );
    void DrawCameraControlButtons(
        PolarPerspectiveCamera&,
        Rect const&,
        std::optional<AABB> const& maybeSceneAABB,
        IconCache&
    );
    void DrawViewerImGuiOverlays(
        ModelRendererParams&,
        nonstd::span<SceneDecoration const>,
        std::optional<AABB>,
        Rect const&,
        IconCache&,
        std::function<void()> const& drawExtraElementsInTop = []() {}
    );
}