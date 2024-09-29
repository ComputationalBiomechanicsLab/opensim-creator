#include "FrameDefinitionTab.h"

#include <OpenSimCreator/Documents/CustomComponents/CrossProductEdge.h>
#include <OpenSimCreator/Documents/CustomComponents/Edge.h>
#include <OpenSimCreator/Documents/CustomComponents/MidpointLandmark.h>
#include <OpenSimCreator/Documents/CustomComponents/PointToPointEdge.h>
#include <OpenSimCreator/Documents/FrameDefinition/FrameDefinitionActions.h>
#include <OpenSimCreator/Documents/FrameDefinition/FrameDefinitionHelpers.h>
#include <OpenSimCreator/Documents/Model/IModelStatePair.h>
#include <OpenSimCreator/Documents/Model/UndoableModelActions.h>
#include <OpenSimCreator/Documents/Model/UndoableModelStatePair.h>
#include <OpenSimCreator/UI/FrameDefinition/FrameDefinitionTabToolbar.h>
#include <OpenSimCreator/UI/FrameDefinition/FrameDefinitionUIHelpers.h>
#include <OpenSimCreator/UI/ModelEditor/IEditorAPI.h>
#include <OpenSimCreator/UI/Shared/BasicWidgets.h>
#include <OpenSimCreator/UI/Shared/ChooseComponentsEditorLayer.h>
#include <OpenSimCreator/UI/Shared/ChooseComponentsEditorLayerParameters.h>
#include <OpenSimCreator/UI/Shared/MainMenu.h>
#include <OpenSimCreator/UI/Shared/ModelEditorViewerPanel.h>
#include <OpenSimCreator/UI/Shared/ModelEditorViewerPanelParameters.h>
#include <OpenSimCreator/UI/Shared/ModelEditorViewerPanelRightClickEvent.h>
#include <OpenSimCreator/UI/Shared/NavigatorPanel.h>
#include <OpenSimCreator/UI/Shared/PropertiesPanel.h>
#include <OpenSimCreator/UI/MainUIScreen.h>
#include <OpenSimCreator/Utils/OpenSimHelpers.h>

#include <OpenSim/Common/Component.h>
#include <OpenSim/Common/ComponentPath.h>
#include <OpenSim/Common/ComponentSocket.h>
#include <OpenSim/Simulation/Model/Ground.h>
#include <OpenSim/Simulation/Model/Model.h>
#include <OpenSim/Simulation/Model/PhysicalOffsetFrame.h>
#include <OpenSim/Simulation/SimbodyEngine/FreeJoint.h>
#include <oscar/Formats/OBJ.h>
#include <oscar/Graphics/Color.h>
#include <oscar/Maths/CoordinateDirection.h>
#include <oscar/Maths/MathHelpers.h>
#include <oscar/Maths/Vec2.h>
#include <oscar/Maths/Vec3.h>
#include <oscar/Platform/App.h>
#include <oscar/Platform/Event.h>
#include <oscar/Platform/IconCodepoints.h>
#include <oscar/Platform/Log.h>
#include <oscar/UI/oscimgui.h>
#include <oscar/UI/Panels/LogViewerPanel.h>
#include <oscar/UI/Panels/PanelManager.h>
#include <oscar/UI/Panels/PerfPanel.h>
#include <oscar/UI/Tabs/TabPrivate.h>
#include <oscar/UI/Widgets/IPopup.h>
#include <oscar/UI/Widgets/PopupManager.h>
#include <oscar/UI/Widgets/StandardPopup.h>
#include <oscar/UI/Widgets/WindowMenu.h>
#include <oscar/Utils/Assertions.h>
#include <oscar/Utils/CStringView.h>
#include <oscar/Utils/UID.h>
#include <oscar_simbody/SimTKHelpers.h>

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_set>
#include <utility>

using namespace osc::fd;
using namespace osc;

// choose `n` components UI flow
namespace
{
    /////
    // layer pushing routines
    /////

    void PushCreateEdgeToOtherPointLayer(
        IEditorAPI& editor,
        const std::shared_ptr<IModelStatePair>& model,
        const OpenSim::Point& point,
        const ModelEditorViewerPanelRightClickEvent& sourceEvent)
    {
        auto* const visualizer = editor.getPanelManager()->try_upd_panel_by_name_T<ModelEditorViewerPanel>(sourceEvent.sourcePanelName);
        if (not visualizer) {
            return;  // can't figure out which visualizer to push the layer to
        }

        ChooseComponentsEditorLayerParameters options;
        options.popupHeaderText = "choose other point";
        options.canChooseItem = IsPoint;
        options.componentsBeingAssignedTo = {GetAbsolutePathStringName(point)};
        options.numComponentsUserMustChoose = 1;
        options.onUserFinishedChoosing = [model, pointAPath = point.getAbsolutePathString()](const auto& choices) -> bool
        {
            if (choices.empty()) {
                log_error("user selections from the 'choose components' layer was empty: this bug should be reported");
                return false;
            }
            if (choices.size() > 1) {
                log_warn("number of user selections from 'choose components' layer was greater than expected: this bug should be reported");
            }
            const auto& pointBPath = *choices.begin();

            const auto* pointA = FindComponent<OpenSim::Point>(model->getModel(), pointAPath);
            if (not pointA) {
                log_error("point A's component path (%s) does not exist in the model", pointAPath.c_str());
                return false;
            }

            const auto* pointB = FindComponent<OpenSim::Point>(model->getModel(), pointBPath);
            if (not pointB) {
                log_error("point B's component path (%s) does not exist in the model", pointBPath.c_str());
                return false;
            }

            ActionAddPointToPointEdge(*model, *pointA, *pointB);
            return true;
        };

        visualizer->pushLayer(std::make_unique<ChooseComponentsEditorLayer>(model, options));
    }

