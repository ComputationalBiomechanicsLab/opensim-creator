#pragma once

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
    void DrawComponentHoverTooltip(OpenSim::Component const&);
    void DrawNothingRightClickedContextMenuHeader();
    void DrawContextMenuHeader(CStringView title, CStringView subtitle);
    void DrawRightClickedComponentContextMenuHeader(OpenSim::Component const&);
    void DrawContextMenuSeparator();
    void DrawSelectOwnerMenu(IModelStatePair&, OpenSim::Component const&);
    bool DrawWatchOutputMenu(IMainUIStateAPI&, OpenSim::Component const&);
    void DrawSimulationParams(ParamBlock const&);
    void DrawSearchBar(std::string&);
    void DrawOutputNameColumn(
        IOutputExtractor const& output,
        bool centered = true,
        SimulationModelStatePair* maybeActiveSate = nullptr
    );

    // draws a "With Respect to" menu that prompts the user to hover a frame
    // within the given component hierarchy (from `root`)
    //
    // calls `onFrameMenuOpened` when the user is hovering a frame's menu
    // (i.e. `ui::BeginMenu($FRAME)` returned `true`)
    void DrawWithRespectToMenuContainingMenuPerFrame(
        OpenSim::Component const& root,
        std::function<void(OpenSim::Frame const&)> const& onFrameMenuOpened,
        OpenSim::Frame const* maybeParent
    );

    // draws a "With Respect to" menu that prompts the user to click a frame
    // within the given component hierarchy (from `root`)
    //
    // calls `onFrameMenuItemClicked` when the user clicks the `ui::MenuItem`
    // associated with a frame
    void DrawWithRespectToMenuContainingMenuItemPerFrame(
        OpenSim::Component const& root,
        std::function<void(OpenSim::Frame const&)> const& onFrameMenuItemClicked,
        OpenSim::Frame const* maybeParent
    );

    void DrawPointTranslationInformationWithRespectTo(
        OpenSim::Frame const&,
        SimTK::State const&,
        Vec3 locationInGround
    );
    void DrawDirectionInformationWithRepsectTo(
        OpenSim::Frame const&,
        SimTK::State const&,
        Vec3 directionInGround
    );
    void DrawFrameInformationExpressedIn(
        OpenSim::Frame const& parent,
        SimTK::State const&,
        OpenSim::Frame const&
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
        OpenSim::Component const& root,
        SimTK::State const&,
        OpenSim::Point const&,
        OpenSim::Frame const* maybeParent
    );
    void DrawCalculateMenu(
        OpenSim::Component const& root,
        SimTK::State const&,
        OpenSim::Station const&,
        CalculateMenuFlags = CalculateMenuFlags::None
    );
    void DrawCalculateMenu(
        OpenSim::Component const& root,
        SimTK::State const&,
        OpenSim::Point const&,
        CalculateMenuFlags = CalculateMenuFlags::None
    );
    void DrawCalculateTransformMenu(
        OpenSim::Component const& root,
        SimTK::State const&,
        OpenSim::Frame const&
    );
    void DrawCalculateOriginMenu(
        OpenSim::Component const& root,
        SimTK::State const&,
        OpenSim::Frame const&
    );
    void DrawCalculateAxisDirectionsMenu(
        OpenSim::Component const& root,
        SimTK::State const&,
        OpenSim::Frame const&
    );
    void DrawCalculateOriginMenu(
        OpenSim::Component const& root,
        SimTK::State const&,
        OpenSim::Sphere const&
    );
    void DrawCalculateRadiusMenu(
        OpenSim::Component const& root,
        SimTK::State const&,
        OpenSim::Sphere const&
    );
    void DrawCalculateVolumeMenu(
        OpenSim::Component const& root,
        SimTK::State const&,
        OpenSim::Sphere const&
    );
    void DrawCalculateMenu(
        OpenSim::Component const& root,
        SimTK::State const&,
        OpenSim::Frame const&,
        CalculateMenuFlags = CalculateMenuFlags::None
    );
    void DrawCalculateMenu(
        OpenSim::Component const& root,
        SimTK::State const&,
        OpenSim::Geometry const&,
        CalculateMenuFlags = CalculateMenuFlags::None
    );
    void TryDrawCalculateMenu(
        OpenSim::Component const& root,
        SimTK::State const&,
        OpenSim::Component const& selected,
        CalculateMenuFlags = CalculateMenuFlags::None
    );
    void DrawCalculateOriginMenu(
        OpenSim::Component const& root,
        SimTK::State const&,
        OpenSim::Ellipsoid const&
    );
    void DrawCalculateRadiiMenu(
        OpenSim::Component const& root,
        SimTK::State const&,
        OpenSim::Ellipsoid const&
    );
    void DrawCalculateRadiiDirectionsMenu(
        OpenSim::Component const& root,
        SimTK::State const&,
        OpenSim::Ellipsoid const&
    );
    void DrawCalculateScaledRadiiDirectionsMenu(
        OpenSim::Component const& root,
        SimTK::State const&,
        OpenSim::Ellipsoid const&
    );
    void DrawCalculateMenu(
        OpenSim::Component const& root,
        SimTK::State const&,
        OpenSim::Ellipsoid const&,
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
        Rect const&,
        std::optional<AABB> const& maybeSceneAABB,
        IconCache&,
        Vec2 desiredTopCentroid
    );
    bool DrawViewerImGuiOverlays(
        ModelRendererParams&,
        std::span<SceneDecoration const>,
        std::optional<AABB>,
        Rect const&,
        IconCache&,
        std::function<bool()> const& drawExtraElementsInTop = []() { return false; }
    );

    // toolbar stuff
    bool BeginToolbar(CStringView label, std::optional<Vec2> padding = {});  // behaves the same as ui::Begin (i.e. you must call ui::End)
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
        UndoableModelStatePair const&,
        OpenSim::Mesh const&
    );
}
