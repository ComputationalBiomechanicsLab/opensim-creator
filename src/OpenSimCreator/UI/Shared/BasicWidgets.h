#pragma once

#include <OpenSimCreator/Documents/OutputExtractors/ComponentOutputSubfield.h>
#include <OpenSimCreator/UI/MainUIScreen.h>

#include <oscar/Maths/AABB.h>
#include <oscar/Maths/Vec2.h>
#include <oscar/Maths/Vec3.h>
#include <oscar/Shims/Cpp23/utility.h>
#include <oscar/Utils/CStringView.h>
#include <oscar/Utils/ParentPtr.h>

#include <filesystem>
#include <functional>
#include <optional>
#include <span>
#include <string>

namespace OpenSim { class AbstractOutput; }
namespace OpenSim { class Component; }
namespace OpenSim { class Ellipsoid; }
namespace OpenSim { class Frame; }
namespace OpenSim { class Geometry; }
namespace OpenSim { class Mesh; }
namespace OpenSim { class Point; }
namespace OpenSim { class Sphere; }
namespace OpenSim { class Station; }
namespace osc { class CustomRenderingOptions; }
namespace osc { class IconCache; }
namespace osc { class IModelStatePair; }
namespace osc { class IOutputExtractor; }
namespace osc { class OpenSimDecorationOptions; }
namespace osc { class OverlayDecorationOptions; }
namespace osc { class ParamBlock; }
namespace osc { class SimulationModelStatePair; }
namespace osc { class UndoableModelStatePair; }
namespace osc { struct ModelRendererParams; }
namespace osc { struct PolarPerspectiveCamera; }
namespace osc { struct Rect; }
namespace osc { struct SceneDecoration; }
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
        const std::function<void(const OpenSim::AbstractOutput&, std::optional<ComponentOutputSubfield>)>& onUserSelection
    );
    bool DrawWatchOutputMenu(
        const OpenSim::Component&,
        const std::function<void(const OpenSim::AbstractOutput&, std::optional<ComponentOutputSubfield>)>& onUserSelection
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
        const std::function<void(const OpenSim::Frame&)>& onFrameMenuOpened,
        const OpenSim::Frame* maybeParent
    );

    // draws a "With Respect to" menu that prompts the user to click a frame
    // within the given component hierarchy (from `root`)
    //
    // calls `onFrameMenuItemClicked` when the user clicks the `ui::draw_menu_item`
    // associated with a frame
    void DrawWithRespectToMenuContainingMenuItemPerFrame(
        const OpenSim::Component& root,
        const std::function<void(const OpenSim::Frame&)>& onFrameMenuItemClicked,
        const OpenSim::Frame* maybeParent
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
        const OpenSim::Frame* maybeParent
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
    bool DrawAdvancedParamsEditor(ModelRendererParams&, std::span<const SceneDecoration>);
    bool DrawVisualAidsContextMenuContent(ModelRendererParams&);
    bool DrawViewerTopButtonRow(
        ModelRendererParams&,
        std::span<const SceneDecoration>,
        IconCache&,
        const std::function<bool()>& drawExtraElements = []() { return false; }
    );
    bool DrawCameraControlButtons(
        ModelRendererParams&,
        std::span<const SceneDecoration>,
        const Rect&,
        const std::optional<AABB>& maybeSceneAABB,
        IconCache&,
        Vec2 desiredTopCentroid
    );
    bool DrawViewerImGuiOverlays(
        ModelRendererParams&,
        std::span<const SceneDecoration>,
        std::optional<AABB>,
        const Rect&,
        IconCache&,
        const std::function<bool()>& drawExtraElementsInTop = []() { return false; }
    );

    // toolbar stuff
    bool BeginToolbar(CStringView label, std::optional<Vec2> padding = {});  // behaves the same as `ui::begin_panel` (i.e. you must call `ui::end_panel`)
    void DrawNewModelButton(const ParentPtr<MainUIScreen>&);
    void DrawOpenModelButtonWithRecentFilesDropdown(
        const std::function<void(std::optional<std::filesystem::path>)>& onUserClickedOpenOrSelectedFile
    );
    void DrawOpenModelButtonWithRecentFilesDropdown(const ParentPtr<MainUIScreen>&);
    void DrawSaveModelButton(const ParentPtr<MainUIScreen>&, UndoableModelStatePair&);
    void DrawReloadModelButton(UndoableModelStatePair&);
    void DrawUndoButton(IModelStatePair&);
    void DrawRedoButton(IModelStatePair&);
    void DrawUndoAndRedoButtons(IModelStatePair&);
    void DrawToggleFramesButton(IModelStatePair&, IconCache&);
    void DrawToggleMarkersButton(IModelStatePair&, IconCache&);
    void DrawToggleWrapGeometryButton(IModelStatePair&, IconCache&);
    void DrawToggleContactGeometryButton(IModelStatePair&, IconCache&);
    void DrawToggleForcesButton(IModelStatePair&, IconCache&);
    void DrawAllDecorationToggleButtons(IModelStatePair&, IconCache&);
    void DrawSceneScaleFactorEditorControls(IModelStatePair&);

    // mesh stuff
    void DrawMeshExportContextMenuContent(const IModelStatePair&, const OpenSim::Mesh&);
}