    void PushCreateMidpointToAnotherPointLayer(
        IEditorAPI& editor,
        const std::shared_ptr<IModelStatePair>& model,
        const OpenSim::Point& point,
        const ModelEditorViewerPanelRightClickEvent& sourceEvent)
    {
        auto* const visualizer = editor.getPanelManager()->try_upd_panel_by_name_T<ModelEditorViewerPanel>(sourceEvent.sourcePanelName);
        if (not visualizer) {
            return;  // can't figure out which visualizer to push the layer to
        }

        ChooseComponentsEditorLayerParameters options;
        options.popupHeaderText = "choose other point";
        options.canChooseItem = IsPoint;
        options.componentsBeingAssignedTo = {GetAbsolutePathStringName(point)};
        options.numComponentsUserMustChoose = 1;
        options.onUserFinishedChoosing = [model, pointAPath = point.getAbsolutePathString()](const auto& choices) -> bool
        {
            if (choices.empty()) {
                log_error("user selections from the 'choose components' layer was empty: this bug should be reported");
                return false;
            }
            if (choices.size() > 1) {
                log_warn("number of user selections from 'choose components' layer was greater than expected: this bug should be reported");
            }
            const auto& pointBPath = *choices.begin();

            const auto* pointA = FindComponent<OpenSim::Point>(model->getModel(), pointAPath);
            if (not pointA) {
                log_error("point A's component path (%s) does not exist in the model", pointAPath.c_str());
                return false;
            }

            const auto* pointB = FindComponent<OpenSim::Point>(model->getModel(), pointBPath);
            if (not pointB) {
                log_error("point B's component path (%s) does not exist in the model", pointBPath.c_str());
                return false;
            }

            ActionAddMidpoint(*model, *pointA, *pointB);
            return true;
        };

        visualizer->pushLayer(std::make_unique<ChooseComponentsEditorLayer>(model, options));
    }

    void PushCreateCrossProductEdgeLayer(
        IEditorAPI& editor,
        const std::shared_ptr<IModelStatePair>& model,
        const Edge& firstEdge,
        const ModelEditorViewerPanelRightClickEvent& sourceEvent)
    {
        auto* const visualizer = editor.getPanelManager()->try_upd_panel_by_name_T<ModelEditorViewerPanel>(sourceEvent.sourcePanelName);
        if (not visualizer) {
            return;  // can't figure out which visualizer to push the layer to
        }

        ChooseComponentsEditorLayerParameters options;
        options.popupHeaderText = "choose other edge";
        options.canChooseItem = IsEdge;
        options.componentsBeingAssignedTo = {GetAbsolutePathStringName(firstEdge)};
        options.numComponentsUserMustChoose = 1;
        options.onUserFinishedChoosing = [model, edgeAPath = GetAbsolutePathStringName(firstEdge)](const auto& choices) -> bool
        {
            if (choices.empty()) {
                log_error("user selections from the 'choose components' layer was empty: this bug should be reported");
                return false;
            }
            if (choices.size() > 1) {
                log_warn("number of user selections from 'choose components' layer was greater than expected: this bug should be reported");
            }
            const auto& edgeBPath = *choices.begin();

            const auto* edgeA = FindComponent<Edge>(model->getModel(), edgeAPath);
            if (not edgeA) {
                log_error("edge A's component path (%s) does not exist in the model", edgeAPath.c_str());
                return false;
            }

            const auto* edgeB = FindComponent<Edge>(model->getModel(), edgeBPath);
            if (not edgeB) {
                log_error("point B's component path (%s) does not exist in the model", edgeBPath.c_str());
                return false;
            }

            ActionAddCrossProductEdge(*model, *edgeA, *edgeB);
            return true;
        };

        visualizer->pushLayer(std::make_unique<ChooseComponentsEditorLayer>(model, options));
    }

    void PushPickOriginForFrameDefinitionLayer(
        ModelEditorViewerPanel& visualizer,
        const std::shared_ptr<IModelStatePair>& model,
        const StringName& firstEdgeAbsPath,
        CoordinateDirection firstEdgeAxis,
        const StringName& secondEdgeAbsPath)
    {
        ChooseComponentsEditorLayerParameters options;
        options.popupHeaderText = "choose frame origin";
        options.canChooseItem = IsPoint;
        options.numComponentsUserMustChoose = 1;
        options.onUserFinishedChoosing = [
            model,
            firstEdgeAbsPath,
            firstEdgeAxis,
            secondEdgeAbsPath
        ](const auto& choices) -> bool
        {
            if (choices.empty()) {
                log_error("user selections from the 'choose components' layer was empty: this bug should be reported");
                return false;
            }
            if (choices.size() > 1) {
                log_warn("number of user selections from 'choose components' layer was greater than expected: this bug should be reported");
            }
            const auto& originPath = *choices.begin();

            const auto* firstEdge = FindComponent<Edge>(model->getModel(), firstEdgeAbsPath);
            if (not firstEdge) {
                log_error("the first edge's component path (%s) does not exist in the model", firstEdgeAbsPath.c_str());
                return false;
            }

            const auto* otherEdge = FindComponent<Edge>(model->getModel(), secondEdgeAbsPath);
            if (not otherEdge) {
                log_error("the second edge's component path (%s) does not exist in the model", secondEdgeAbsPath.c_str());
                return false;
            }

            const auto* originPoint = FindComponent<OpenSim::Point>(model->getModel(), originPath);
            if (not originPoint) {
                log_error("the origin's component path (%s) does not exist in the model", originPath.c_str());
                return false;
            }

            ActionAddFrame(
                model,
                *firstEdge,
                firstEdgeAxis,
                *otherEdge,
                *originPoint
            );
            return true;
        };

        visualizer.pushLayer(std::make_unique<ChooseComponentsEditorLayer>(model, options));
    }

