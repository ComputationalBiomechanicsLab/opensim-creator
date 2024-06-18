#pragma once

#include <OpenSimCreator/Documents/OutputExtractors/ComponentOutputSubfield.h>

#include <oscar/Maths/AABB.h>
#include <oscar/Maths/Vec2.h>
#include <oscar/Maths/Vec3.h>
#include <oscar/Shims/Cpp23/utility.h>
#include <oscar/Utils/CStringView.h>

#include <filesystem>
#include <functional>
#include <optional>
#include <span>
#include <string>

namespace osc { class OpenSimDecorationOptions; }
namespace osc { class OverlayDecorationOptions; }
namespace osc { class CustomRenderingOptions; }
namespace osc { class IconCache; }
namespace osc { class IMainUIStateAPI; }
namespace osc { struct ModelRendererParams; }
namespace osc { template<typename T> class ParentPtr; }
namespace osc { class ParamBlock; }
namespace osc { struct PolarPerspectiveCamera; }
namespace osc { struct Rect; }
namespace osc { struct SceneDecoration; }
namespace osc { class UndoableModelStatePair; }
namespace osc { class IModelStatePair; }
namespace osc { class IOutputExtractor; }
namespace osc { class SimulationModelStatePair; }
namespace OpenSim { class AbstractOutput; }
namespace OpenSim { class Component; }
namespace OpenSim { class Ellipsoid; }
namespace OpenSim { class Frame; }
namespace OpenSim { class Geometry; }
namespace OpenSim { class Mesh; }
namespace OpenSim { class Point; }
namespace OpenSim { class Sphere; }
namespace OpenSim { class Station; }
namespace SimTK { class State; }

namespace osc
{
    void DrawComponentHoverTooltip(
        const OpenSim::Component&
    );
    void DrawNothingRightClickedContextMenuHeader();
    void DrawContextMenuHeader(
        CStringView title,
        CStringView subtitle
    );
    void DrawRightClickedComponentContextMenuHeader(
        const OpenSim::Component&
    );
    void DrawContextMenuSeparator();
    void DrawSelectOwnerMenu(
        IModelStatePair&,
        const OpenSim::Component&
    );
    bool DrawRequestOutputMenuOrMenuItem(
        const OpenSim::AbstractOutput& o,
        std::function<void(const OpenSim::AbstractOutput&, std::optional<ComponentOutputSubfield>)> const& onUserSelection
    );
    bool DrawWatchOutputMenu(
        const OpenSim::Component&,
        std::function<void(const OpenSim::AbstractOutput&, std::optional<ComponentOutputSubfield>)> const& onUserSelection
    );
    void DrawSimulationParams(
        const ParamBlock&
    );
    void DrawSearchBar(std::string&);
    void DrawOutputNameColumn(
        const IOutputExtractor& output,
        bool centered = true,
        SimulationModelStatePair* maybeActiveSate = nullptr
    );

    // draws a "With Respect to" menu that prompts the user to hover a frame
    // within the given component hierarchy (from `root`)
    //
    // calls `onFrameMenuOpened` when the user is hovering a frame's menu
    // (i.e. `ui::begin_menu($FRAME)` returned `true`)
    void DrawWithRespectToMenuContainingMenuPerFrame(
        const OpenSim::Component& root,
        std::function<void(const OpenSim::Frame&)> const& onFrameMenuOpened,
        OpenSim::Frame const* maybeParent
    );

    // draws a "With Respect to" menu that prompts the user to click a frame
    // within the given component hierarchy (from `root`)
    //
    // calls `onFrameMenuItemClicked` when the user clicks the `ui::draw_menu_item`
    // associated with a frame
    void DrawWithRespectToMenuContainingMenuItemPerFrame(
        const OpenSim::Component& root,
        std::function<void(const OpenSim::Frame&)> const& onFrameMenuItemClicked,
        OpenSim::Frame const* maybeParent
    );

    void DrawPointTranslationInformationWithRespectTo(
        const OpenSim::Frame&,
        const SimTK::State&,
        Vec3 locationInGround
    );
    void DrawDirectionInformationWithRepsectTo(
        const OpenSim::Frame&,
        const SimTK::State&,
        Vec3 directionInGround
    );
    void DrawFrameInformationExpressedIn(
        const OpenSim::Frame& parent,
        const SimTK::State&,
        const OpenSim::Frame&
    );

