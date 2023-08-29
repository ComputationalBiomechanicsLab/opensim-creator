#pragma once

#include <oscar/Maths/AABB.hpp>
#include <oscar/Utils/CStringView.hpp>

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <nonstd/span.hpp>

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <type_traits>

namespace osc { class OpenSimDecorationOptions; }
namespace osc { class OverlayDecorationOptions; }
namespace osc { class CustomRenderingOptions; }
namespace osc { class IconCache; }
namespace osc { class MainUIStateAPI; }
namespace osc { struct ModelRendererParams; }
namespace osc { template<typename T> class ParentPtr; }
namespace osc { class ParamBlock; }
namespace osc { struct PolarPerspectiveCamera; }
namespace osc { struct Rect; }
namespace osc { struct SceneDecoration; }
namespace osc { class UndoableModelStatePair; }
namespace osc { class VirtualModelStatePair; }
namespace osc { class VirtualOutputExtractor; }
namespace osc { class SimulationModelStatePair; }
namespace OpenSim { class Component; }
namespace OpenSim { class Point; }
namespace OpenSim { class Frame; }
namespace SimTK { class State; }

namespace osc
{
    void DrawComponentHoverTooltip(OpenSim::Component const&);
    void DrawNothingRightClickedContextMenuHeader();
    void DrawRightClickedComponentContextMenuHeader(OpenSim::Component const&);
    void DrawContextMenuSeparator();
    void DrawSelectOwnerMenu(VirtualModelStatePair&, OpenSim::Component const&);
    bool DrawWatchOutputMenu(MainUIStateAPI&, OpenSim::Component const&);
    void DrawSimulationParams(ParamBlock const&);
    void DrawSearchBar(std::string&);
    void DrawOutputNameColumn(
        VirtualOutputExtractor const& output,
        bool centered = true,
        SimulationModelStatePair* maybeActiveSate = nullptr
    );

    // draws a "With Respect to" menu that prompts the user to hover a frame
    // within the given component hierarchy (from `root`)
    //
    // calls `onFrameMenuOpened` when the user is hovering a frame's menu
    // (i.e. `ImGui::BeginMenu($FRAME)` returned `true`)
    void DrawWithRespectToMenuContainingMenuPerFrame(
        OpenSim::Component const& root,
        std::function<void(OpenSim::Frame const&)> const& onFrameMenuOpened
    );

    // draws a "With Respect to" menu that prompts the user to click a frame
    // within the given component hierarchy (from `root`)
    //
    // calls `onFrameMenuItemClicked` when the user clicks the `ImGui::MenuItem`
    // associated with a frame
    void DrawWithRespectToMenuContainingMenuItemPerFrame(
        OpenSim::Component const& root,
        std::function<void(OpenSim::Frame const&)> const& onFrameMenuItemClicked
    );

    void DrawPointTranslationInformationWithRespectTo(
        OpenSim::Frame const&,
        SimTK::State const&,
        glm::vec3 locationInGround
    );
    void DrawDirectionInformationWithRepsectTo(
        OpenSim::Frame const&,
        SimTK::State const&,
        glm::vec3 directionInGround
    );
    void DrawFrameInformationExpressedIn(
        OpenSim::Frame const& parent,
        SimTK::State const&,
        OpenSim::Frame const&
    );

    enum class CalculateMenuFlags {
        None,
        NoCalculatorIcon,
    };

    constexpr bool operator&(CalculateMenuFlags a, CalculateMenuFlags b) noexcept
    {
        auto const aV = static_cast<std::underlying_type_t<CalculateMenuFlags>>(a);
        auto const bV = static_cast<std::underlying_type_t<CalculateMenuFlags>>(b);
        return (aV & bV) != 0;
    }

    void DrawCalculateMenu(
        OpenSim::Component const& root,
        SimTK::State const&,
        OpenSim::Point const&,
        CalculateMenuFlags = CalculateMenuFlags::None
    );
    void DrawCalculateMenu(
        OpenSim::Component const& root,
        SimTK::State const&,
        OpenSim::Frame const&,
        CalculateMenuFlags = CalculateMenuFlags::None
    );
    void TryDrawCalculateMenu(
        OpenSim::Component const& root,
        SimTK::State const&,
        OpenSim::Component const& selected,
        CalculateMenuFlags = CalculateMenuFlags::None
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

    // toolbar stuff
    bool BeginToolbar(CStringView label, std::optional<glm::vec2> padding = {});  // behaves the same as ImGui::Begin (i.e. you must call ImGui::End)
    void DrawNewModelButton(ParentPtr<MainUIStateAPI> const&);
    void DrawOpenModelButtonWithRecentFilesDropdown(ParentPtr<MainUIStateAPI> const&);
    void DrawSaveModelButton(ParentPtr<MainUIStateAPI> const&, UndoableModelStatePair&);
    void DrawReloadModelButton(UndoableModelStatePair&);
    void DrawUndoButton(UndoableModelStatePair&);
    void DrawRedoButton(UndoableModelStatePair&);
    void DrawUndoAndRedoButtons(UndoableModelStatePair&);
    void DrawToggleFramesButton(UndoableModelStatePair&, IconCache&);
    void DrawToggleMarkersButton(UndoableModelStatePair&, IconCache&);
    void DrawToggleWrapGeometryButton(UndoableModelStatePair&, IconCache&);
    void DrawToggleContactGeometryButton(UndoableModelStatePair&, IconCache&);
    void DrawAllDecorationToggleButtons(UndoableModelStatePair&, IconCache&);
    void DrawSceneScaleFactorEditorControls(UndoableModelStatePair&);
}