    void PushPickOtherEdgeStateForFrameDefinitionLayer(
        ModelEditorViewerPanel& visualizer,
        const std::shared_ptr<IModelStatePair>& model,
        const Edge& firstEdge,
        CoordinateDirection firstEdgeAxis)
    {
        ChooseComponentsEditorLayerParameters options;
        options.popupHeaderText = "choose other edge";
        options.canChooseItem = IsEdge;
        options.componentsBeingAssignedTo = {GetAbsolutePathStringName(firstEdge)};
        options.numComponentsUserMustChoose = 1;
        options.onUserFinishedChoosing = [
            visualizerPtr = &visualizer,  // TODO: implement weak_ptr for panel lookup
            model,
            firstEdgeAbsPath = GetAbsolutePathStringName(firstEdge),
            firstEdgeAxis
        ](const auto& choices) -> bool
        {
            // go into "pick origin" state

            if (choices.empty()) {
                log_error("user selections from the 'choose components' layer was empty: this bug should be reported");
                return false;
            }
            const auto& otherEdgePath = *choices.begin();

            PushPickOriginForFrameDefinitionLayer(
                *visualizerPtr,  // TODO: unsafe if not guarded by weak_ptr or similar
                model,
                firstEdgeAbsPath,
                firstEdgeAxis,
                otherEdgePath
            );
            return true;
        };

        visualizer.pushLayer(std::make_unique<ChooseComponentsEditorLayer>(model, options));
    }
}

namespace
{
    void ActionPushCreateFrameLayer(
        IEditorAPI& editor,
        const std::shared_ptr<IModelStatePair>& model,
        const Edge& firstEdge,
        CoordinateDirection firstEdgeAxis,
        const std::optional<ModelEditorViewerPanelRightClickEvent>& maybeSourceEvent)
    {
        if (model->isReadonly()) {
            return;
        }

        if (not maybeSourceEvent) {
            return;  // there is no way to figure out which visualizer to push the layer to
        }

        auto* const visualizer = editor.getPanelManager()->try_upd_panel_by_name_T<ModelEditorViewerPanel>(maybeSourceEvent->sourcePanelName);
        if (not visualizer) {
            return;  // the visualizer that the user clicked cannot be found
        }

        PushPickOtherEdgeStateForFrameDefinitionLayer(
            *visualizer,
            model,
            firstEdge,
            firstEdgeAxis
        );
    }

    void PushPickParentFrameForBodyCreactionLayer(
        ModelEditorViewerPanel& visualizer,
        const std::shared_ptr<IModelStatePair>& model,
        const OpenSim::ComponentPath& frameAbsPath,
        const OpenSim::ComponentPath& meshAbsPath,
        const OpenSim::ComponentPath& jointFrameAbsPath)
    {
        ChooseComponentsEditorLayerParameters options;
        options.popupHeaderText = "choose parent frame";
        options.canChooseItem = [bodyFrame = FindComponent(model->getModel(), frameAbsPath)](const OpenSim::Component& c)
        {
            return
                IsPhysicalFrame(c) &&
                &c != bodyFrame &&
                !IsChildOfA<OpenSim::ComponentSet>(c) &&
                (
                    dynamic_cast<const OpenSim::Ground*>(&c) != nullptr ||
                    IsChildOfA<OpenSim::BodySet>(c)
                );
        };
        options.numComponentsUserMustChoose = 1;
        options.onUserFinishedChoosing = [
            model,
            frameAbsPath,
            meshAbsPath,
            jointFrameAbsPath
        ](const auto& choices) -> bool
        {
            if (choices.empty()) {
                log_error("user selections from the 'choose components' layer was empty: this bug should be reported");
                return false;
            }

            const auto* const parentFrame = FindComponent<OpenSim::PhysicalFrame>(model->getModel(), *choices.begin());
            if (not parentFrame) {
                log_error("user selection from 'choose components' layer did not select a frame: this shouldn't happen?");
                return false;
            }

            osc::fd::ActionCreateBodyFromFrame(
                model,
                frameAbsPath,
                meshAbsPath,
                jointFrameAbsPath,
                parentFrame->getAbsolutePath()
            );

            return true;
        };

        visualizer.pushLayer(std::make_unique<ChooseComponentsEditorLayer>(model, options));
    }

