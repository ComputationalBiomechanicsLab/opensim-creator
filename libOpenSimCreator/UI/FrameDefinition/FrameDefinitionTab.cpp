#include "FrameDefinitionTab.h"

#include <libOpenSimCreator/Documents/CustomComponents/CrossProductEdge.h>
#include <libOpenSimCreator/Documents/CustomComponents/Edge.h>
#include <libOpenSimCreator/Documents/CustomComponents/MidpointLandmark.h>
#include <libOpenSimCreator/Documents/CustomComponents/PointToPointEdge.h>
#include <libOpenSimCreator/Documents/FrameDefinition/FrameDefinitionActions.h>
#include <libOpenSimCreator/Documents/FrameDefinition/FrameDefinitionHelpers.h>
#include <libOpenSimCreator/Documents/Model/IModelStatePair.h>
#include <libOpenSimCreator/Documents/Model/UndoableModelActions.h>
#include <libOpenSimCreator/Documents/Model/UndoableModelStatePair.h>
#include <libOpenSimCreator/UI/Events/OpenComponentContextMenuEvent.h>
#include <libOpenSimCreator/UI/FrameDefinition/FrameDefinitionTabToolbar.h>
#include <libOpenSimCreator/UI/FrameDefinition/FrameDefinitionUIHelpers.h>
#include <libOpenSimCreator/UI/Shared/BasicWidgets.h>
#include <libOpenSimCreator/UI/Shared/ChooseComponentsEditorLayer.h>
#include <libOpenSimCreator/UI/Shared/ChooseComponentsEditorLayerParameters.h>
#include <libOpenSimCreator/UI/Shared/MainMenu.h>
#include <libOpenSimCreator/UI/Shared/ModelViewerPanel.h>
#include <libOpenSimCreator/UI/Shared/ModelViewerPanelParameters.h>
#include <libOpenSimCreator/UI/Shared/ModelViewerPanelRightClickEvent.h>
#include <libOpenSimCreator/UI/Shared/NavigatorPanel.h>
#include <libOpenSimCreator/UI/Shared/PropertiesPanel.h>
#include <libOpenSimCreator/Utils/OpenSimHelpers.h>
#include <libOpenSimCreator/Utils/SimTKConverters.h>

