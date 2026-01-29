#pragma once

#include <libopynsim/documents/output_extractors/shared_output_extractor.h>
#include <liboscar/maths/aabb.h>
#include <liboscar/maths/vector2.h>
#include <liboscar/maths/vector3.h>
#include <liboscar/utilities/c_string_view.h>

#include <filesystem>
#include <functional>
#include <optional>
#include <span>
#include <string>
#include <utility>

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
namespace opyn { class CustomRenderingOptions; }
namespace opyn { class ModelStatePair; }
namespace opyn { class OpenSimDecorationOptions; }
namespace opyn { class OutputExtractor; }
namespace opyn { class OverlayDecorationOptions; }
namespace opyn { struct ModelRendererParams; }
namespace osc { class IconCache; }
namespace osc { class ParamBlock; }
namespace osc { class Rect; }
namespace osc { class SimulationModelStatePair; }
namespace osc { class UndoableModelStatePair; }
namespace osc { class Widget; }
namespace osc { struct PolarPerspectiveCamera; }
namespace osc { struct SceneDecoration; }

namespace osc
{
    // returns the recommended icon codepoint for the given component
    CStringView IconFor(const OpenSim::Component&);

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
        opyn::ModelStatePair&,
        const OpenSim::Component&
    );
    bool DrawRequestOutputMenuOrMenuItem(
        const OpenSim::AbstractOutput& o,
        const std::function<void(opyn::SharedOutputExtractor)>& onUserSelection
    );
    bool DrawWatchOutputMenu(
        const OpenSim::Component&,
        const std::function<void(opyn::SharedOutputExtractor)>& onUserSelection
    );
    void DrawSimulationParams(
        const ParamBlock&
    );
    void DrawSearchBar(std::string&);
    void DrawOutputNameColumn(
        const opyn::OutputExtractor& output,
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
        Vector3 locationInGround
    );
    void DrawDirectionInformationWithRepsectTo(
        const OpenSim::Frame&,
        const SimTK::State&,
        Vector3 directionInGround
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
        return (std::to_underlying(lhs) & std::to_underlying(rhs)) != 0;
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
    bool DrawMuscleRenderingOptionsRadioButtions(opyn::OpenSimDecorationOptions&);
    bool DrawMuscleSizingOptionsRadioButtons(opyn::OpenSimDecorationOptions&);
    bool DrawMuscleColorSourceOptionsRadioButtons(opyn::OpenSimDecorationOptions&);
    bool DrawMuscleColorScalingOptionsRadioButtons(opyn::OpenSimDecorationOptions&);
    bool DrawMuscleDecorationOptionsEditor(opyn::OpenSimDecorationOptions&);
    bool DrawRenderingOptionsEditor(opyn::CustomRenderingOptions&);
    bool DrawOverlayOptionsEditor(opyn::OverlayDecorationOptions&);
    bool DrawCustomDecorationOptionCheckboxes(opyn::OpenSimDecorationOptions&);
    bool DrawAdvancedParamsEditor(opyn::ModelRendererParams&, std::span<const SceneDecoration>);
    bool DrawVisualAidsContextMenuContent(opyn::ModelRendererParams&);
    bool DrawViewerTopButtonRow(
        opyn::ModelRendererParams&,
        std::span<const SceneDecoration>,
        IconCache&,
        const std::function<bool()>& drawExtraElements = []{ return false; }
    );
    bool DrawCameraControlButtons(
        opyn::ModelRendererParams&,
        std::span<const SceneDecoration>,
        const Rect&,
        const std::optional<AABB>& maybeSceneAABB,
        IconCache&,
        Vector2 desiredTopCentroid
    );
    bool DrawViewerImGuiOverlays(
        opyn::ModelRendererParams&,
        std::span<const SceneDecoration>,
        std::optional<AABB>,
        const Rect&,
        IconCache&,
        const std::function<bool()>& drawExtraElementsInTop = []{ return false; }
    );

    // toolbar stuff
    bool BeginToolbar(CStringView label, std::optional<Vector2> padding = {});  // behaves the same as `ui::begin_panel` (i.e. you must call `ui::end_panel`)
    void DrawNewModelButton(Widget&);
    void DrawOpenModelButtonWithRecentFilesDropdown(
        const std::function<void(std::optional<std::filesystem::path>)>& onUserClickedOpenOrSelectedFile
    );
    void DrawOpenModelButtonWithRecentFilesDropdown(Widget&);
    void DrawSaveModelButton(const std::shared_ptr<opyn::ModelStatePair>&);
    void DrawReloadModelButton(UndoableModelStatePair&);
    void DrawUndoButton(opyn::ModelStatePair&);
    void DrawRedoButton(opyn::ModelStatePair&);
    void DrawUndoAndRedoButtons(opyn::ModelStatePair&);
    void DrawToggleFramesButton(opyn::ModelStatePair&, IconCache&);
    void DrawToggleMarkersButton(opyn::ModelStatePair&, IconCache&);
    void DrawToggleWrapGeometryButton(opyn::ModelStatePair&, IconCache&);
    void DrawToggleContactGeometryButton(opyn::ModelStatePair&, IconCache&);
    void DrawToggleForcesButton(opyn::ModelStatePair&, IconCache&);
    void DrawAllDecorationToggleButtons(opyn::ModelStatePair&, IconCache&);
    void DrawSceneScaleFactorEditorControls(opyn::ModelStatePair&);

    // mesh stuff
    void DrawMeshExportContextMenuContent(const opyn::ModelStatePair&, const OpenSim::Mesh&);
}