    void PushPickJointFrameForBodyCreactionLayer(
        ModelEditorViewerPanel& visualizer,
        const std::shared_ptr<IModelStatePair>& model,
        const OpenSim::ComponentPath& frameAbsPath,
        const OpenSim::ComponentPath& meshAbsPath)
    {
        ChooseComponentsEditorLayerParameters options;
        options.popupHeaderText = "choose joint center frame";
        options.canChooseItem = IsPhysicalFrame;
        options.numComponentsUserMustChoose = 1;
        options.onUserFinishedChoosing = [
            visualizerPtr = &visualizer,  // TODO: implement weak_ptr for panel lookup
            model,
            frameAbsPath,
            meshAbsPath
        ](const auto& choices) -> bool
        {
            if (choices.empty()) {
                log_error("user selections from the 'choose components' layer was empty: this bug should be reported");
                return false;
            }

            const auto* const jointFrame = FindComponent<OpenSim::Frame>(model->getModel(), *choices.begin());
            if (not jointFrame) {
                log_error("user selection from 'choose components' layer did not select a frame: this shouldn't happen?");
                return false;
            }

            PushPickParentFrameForBodyCreactionLayer(
                *visualizerPtr,
                model,
                frameAbsPath,
                meshAbsPath,
                jointFrame->getAbsolutePath()
            );

            return true;
        };

        visualizer.pushLayer(std::make_unique<ChooseComponentsEditorLayer>(model, options));
    }

    void PushPickMeshForBodyCreationLayer(
        ModelEditorViewerPanel& visualizer,
        const std::shared_ptr<IModelStatePair>& model,
        const OpenSim::Frame& frame)
    {
        ChooseComponentsEditorLayerParameters options;
        options.popupHeaderText = "choose mesh to attach the body to";
        options.canChooseItem = [](const OpenSim::Component& c) { return IsMesh(c) && !IsChildOfA<OpenSim::Body>(c); };
        options.numComponentsUserMustChoose = 1;
        options.onUserFinishedChoosing = [
            visualizerPtr = &visualizer,  // TODO: implement weak_ptr for panel lookup
            model,
            frameAbsPath = frame.getAbsolutePath()
        ](const auto& choices) -> bool
        {
            if (choices.empty()) {
                log_error("user selections from the 'choose components' layer was empty: this bug should be reported");
                return false;
            }

            const auto* const mesh = FindComponent<OpenSim::Mesh>(model->getModel(), *choices.begin());
            if (not mesh) {
                log_error("user selection from 'choose components' layer did not select a mesh: this shouldn't happen?");
                return false;
            }

            PushPickJointFrameForBodyCreactionLayer(
                *visualizerPtr,  // TODO: unsafe if not guarded by weak_ptr or similar
                model,
                frameAbsPath,
                mesh->getAbsolutePath()
            );
            return true;
        };

        visualizer.pushLayer(std::make_unique<ChooseComponentsEditorLayer>(model, options));
    }

    void ActionCreateBodyFromFrame(
        IEditorAPI& editor,
        const std::shared_ptr<IModelStatePair>& model,
        const std::optional<ModelEditorViewerPanelRightClickEvent>& maybeSourceEvent,
        const OpenSim::Frame& frame)
    {
        if (model->isReadonly()) {
            return;
        }

        if (not maybeSourceEvent) {
            return;  // there is no way to figure out which visualizer to push the layer to
        }

        auto* const visualizer = editor.getPanelManager()->try_upd_panel_by_name_T<ModelEditorViewerPanel>(maybeSourceEvent->sourcePanelName);
        if (not visualizer) {
            return;  // the visualizer that the user clicked cannot be found
        }

        PushPickMeshForBodyCreationLayer(
            *visualizer,
            model,
            frame
        );
    }
}

// context menu stuff
namespace
{
    // draws the calculate menu for an edge
    void DrawCalculateMenu(
        const OpenSim::Component& root,
        const SimTK::State& state,
        const Edge& edge)
    {
        if (ui::begin_menu(OSC_ICON_CALCULATOR " Calculate"))
        {
            if (ui::begin_menu("Start Point"))
            {
                const auto onFrameMenuOpened = [&state, &edge](const OpenSim::Frame& frame)
                {
                    DrawPointTranslationInformationWithRespectTo(
                        frame,
                        state,
                        to<Vec3>(edge.getStartLocationInGround(state))
                    );
                };
                DrawWithRespectToMenuContainingMenuPerFrame(root, onFrameMenuOpened, nullptr);
                ui::end_menu();
            }

            if (ui::begin_menu("End Point"))
            {
                const auto onFrameMenuOpened = [&state, &edge](const OpenSim::Frame& frame)
                {
                    DrawPointTranslationInformationWithRespectTo(
                        frame,
                        state,
                        to<Vec3>(edge.getEndLocationInGround(state))
                    );
                };

                DrawWithRespectToMenuContainingMenuPerFrame(root, onFrameMenuOpened, nullptr);
                ui::end_menu();
            }

            if (ui::begin_menu("Direction"))
            {
                const auto onFrameMenuOpened = [&state, &edge](const OpenSim::Frame& frame)
                {
                    DrawDirectionInformationWithRepsectTo(
                        frame,
                        state,
                        to<Vec3>(CalcDirection(edge.getLocationsInGround(state)))
                    );
                };

                DrawWithRespectToMenuContainingMenuPerFrame(root, onFrameMenuOpened, nullptr);
                ui::end_menu();
            }

            ui::end_menu();
        }
    }