#include <liboscar/Formats/OBJ.h>
#include <liboscar/Graphics/Color.h>
#include <liboscar/Maths/CoordinateDirection.h>
#include <liboscar/Maths/MathHelpers.h>
#include <liboscar/Maths/Vec2.h>
#include <liboscar/Maths/Vec3.h>
#include <liboscar/Platform/App.h>
#include <liboscar/Platform/Events/Event.h>
#include <liboscar/Platform/Events/KeyEvent.h>
#include <liboscar/Platform/IconCodepoints.h>
#include <liboscar/Platform/Log.h>
#include <liboscar/UI/Events/OpenPopupEvent.h>
#include <liboscar/UI/oscimgui.h>
#include <liboscar/UI/Panels/LogViewerPanel.h>
#include <liboscar/UI/Panels/PanelManager.h>
#include <liboscar/UI/Panels/PerfPanel.h>
#include <liboscar/UI/Popups/IPopup.h>
#include <liboscar/UI/Popups/PopupManager.h>
#include <liboscar/UI/Popups/StandardPopup.h>
#include <liboscar/UI/Tabs/TabPrivate.h>
#include <liboscar/UI/Widgets/WindowMenu.h>
#include <liboscar/Utils/Assertions.h>
#include <liboscar/Utils/CStringView.h>
#include <liboscar/Utils/UID.h>
#include <OpenSim/Common/Component.h>
#include <OpenSim/Common/ComponentPath.h>
#include <OpenSim/Common/ComponentSocket.h>
#include <OpenSim/Simulation/Model/Ground.h>
#include <OpenSim/Simulation/Model/Model.h>
#include <OpenSim/Simulation/Model/PhysicalOffsetFrame.h>
#include <OpenSim/Simulation/SimbodyEngine/FreeJoint.h>

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
        PanelManager& panelManager,
        const std::shared_ptr<IModelStatePair>& model,
        const OpenSim::Point& point,
        const ModelViewerPanelRightClickEvent& sourceEvent)
    {
        auto* const visualizer = panelManager.try_upd_panel_by_name_T<ModelViewerPanel>(sourceEvent.sourcePanelName);
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
        PanelManager& panelManager,
        const std::shared_ptr<IModelStatePair>& model,
        const OpenSim::Point& point,
        const ModelViewerPanelRightClickEvent& sourceEvent)
    {
        auto* const visualizer = panelManager.try_upd_panel_by_name_T<ModelViewerPanel>(sourceEvent.sourcePanelName);
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
        PanelManager& panelManager,
        const std::shared_ptr<IModelStatePair>& model,
        const Edge& firstEdge,
        const ModelViewerPanelRightClickEvent& sourceEvent)
    {
        auto* const visualizer = panelManager.try_upd_panel_by_name_T<ModelViewerPanel>(sourceEvent.sourcePanelName);
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
        ModelViewerPanel& visualizer,
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
        ModelViewerPanel& visualizer,
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
        PanelManager& panelManager,
        const std::shared_ptr<IModelStatePair>& model,
        const Edge& firstEdge,
        CoordinateDirection firstEdgeAxis,
        const std::optional<ModelViewerPanelRightClickEvent>& maybeSourceEvent)
    {
        if (model->isReadonly()) {
            return;
        }

        if (not maybeSourceEvent) {
            return;  // there is no way to figure out which visualizer to push the layer to
        }

        auto* const visualizer = panelManager.try_upd_panel_by_name_T<ModelViewerPanel>(maybeSourceEvent->sourcePanelName);
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
        ModelViewerPanel& visualizer,
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
        ModelViewerPanel& visualizer,
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
        ModelViewerPanel& visualizer,
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
        PanelManager& panelManager,
        const std::shared_ptr<IModelStatePair>& model,
        const std::optional<ModelViewerPanelRightClickEvent>& maybeSourceEvent,
        const OpenSim::Frame& frame)
    {
        if (model->isReadonly()) {
            return;
        }

        if (not maybeSourceEvent) {
            return;  // there is no way to figure out which visualizer to push the layer to
        }

        auto* const visualizer = panelManager.try_upd_panel_by_name_T<ModelViewerPanel>(maybeSourceEvent->sourcePanelName);
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
        PanelManager& panelManager,
        const std::shared_ptr<IModelStatePair>&,
        const std::optional<ModelViewerPanelRightClickEvent>& maybeSourceEvent,
        const OpenSim::Component&)
    {
        if (maybeSourceEvent and ui::begin_menu(OSC_ICON_CAMERA " Focus Camera")) {
            if (ui::draw_menu_item("on Ground")) {
                auto* visualizer = panelManager.try_upd_panel_by_name_T<ModelViewerPanel>(maybeSourceEvent->sourcePanelName);
                if (visualizer) {
                    visualizer->focusOn({});
                }
            }

            if (maybeSourceEvent->maybeClickPositionInGround and
                ui::draw_menu_item("on Click Position")) {
                auto* visualizer = panelManager.try_upd_panel_by_name_T<ModelViewerPanel>(maybeSourceEvent->sourcePanelName);
                if (visualizer) {
                    visualizer->focusOn(*maybeSourceEvent->maybeClickPositionInGround);
                }
            }

            ui::end_menu();
        }
    }

    void DrawEdgeAddContextMenuItems(
        PanelManager& panelManager,
        const std::shared_ptr<IModelStatePair>& model,
        const std::optional<ModelViewerPanelRightClickEvent>& maybeSourceEvent,
        const Edge& edge)
    {
        if (maybeSourceEvent and ui::draw_menu_item(OSC_ICON_TIMES " Cross Product Edge")) {
            PushCreateCrossProductEdgeLayer(panelManager, model, edge, *maybeSourceEvent);
        }

        if (maybeSourceEvent and ui::begin_menu(OSC_ICON_ARROWS_ALT " Frame With This Edge as")) {
            ui::push_style_color(ui::ColorVar::Text, Color::muted_red());
            if (ui::draw_menu_item("+x", {}, nullptr, model->canUpdModel())) {
                ActionPushCreateFrameLayer(
                    panelManager,
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
                    panelManager,
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
                    panelManager,
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
                    panelManager,
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
                    panelManager,
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
                    panelManager,
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
        PanelManager& panelManager,
        const std::shared_ptr<IModelStatePair>& model,
        const std::optional<ModelViewerPanelRightClickEvent>& maybeSourceEvent,
        const OpenSim::Frame& frame)
    {
        const OpenSim::Component* groundOrExistingBody = dynamic_cast<const OpenSim::Ground*>(&frame);
        if (not groundOrExistingBody) {
            groundOrExistingBody = FindFirstDescendentOfType<OpenSim::Body>(frame);
        }

        if (ui::draw_menu_item(OSC_ICON_WEIGHT " Body From This", {}, false, groundOrExistingBody == nullptr and model->canUpdModel())) {
            ActionCreateBodyFromFrame(panelManager, model, maybeSourceEvent, frame);
        }
        if (groundOrExistingBody and ui::is_item_hovered(ui::HoveredFlag::AllowWhenDisabled)) {
            std::stringstream ss;
            ss << "Cannot create a body from this frame: it is already the frame of " << groundOrExistingBody->getName();
            ui::draw_tooltip_body_only(std::move(ss).str());
        }
    }
    void DrawMeshAddContextMenuItems(
        const std::shared_ptr<IModelStatePair>& model,
        const std::optional<ModelViewerPanelRightClickEvent>& maybeSourceEvent,
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
        PanelManager& panelManager,
        const std::shared_ptr<IModelStatePair>& model,
        const std::optional<ModelViewerPanelRightClickEvent>& maybeSourceEvent,
        const OpenSim::Point& point)
    {
        if (maybeSourceEvent and ui::draw_menu_item(OSC_ICON_GRIP_LINES " Edge", {}, nullptr, model->canUpdModel())) {
            PushCreateEdgeToOtherPointLayer(panelManager, model, point, *maybeSourceEvent);
        }
        if (maybeSourceEvent and ui::draw_menu_item(OSC_ICON_DOT_CIRCLE " Midpoint", {}, nullptr, model->canUpdModel())) {
            PushCreateMidpointToAnotherPointLayer(panelManager, model, point, *maybeSourceEvent);
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
        PanelManager& panelManager,
        const std::shared_ptr<IModelStatePair>& model,
        const std::optional<ModelViewerPanelRightClickEvent>& maybeSourceEvent,
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
        DrawFocusCameraMenu(panelManager, model, maybeSourceEvent, mesh);
    }

    void DrawRightClickedPointContextMenu(
        PanelManager& panelManager,
        const std::shared_ptr<IModelStatePair>& model,
        const std::optional<ModelViewerPanelRightClickEvent>& maybeSourceEvent,
        const OpenSim::Point& point)
    {
        DrawRightClickedComponentContextMenuHeader(point);
        DrawContextMenuSeparator();

        if (ui::begin_menu(OSC_ICON_PLUS " Add")) {
            DrawPointAddContextMenuItems(panelManager, model, maybeSourceEvent, point);
            ui::end_menu();
        }
        osc::DrawCalculateMenu(model->getModel(), model->getState(), point);
        DrawFocusCameraMenu(panelManager, model, maybeSourceEvent, point);
    }

    void DrawRightClickedPointToPointEdgeContextMenu(
        PanelManager& panelManager,
        const std::shared_ptr<IModelStatePair>& model,
        const std::optional<ModelViewerPanelRightClickEvent>& maybeSourceEvent,
        const PointToPointEdge& edge)
    {
        DrawRightClickedComponentContextMenuHeader(edge);
        DrawContextMenuSeparator();

        if (ui::begin_menu(OSC_ICON_PLUS " Add")) {
            DrawEdgeAddContextMenuItems(panelManager, model, maybeSourceEvent, edge);
            ui::end_menu();
        }
        if (ui::draw_menu_item(OSC_ICON_RECYCLE " Swap Direction", {}, nullptr, model->canUpdModel())) {
            ActionSwapPointToPointEdgeEnds(*model, edge);
        }
        DrawCalculateMenu(model->getModel(), model->getState(), edge);
        DrawFocusCameraMenu(panelManager, model, maybeSourceEvent, edge);
    }

    void DrawRightClickedCrossProductEdgeContextMenu(
        PanelManager& panelManager,
        const std::shared_ptr<IModelStatePair>& model,
        const std::optional<ModelViewerPanelRightClickEvent>& maybeSourceEvent,
        const CrossProductEdge& edge)
    {
        DrawRightClickedComponentContextMenuHeader(edge);
        DrawContextMenuSeparator();

        if (ui::begin_menu(OSC_ICON_PLUS " Add")) {
            DrawEdgeAddContextMenuItems(panelManager, model, maybeSourceEvent, edge);
            ui::end_menu();
        }
        if (ui::draw_menu_item(OSC_ICON_RECYCLE " Swap Operands")) {
            ActionSwapCrossProductEdgeOperands(*model, edge);
        }
        DrawCalculateMenu(model->getModel(), model->getState(), edge);
        DrawFocusCameraMenu(panelManager, model, maybeSourceEvent, edge);
    }

    void DrawRightClickedFrameContextMenu(
        PanelManager& panelManager,
        const std::shared_ptr<IModelStatePair>& model,
        const std::optional<ModelViewerPanelRightClickEvent>& maybeSourceEvent,
        const OpenSim::Frame& frame)
    {
        DrawRightClickedComponentContextMenuHeader(frame);
        DrawContextMenuSeparator();

        if (ui::begin_menu(OSC_ICON_PLUS " Add")) {
            DrawCreateBodyMenuItem(panelManager, model, maybeSourceEvent, frame);
            ui::end_menu();
        }
        osc::DrawCalculateMenu(model->getModel(), model->getState(), frame);
        DrawFocusCameraMenu(panelManager, model, maybeSourceEvent, frame);
    }

    void DrawRightClickedUnknownComponentContextMenu(
        PanelManager& panelManager,
        const std::shared_ptr<IModelStatePair>& model,
        const std::optional<ModelViewerPanelRightClickEvent>& maybeSourceEvent,
        const OpenSim::Component& component)
    {
        DrawRightClickedComponentContextMenuHeader(component);
        DrawContextMenuSeparator();

        DrawFocusCameraMenu(panelManager, model, maybeSourceEvent, component);
    }

    // popup state for the frame definition tab's general context menu
    class FrameDefinitionContextMenu final : public StandardPopup {
    public:
        FrameDefinitionContextMenu(
            std::string_view popupName_,
            std::shared_ptr<PanelManager> panelManager_,
            std::shared_ptr<IModelStatePair> model_,
            OpenSim::ComponentPath componentPath_,
            std::optional<ModelViewerPanelRightClickEvent> maybeSourceVisualizerEvent_ = std::nullopt) :

            StandardPopup{popupName_, {10.0f, 10.0f}, ui::WindowFlag::NoMove},
            m_PanelManager{std::move(panelManager_)},
            m_Model{std::move(model_)},
            m_ComponentPath{std::move(componentPath_)},
            m_MaybeSourceVisualizerEvent{std::move(maybeSourceVisualizerEvent_)}
        {
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
                DrawRightClickedMeshContextMenu(*m_PanelManager, m_Model, m_MaybeSourceVisualizerEvent, *maybeMesh);
            }
            else if (const auto* maybePoint = dynamic_cast<const OpenSim::Point*>(maybeComponent)) {
                DrawRightClickedPointContextMenu(*m_PanelManager, m_Model, m_MaybeSourceVisualizerEvent, *maybePoint);
            }
            else if (const auto* maybeFrame = dynamic_cast<const OpenSim::Frame*>(maybeComponent)) {
                DrawRightClickedFrameContextMenu(*m_PanelManager, m_Model, m_MaybeSourceVisualizerEvent, *maybeFrame);
            }
            else if (const auto* maybeP2PEdge = dynamic_cast<const PointToPointEdge*>(maybeComponent)) {
                DrawRightClickedPointToPointEdgeContextMenu(*m_PanelManager, m_Model, m_MaybeSourceVisualizerEvent, *maybeP2PEdge);
            }
            else if (const auto* maybeCPEdge = dynamic_cast<const CrossProductEdge*>(maybeComponent)) {
                DrawRightClickedCrossProductEdgeContextMenu(*m_PanelManager, m_Model, m_MaybeSourceVisualizerEvent, *maybeCPEdge);
            }
            else {
                DrawRightClickedUnknownComponentContextMenu(*m_PanelManager, m_Model, m_MaybeSourceVisualizerEvent, *maybeComponent);
            }
        }

        std::shared_ptr<PanelManager> m_PanelManager;
        std::shared_ptr<IModelStatePair> m_Model;
        OpenSim::ComponentPath m_ComponentPath;
        std::optional<ModelViewerPanelRightClickEvent> m_MaybeSourceVisualizerEvent;
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

class osc::FrameDefinitionTab::Impl final : public TabPrivate {
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
                        auto popup = std::make_unique<FrameDefinitionContextMenu>(
                            "##ContextMenu",
                            m_PanelManager,
                            m_Model,
                            rightClickedPath
                        );
                        App::post_event<OpenPopupEvent>(this->owner(), std::move(popup));
                    }
                );
            }
        );
        m_PanelManager->register_toggleable_panel(
            "Properties",
            [this](std::string_view panelName)
            {
                return std::make_shared<PropertiesPanel>(panelName, this->owner(), m_Model);
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
                ModelViewerPanelParameters panelParams
                {
                    m_Model,
                    [this](const ModelViewerPanelRightClickEvent& e)
                    {
                        auto popup = std::make_unique<FrameDefinitionContextMenu>(
                            "##ContextMenu",
                            m_PanelManager,
                            m_Model,
                            e.componentAbsPathOrEmpty,
                            e
                        );
                        App::post_event<OpenPopupEvent>(this->owner(), std::move(popup));
                    }
                };
                SetupDefault3DViewportRenderingParams(panelParams.updRenderParams());

                return std::make_shared<ModelViewerPanel>(panelName, panelParams);
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
        if (auto* openPopup = dynamic_cast<OpenPopupEvent*>(&e)) {
            if (openPopup->has_tab()) {
                auto tab = openPopup->take_tab();
                tab->open();
                m_PopupManager.push_back(std::move(tab));
                return true;
            }
        }
        else if (auto* contextMenuEvent = dynamic_cast<OpenComponentContextMenuEvent*>(&e)) {
            auto popup = std::make_unique<FrameDefinitionContextMenu>(
                "##ContextMenu",
                m_PanelManager,
                m_Model,
                contextMenuEvent->path()
            );
            App::post_event<OpenPopupEvent>(this->owner(), std::move(popup));
            return true;
        }

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