    // calculate menus
    enum class CalculateMenuFlags {
        None             = 0,
        NoCalculatorIcon = 1<<0,
    };
    constexpr bool operator&(CalculateMenuFlags lhs, CalculateMenuFlags rhs)
    {
        return (cpp23::to_underlying(lhs) & cpp23::to_underlying(rhs)) != 0;
    }
    bool BeginCalculateMenu(
        CalculateMenuFlags = CalculateMenuFlags::None
    );
    void EndCalculateMenu(
    );
    void DrawCalculatePositionMenu(
        const OpenSim::Component& root,
        const SimTK::State&,
        const OpenSim::Point&,
        OpenSim::Frame const* maybeParent
    );
    void DrawCalculateMenu(
        const OpenSim::Component& root,
        const SimTK::State&,
        const OpenSim::Station&,
        CalculateMenuFlags = CalculateMenuFlags::None
    );
    void DrawCalculateMenu(
        const OpenSim::Component& root,
        const SimTK::State&,
        const OpenSim::Point&,
        CalculateMenuFlags = CalculateMenuFlags::None
    );
    void DrawCalculateTransformMenu(
        const OpenSim::Component& root,
        const SimTK::State&,
        const OpenSim::Frame&
    );
    void DrawCalculateOriginMenu(
        const OpenSim::Component& root,
        const SimTK::State&,
        const OpenSim::Frame&
    );
    void DrawCalculateAxisDirectionsMenu(
        const OpenSim::Component& root,
        const SimTK::State&,
        const OpenSim::Frame&
    );
    void DrawCalculateOriginMenu(
        const OpenSim::Component& root,
        const SimTK::State&,
        const OpenSim::Sphere&
    );
    void DrawCalculateRadiusMenu(
        const OpenSim::Component& root,
        const SimTK::State&,
        const OpenSim::Sphere&
    );
    void DrawCalculateVolumeMenu(
        const OpenSim::Component& root,
        const SimTK::State&,
        const OpenSim::Sphere&
    );
    void DrawCalculateMenu(
        const OpenSim::Component& root,
        const SimTK::State&,
        const OpenSim::Frame&,
        CalculateMenuFlags = CalculateMenuFlags::None
    );
    void DrawCalculateMenu(
        const OpenSim::Component& root,
        const SimTK::State&,
        const OpenSim::Geometry&,
        CalculateMenuFlags = CalculateMenuFlags::None
    );
    void TryDrawCalculateMenu(
        const OpenSim::Component& root,
        const SimTK::State&,
        const OpenSim::Component& selected,
        CalculateMenuFlags = CalculateMenuFlags::None
    );
    void DrawCalculateOriginMenu(
        const OpenSim::Component& root,
        const SimTK::State&,
        const OpenSim::Ellipsoid&
    );
    void DrawCalculateRadiiMenu(
        const OpenSim::Component& root,
        const SimTK::State&,
        const OpenSim::Ellipsoid&
    );
    void DrawCalculateRadiiDirectionsMenu(
        const OpenSim::Component& root,
        const SimTK::State&,
        const OpenSim::Ellipsoid&
    );
    void DrawCalculateScaledRadiiDirectionsMenu(
        const OpenSim::Component& root,
        const SimTK::State&,
        const OpenSim::Ellipsoid&
    );
    void DrawCalculateMenu(
        const OpenSim::Component& root,
        const SimTK::State&,
        const OpenSim::Ellipsoid&,
        CalculateMenuFlags = CalculateMenuFlags::None
    );

    // basic wigetized parts of the 3D viewer
    bool DrawMuscleRenderingOptionsRadioButtions(OpenSimDecorationOptions&);
    bool DrawMuscleSizingOptionsRadioButtons(OpenSimDecorationOptions&);
    bool DrawMuscleColoringOptionsRadioButtons(OpenSimDecorationOptions&);
    bool DrawMuscleDecorationOptionsEditor(OpenSimDecorationOptions&);
    bool DrawRenderingOptionsEditor(CustomRenderingOptions&);
    bool DrawOverlayOptionsEditor(OverlayDecorationOptions&);
    bool DrawCustomDecorationOptionCheckboxes(OpenSimDecorationOptions&);
    bool DrawAdvancedParamsEditor(ModelRendererParams&, std::span<SceneDecoration const>);
    bool DrawVisualAidsContextMenuContent(ModelRendererParams&);
    bool DrawViewerTopButtonRow(
        ModelRendererParams&,
        std::span<SceneDecoration const>,
        IconCache&,
        std::function<bool()> const& drawExtraElements = []() { return false; }
    );
    bool DrawCameraControlButtons(
        ModelRendererParams&,
        std::span<SceneDecoration const>,
        const Rect&,
        std::optional<AABB> const& maybeSceneAABB,
        IconCache&,
        Vec2 desiredTopCentroid
    );
    bool DrawViewerImGuiOverlays(
        ModelRendererParams&,
        std::span<SceneDecoration const>,
        std::optional<AABB>,
        const Rect&,
        IconCache&,
        std::function<bool()> const& drawExtraElementsInTop = []() { return false; }
    );

    // toolbar stuff
    bool BeginToolbar(CStringView label, std::optional<Vec2> padding = {});  // behaves the same as `ui::begin_panel` (i.e. you must call `ui::end_panel`)
    void DrawNewModelButton(ParentPtr<IMainUIStateAPI> const&);
    void DrawOpenModelButtonWithRecentFilesDropdown(
        std::function<void(std::optional<std::filesystem::path>)> const& onUserClickedOpenOrSelectedFile
    );
    void DrawOpenModelButtonWithRecentFilesDropdown(ParentPtr<IMainUIStateAPI> const&);
    void DrawSaveModelButton(ParentPtr<IMainUIStateAPI> const&, UndoableModelStatePair&);
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

    // mesh stuff
    void DrawMeshExportContextMenuContent(
        const UndoableModelStatePair&,
        const OpenSim::Mesh&
    );
}