    void DrawFocusCameraMenu(
        IEditorAPI& editor,
        const std::shared_ptr<IModelStatePair>&,
        const std::optional<ModelEditorViewerPanelRightClickEvent>& maybeSourceEvent,
        const OpenSim::Component&)
    {
        if (maybeSourceEvent and ui::begin_menu(OSC_ICON_CAMERA " Focus Camera")) {
            if (ui::draw_menu_item("on Ground")) {
                auto* visualizer = editor.getPanelManager()->try_upd_panel_by_name_T<ModelEditorViewerPanel>(maybeSourceEvent->sourcePanelName);
                if (visualizer) {
                    visualizer->focusOn({});
                }
            }

            if (maybeSourceEvent->maybeClickPositionInGround and
                ui::draw_menu_item("on Click Position")) {
                auto* visualizer = editor.getPanelManager()->try_upd_panel_by_name_T<ModelEditorViewerPanel>(maybeSourceEvent->sourcePanelName);
                if (visualizer) {
                    visualizer->focusOn(*maybeSourceEvent->maybeClickPositionInGround);
                }
            }

            ui::end_menu();
        }
    }

    void DrawEdgeAddContextMenuItems(
        IEditorAPI& editor,
        const std::shared_ptr<IModelStatePair>& model,
        const std::optional<ModelEditorViewerPanelRightClickEvent>& maybeSourceEvent,
        const Edge& edge)
    {
        if (maybeSourceEvent and ui::draw_menu_item(OSC_ICON_TIMES " Cross Product Edge")) {
            PushCreateCrossProductEdgeLayer(editor, model, edge, *maybeSourceEvent);
        }

        if (maybeSourceEvent and ui::begin_menu(OSC_ICON_ARROWS_ALT " Frame With This Edge as")) {
            ui::push_style_color(ui::ColorVar::Text, Color::muted_red());
            if (ui::draw_menu_item("+x", {}, nullptr, model->canUpdModel())) {
                ActionPushCreateFrameLayer(
                    editor,
                    model,
                    edge,
                    CoordinateDirection::x(),
                    maybeSourceEvent
                );
            }
            ui::pop_style_color();

            ui::push_style_color(ui::ColorVar::Text, Color::muted_green());
            if (ui::draw_menu_item("+y", {}, nullptr, model->canUpdModel())) {
                ActionPushCreateFrameLayer(
                    editor,
                    model,
                    edge,
                    CoordinateDirection::y(),
                    maybeSourceEvent
                );
            }
            ui::pop_style_color();

            ui::push_style_color(ui::ColorVar::Text, Color::muted_blue());
            if (ui::draw_menu_item("+z", {}, nullptr, model->canUpdModel())) {
                ActionPushCreateFrameLayer(
                    editor,
                    model,
                    edge,
                    CoordinateDirection::z(),
                    maybeSourceEvent
                );
            }
            ui::pop_style_color();

            ui::draw_separator();

            ui::push_style_color(ui::ColorVar::Text, Color::muted_red());
            if (ui::draw_menu_item("-x", {}, nullptr, model->canUpdModel())) {
                ActionPushCreateFrameLayer(
                    editor,
                    model,
                    edge,
                    CoordinateDirection::minus_x(),
                    maybeSourceEvent
                );
            }
            ui::pop_style_color();

            ui::push_style_color(ui::ColorVar::Text, Color::muted_green());
            if (ui::draw_menu_item("-y", {}, nullptr, model->canUpdModel())) {
                ActionPushCreateFrameLayer(
                    editor,
                    model,
                    edge,
                    CoordinateDirection::minus_y(),
                    maybeSourceEvent
                );
            }
            ui::pop_style_color();

            ui::push_style_color(ui::ColorVar::Text, Color::muted_blue());
            if (ui::draw_menu_item("-z", {}, nullptr, model->canUpdModel())) {
                ActionPushCreateFrameLayer(
                    editor,
                    model,
                    edge,
                    CoordinateDirection::minus_z(),
                    maybeSourceEvent
                );
            }
            ui::pop_style_color();

            ui::end_menu();
        }
    }

    void DrawCreateBodyMenuItem(
        IEditorAPI& editor,
        const std::shared_ptr<IModelStatePair>& model,
        const std::optional<ModelEditorViewerPanelRightClickEvent>& maybeSourceEvent,
        const OpenSim::Frame& frame)
    {
        const OpenSim::Component* groundOrExistingBody = dynamic_cast<const OpenSim::Ground*>(&frame);
        if (not groundOrExistingBody) {
            groundOrExistingBody = FindFirstDescendentOfType<OpenSim::Body>(frame);
        }

        if (ui::draw_menu_item(OSC_ICON_WEIGHT " Body From This", {}, false, not groundOrExistingBody and model->canUpdModel())) {
            ActionCreateBodyFromFrame(editor, model, maybeSourceEvent, frame);
        }
        if (groundOrExistingBody and ui::is_item_hovered(ui::HoveredFlag::AllowWhenDisabled)) {
            std::stringstream ss;
            ss << "Cannot create a body from this frame: it is already the frame of " << groundOrExistingBody->getName();
            ui::draw_tooltip_body_only(std::move(ss).str());
        }
    }
    void DrawMeshAddContextMenuItems(
        const std::shared_ptr<IModelStatePair>& model,
        const std::optional<ModelEditorViewerPanelRightClickEvent>& maybeSourceEvent,
        const OpenSim::Mesh& mesh)
    {
        if (ui::draw_menu_item(OSC_ICON_CIRCLE " Sphere Landmark", {}, nullptr, model->canUpdModel())) {
            ActionAddSphereInMeshFrame(
                *model,
                mesh,
                maybeSourceEvent ? maybeSourceEvent->maybeClickPositionInGround : std::nullopt
            );
        }
        if (ui::draw_menu_item(OSC_ICON_ARROWS_ALT " Custom (Offset) Frame", {}, nullptr, model->canUpdModel())) {
            ActionAddOffsetFrameInMeshFrame(
                *model,
                mesh,
                maybeSourceEvent ? maybeSourceEvent->maybeClickPositionInGround : std::nullopt
            );
        }
    }

    void DrawPointAddContextMenuItems(
        IEditorAPI& editor,
        const std::shared_ptr<IModelStatePair>& model,
        const std::optional<ModelEditorViewerPanelRightClickEvent>& maybeSourceEvent,
        const OpenSim::Point& point)
    {
        if (maybeSourceEvent and ui::draw_menu_item(OSC_ICON_GRIP_LINES " Edge", {}, nullptr, model->canUpdModel())) {
            PushCreateEdgeToOtherPointLayer(editor, model, point, *maybeSourceEvent);
        }
        if (maybeSourceEvent and ui::draw_menu_item(OSC_ICON_DOT_CIRCLE " Midpoint", {}, nullptr, model->canUpdModel())) {
            PushCreateMidpointToAnotherPointLayer(editor, model, point, *maybeSourceEvent);
        }
    }

    void DrawRightClickedNothingContextMenu(
        IModelStatePair& model)
    {
        DrawNothingRightClickedContextMenuHeader();
        DrawContextMenuSeparator();

        if (ui::begin_menu(OSC_ICON_PLUS " Add")) {
            if (ui::draw_menu_item(OSC_ICON_CUBES " Meshes", {}, nullptr, model.canUpdModel())) {
                ActionPromptUserToAddMeshFiles(model);
            }
            ui::end_menu();
        }
    }

    void DrawRightClickedMeshContextMenu(
        IEditorAPI& editor,
        const std::shared_ptr<IModelStatePair>& model,
        const std::optional<ModelEditorViewerPanelRightClickEvent>& maybeSourceEvent,
        const OpenSim::Mesh& mesh)
    {
        DrawRightClickedComponentContextMenuHeader(mesh);
        DrawContextMenuSeparator();

        if (ui::begin_menu(OSC_ICON_PLUS " Add")) {
            DrawMeshAddContextMenuItems(model, maybeSourceEvent, mesh);
            ui::end_menu();
        }
        if (ui::begin_menu(OSC_ICON_FILE_EXPORT " Export")) {
            DrawMeshExportContextMenuContent(*model, mesh);
            ui::end_menu();
        }
        DrawFocusCameraMenu(editor, model, maybeSourceEvent, mesh);
    }

    void DrawRightClickedPointContextMenu(
        IEditorAPI& editor,
        const std::shared_ptr<IModelStatePair>& model,
        const std::optional<ModelEditorViewerPanelRightClickEvent>& maybeSourceEvent,
        const OpenSim::Point& point)
    {
        DrawRightClickedComponentContextMenuHeader(point);
        DrawContextMenuSeparator();

        if (ui::begin_menu(OSC_ICON_PLUS " Add")) {
            DrawPointAddContextMenuItems(editor, model, maybeSourceEvent, point);
            ui::end_menu();
        }
        osc::DrawCalculateMenu(model->getModel(), model->getState(), point);
        DrawFocusCameraMenu(editor, model, maybeSourceEvent, point);
    }

    void DrawRightClickedPointToPointEdgeContextMenu(
        IEditorAPI& editor,
        const std::shared_ptr<IModelStatePair>& model,
        const std::optional<ModelEditorViewerPanelRightClickEvent>& maybeSourceEvent,
        const PointToPointEdge& edge)
    {
        DrawRightClickedComponentContextMenuHeader(edge);
        DrawContextMenuSeparator();

        if (ui::begin_menu(OSC_ICON_PLUS " Add")) {
            DrawEdgeAddContextMenuItems(editor, model, maybeSourceEvent, edge);
            ui::end_menu();
        }
        if (ui::draw_menu_item(OSC_ICON_RECYCLE " Swap Direction", {}, nullptr, model->canUpdModel())) {
            ActionSwapPointToPointEdgeEnds(*model, edge);
        }
        DrawCalculateMenu(model->getModel(), model->getState(), edge);
        DrawFocusCameraMenu(editor, model, maybeSourceEvent, edge);
    }

    void DrawRightClickedCrossProductEdgeContextMenu(
        IEditorAPI& editor,
        const std::shared_ptr<IModelStatePair>& model,
        const std::optional<ModelEditorViewerPanelRightClickEvent>& maybeSourceEvent,
        const CrossProductEdge& edge)
    {
        DrawRightClickedComponentContextMenuHeader(edge);
        DrawContextMenuSeparator();

        if (ui::begin_menu(OSC_ICON_PLUS " Add")) {
            DrawEdgeAddContextMenuItems(editor, model, maybeSourceEvent, edge);
            ui::end_menu();
        }
        if (ui::draw_menu_item(OSC_ICON_RECYCLE " Swap Operands")) {
            ActionSwapCrossProductEdgeOperands(*model, edge);
        }
        DrawCalculateMenu(model->getModel(), model->getState(), edge);
        DrawFocusCameraMenu(editor, model, maybeSourceEvent, edge);
    }

    void DrawRightClickedFrameContextMenu(
        IEditorAPI& editor,
        const std::shared_ptr<IModelStatePair>& model,
        const std::optional<ModelEditorViewerPanelRightClickEvent>& maybeSourceEvent,
        const OpenSim::Frame& frame)
    {
        DrawRightClickedComponentContextMenuHeader(frame);
        DrawContextMenuSeparator();

        if (ui::begin_menu(OSC_ICON_PLUS " Add")) {
            DrawCreateBodyMenuItem(editor, model, maybeSourceEvent, frame);
            ui::end_menu();
        }
        osc::DrawCalculateMenu(model->getModel(), model->getState(), frame);
        DrawFocusCameraMenu(editor, model, maybeSourceEvent, frame);
    }

    void DrawRightClickedUnknownComponentContextMenu(
        IEditorAPI& editor,
        const std::shared_ptr<IModelStatePair>& model,
        const std::optional<ModelEditorViewerPanelRightClickEvent>& maybeSourceEvent,
        const OpenSim::Component& component)
    {
        DrawRightClickedComponentContextMenuHeader(component);
        DrawContextMenuSeparator();

        DrawFocusCameraMenu(editor, model, maybeSourceEvent, component);
    }

    // popup state for the frame definition tab's general context menu
    class FrameDefinitionContextMenu final : public StandardPopup {
    public:
        FrameDefinitionContextMenu(
            std::string_view popupName_,
            IEditorAPI* editorAPI_,
            std::shared_ptr<IModelStatePair> model_,
            OpenSim::ComponentPath componentPath_,
            std::optional<ModelEditorViewerPanelRightClickEvent> maybeSourceVisualizerEvent_ = std::nullopt) :

            StandardPopup{popupName_, {10.0f, 10.0f}, ui::WindowFlag::NoMove},
            m_EditorAPI{editorAPI_},
            m_Model{std::move(model_)},
            m_ComponentPath{std::move(componentPath_)},
            m_MaybeSourceVisualizerEvent{std::move(maybeSourceVisualizerEvent_)}
        {
            OSC_ASSERT(m_EditorAPI != nullptr);
            OSC_ASSERT(m_Model != nullptr);

            set_modal(false);
        }

    private:
        void impl_draw_content() final
        {
            const OpenSim::Component* const maybeComponent = FindComponent(m_Model->getModel(), m_ComponentPath);
            if (not maybeComponent) {
                DrawRightClickedNothingContextMenu(*m_Model);
            }
            else if (const auto* maybeMesh = dynamic_cast<const OpenSim::Mesh*>(maybeComponent)) {
                DrawRightClickedMeshContextMenu(*m_EditorAPI, m_Model, m_MaybeSourceVisualizerEvent, *maybeMesh);
            }
            else if (const auto* maybePoint = dynamic_cast<const OpenSim::Point*>(maybeComponent)) {
                DrawRightClickedPointContextMenu(*m_EditorAPI, m_Model, m_MaybeSourceVisualizerEvent, *maybePoint);
            }
            else if (const auto* maybeFrame = dynamic_cast<const OpenSim::Frame*>(maybeComponent)) {
                DrawRightClickedFrameContextMenu(*m_EditorAPI, m_Model, m_MaybeSourceVisualizerEvent, *maybeFrame);
            }
            else if (const auto* maybeP2PEdge = dynamic_cast<const PointToPointEdge*>(maybeComponent)) {
                DrawRightClickedPointToPointEdgeContextMenu(*m_EditorAPI, m_Model, m_MaybeSourceVisualizerEvent, *maybeP2PEdge);
            }
            else if (const auto* maybeCPEdge = dynamic_cast<const CrossProductEdge*>(maybeComponent)) {
                DrawRightClickedCrossProductEdgeContextMenu(*m_EditorAPI, m_Model, m_MaybeSourceVisualizerEvent, *maybeCPEdge);
            }
            else {
                DrawRightClickedUnknownComponentContextMenu(*m_EditorAPI, m_Model, m_MaybeSourceVisualizerEvent, *maybeComponent);
            }
        }

        IEditorAPI* m_EditorAPI;
        std::shared_ptr<IModelStatePair> m_Model;
        OpenSim::ComponentPath m_ComponentPath;
        std::optional<ModelEditorViewerPanelRightClickEvent> m_MaybeSourceVisualizerEvent;
    };
}

// other panels/widgets
namespace
{
    class FrameDefinitionTabMainMenu final {
    public:
        explicit FrameDefinitionTabMainMenu(
            std::shared_ptr<UndoableModelStatePair> model_,
            std::shared_ptr<PanelManager> panelManager_) :

            m_Model{std::move(model_)},
            m_WindowMenu{std::move(panelManager_)}
        {}

        void onDraw()
        {
            drawEditMenu();
            m_WindowMenu.on_draw();
            m_AboutMenu.onDraw();
        }

    private:
        void drawEditMenu()
        {
            if (ui::begin_menu("Edit")) {
                if (ui::draw_menu_item(OSC_ICON_UNDO " Undo", {}, false, m_Model->canUndo())) {
                    m_Model->doUndo();
                }

                if (ui::draw_menu_item(OSC_ICON_REDO " Redo", {}, false, m_Model->canRedo())) {
                    m_Model->doRedo();
                }
                ui::end_menu();
            }
        }

        std::shared_ptr<UndoableModelStatePair> m_Model;
        WindowMenu m_WindowMenu;
        MainMenuAboutTab m_AboutMenu;
    };
}

class osc::FrameDefinitionTab::Impl final : public TabPrivate, public IEditorAPI {
public:

    explicit Impl(FrameDefinitionTab& owner, Widget& parent_) :
        TabPrivate{owner, &parent_, c_TabStringID},
        m_Toolbar{"##FrameDefinitionToolbar", parent_, m_Model}
    {
        m_PanelManager->register_toggleable_panel(
            "Navigator",
            [this](std::string_view panelName)
            {
                return std::make_shared<NavigatorPanel>(
                    panelName,
                    m_Model,
                    [this](const OpenSim::ComponentPath& rightClickedPath)
                    {
                        pushPopup(std::make_unique<FrameDefinitionContextMenu>(
                            "##ContextMenu",
                            this,
                            m_Model,
                            rightClickedPath
                        ));
                    }
                );
            }
        );
        m_PanelManager->register_toggleable_panel(
            "Properties",
            [this](std::string_view panelName)
            {
                return std::make_shared<PropertiesPanel>(panelName, this, m_Model);
            }
        );
        m_PanelManager->register_toggleable_panel(
            "Log",
            [](std::string_view panelName)
            {
                return std::make_shared<LogViewerPanel>(panelName);
            }
        );
        m_PanelManager->register_toggleable_panel(
            "Performance",
            [](std::string_view panelName)
            {
                return std::make_shared<PerfPanel>(panelName);
            }
        );
        m_PanelManager->register_spawnable_panel(
            "framedef_viewer",
            [this](std::string_view panelName)
            {
                ModelEditorViewerPanelParameters panelParams
                {
                    m_Model,
                    [this](const ModelEditorViewerPanelRightClickEvent& e)
                    {
                        pushPopup(std::make_unique<FrameDefinitionContextMenu>(
                            "##ContextMenu",
                            this,
                            m_Model,
                            e.componentAbsPathOrEmpty,
                            e
                        ));
                    }
                };
                SetupDefault3DViewportRenderingParams(panelParams.updRenderParams());

                return std::make_shared<ModelEditorViewerPanel>(panelName, panelParams);
            },
            1
        );
    }

    void on_mount()
    {
        App::upd().make_main_loop_waiting();
        m_PanelManager->on_mount();
        m_PopupManager.on_mount();
    }

    void on_unmount()
    {
        m_PanelManager->on_unmount();
        App::upd().make_main_loop_polling();
    }

    bool on_event(Event& e)
    {
        if (e.type() == EventType::KeyDown) {
            return onKeyDown(dynamic_cast<const KeyEvent&>(e));
        }
        else {
            return false;
        }
    }

    void on_tick()
    {
        m_PanelManager->on_tick();
    }

    void onDrawMainMenu()
    {
        m_MainMenu.onDraw();
    }

    void onDraw()
    {
        ui::enable_dockspace_over_main_viewport();

        m_Toolbar.onDraw();
        m_PanelManager->on_draw();
        m_PopupManager.on_draw();
    }

private:
    bool onKeyDown(const KeyEvent& e)
    {
        if (e.matches(KeyModifier::CtrlORGui, KeyModifier::Shift, Key::Z)) {
            // Ctrl+Shift+Z: redo
            m_Model->doRedo();
            return true;
        }
        else if (e.matches(KeyModifier::CtrlORGui, Key::Z)) {
            // Ctrl+Z: undo
            m_Model->doUndo();
            return true;
        }
        else if (e.matches(Key::Backspace) or e.matches(Key::Delete)) {
            // BACKSPACE/DELETE: delete selection
            ActionTryDeleteSelectionFromEditedModel(*m_Model);
            return true;
        }
        else {
            return false;
        }
    }

    void implPushComponentContextMenuPopup(const OpenSim::ComponentPath& componentPath) final
    {
        pushPopup(std::make_unique<FrameDefinitionContextMenu>(
            "##ContextMenu",
            this,
            m_Model,
            componentPath
        ));
    }

    void implPushPopup(std::unique_ptr<IPopup> popup) final
    {
        popup->open();
        m_PopupManager.push_back(std::move(popup));
    }

    void implAddMusclePlot(const OpenSim::Coordinate&, const OpenSim::Muscle&) final
    {
        // ignore: not applicable in this tab
    }

    std::shared_ptr<PanelManager> implGetPanelManager() final
    {
        return m_PanelManager;
    }

    std::shared_ptr<UndoableModelStatePair> m_Model = MakeSharedUndoableFrameDefinitionModel();
    std::shared_ptr<PanelManager> m_PanelManager = std::make_shared<PanelManager>();
    PopupManager m_PopupManager;
    FrameDefinitionTabMainMenu m_MainMenu{m_Model, m_PanelManager};
    FrameDefinitionTabToolbar m_Toolbar;
};


CStringView osc::FrameDefinitionTab::id() { return c_TabStringID; }

osc::FrameDefinitionTab::FrameDefinitionTab(Widget& parent_) :
    Tab{std::make_unique<Impl>(*this, parent_)}
{}
void osc::FrameDefinitionTab::impl_on_mount() { private_data().on_mount(); }
void osc::FrameDefinitionTab::impl_on_unmount() { private_data().on_unmount(); }
bool osc::FrameDefinitionTab::impl_on_event(Event& e) { return private_data().on_event(e); }
void osc::FrameDefinitionTab::impl_on_tick() { private_data().on_tick(); }
void osc::FrameDefinitionTab::impl_on_draw_main_menu() { private_data().onDrawMainMenu(); }
void osc::FrameDefinitionTab::impl_on_draw() { private_data().onDraw(); }
