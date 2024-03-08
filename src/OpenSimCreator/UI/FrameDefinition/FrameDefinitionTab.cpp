#include "FrameDefinitionTab.h"

#include <OpenSimCreator/Documents/FrameDefinition/AxisIndex.h>
#include <OpenSimCreator/Documents/FrameDefinition/CrossProductEdge.h>
#include <OpenSimCreator/Documents/FrameDefinition/Edge.h>
#include <OpenSimCreator/Documents/FrameDefinition/FrameDefinitionActions.h>
#include <OpenSimCreator/Documents/FrameDefinition/FrameDefinitionHelpers.h>
#include <OpenSimCreator/Documents/FrameDefinition/MaybeNegatedAxis.h>
#include <OpenSimCreator/Documents/FrameDefinition/MidpointLandmark.h>
#include <OpenSimCreator/Documents/FrameDefinition/PointToPointEdge.h>
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
#include <OpenSimCreator/Utils/OpenSimHelpers.h>
#include <OpenSimCreator/Utils/SimTKHelpers.h>

#include <IconsFontAwesome5.h>
#include <SDL_events.h>
#include <OpenSim/Common/Component.h>
#include <OpenSim/Common/ComponentPath.h>
#include <OpenSim/Common/ComponentSocket.h>
#include <OpenSim/Simulation/Model/Ground.h>
#include <OpenSim/Simulation/Model/Model.h>
#include <OpenSim/Simulation/Model/PhysicalOffsetFrame.h>
#include <OpenSim/Simulation/SimbodyEngine/FreeJoint.h>
#include <oscar/Formats/OBJ.h>
#include <oscar/Graphics/Color.h>
#include <oscar/Maths/MathHelpers.h>
#include <oscar/Maths/Vec2.h>
#include <oscar/Maths/Vec3.h>
#include <oscar/Platform/App.h>
#include <oscar/Platform/Log.h>
#include <oscar/UI/ImGuiHelpers.h>
#include <oscar/UI/oscimgui.h>
#include <oscar/UI/Panels/LogViewerPanel.h>
#include <oscar/UI/Panels/PanelManager.h>
#include <oscar/UI/Panels/PerfPanel.h>
#include <oscar/UI/Widgets/IPopup.h>
#include <oscar/UI/Widgets/PopupManager.h>
#include <oscar/UI/Widgets/StandardPopup.h>
#include <oscar/UI/Widgets/WindowMenu.h>
#include <oscar/Utils/Assertions.h>
#include <oscar/Utils/CStringView.h>
#include <oscar/Utils/ParentPtr.h>
#include <oscar/Utils/UID.h>

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
        std::shared_ptr<UndoableModelStatePair> const& model,
        OpenSim::Point const& point,
        ModelEditorViewerPanelRightClickEvent const& sourceEvent)
    {
        auto* const visualizer = editor.getPanelManager()->tryUpdPanelByNameT<ModelEditorViewerPanel>(sourceEvent.sourcePanelName);
        if (!visualizer)
        {
            return;  // can't figure out which visualizer to push the layer to
        }

        ChooseComponentsEditorLayerParameters options;
        options.popupHeaderText = "choose other point";
        options.canChooseItem = IsPoint;
        options.componentsBeingAssignedTo = {point.getAbsolutePathString()};
        options.numComponentsUserMustChoose = 1;
        options.onUserFinishedChoosing = [model, pointAPath = point.getAbsolutePathString()](std::unordered_set<std::string> const& choices) -> bool
        {
            if (choices.empty())
            {
                log_error("user selections from the 'choose components' layer was empty: this bug should be reported");
                return false;
            }
            if (choices.size() > 1)
            {
                log_warn("number of user selections from 'choose components' layer was greater than expected: this bug should be reported");
            }
            std::string const& pointBPath = *choices.begin();

            auto const* pointA = FindComponent<OpenSim::Point>(model->getModel(), pointAPath);
            if (!pointA)
            {
                log_error("point A's component path (%s) does not exist in the model", pointAPath.c_str());
                return false;
            }

            auto const* pointB = FindComponent<OpenSim::Point>(model->getModel(), pointBPath);
            if (!pointB)
            {
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
        std::shared_ptr<UndoableModelStatePair> const& model,
        OpenSim::Point const& point,
        ModelEditorViewerPanelRightClickEvent const& sourceEvent)
    {
        auto* const visualizer = editor.getPanelManager()->tryUpdPanelByNameT<ModelEditorViewerPanel>(sourceEvent.sourcePanelName);
        if (!visualizer)
        {
            return;  // can't figure out which visualizer to push the layer to
        }

        ChooseComponentsEditorLayerParameters options;
        options.popupHeaderText = "choose other point";
        options.canChooseItem = IsPoint;
        options.componentsBeingAssignedTo = {point.getAbsolutePathString()};
        options.numComponentsUserMustChoose = 1;
        options.onUserFinishedChoosing = [model, pointAPath = point.getAbsolutePathString()](std::unordered_set<std::string> const& choices) -> bool
        {
            if (choices.empty())
            {
                log_error("user selections from the 'choose components' layer was empty: this bug should be reported");
                return false;
            }
            if (choices.size() > 1)
            {
                log_warn("number of user selections from 'choose components' layer was greater than expected: this bug should be reported");
            }
            std::string const& pointBPath = *choices.begin();

            auto const* pointA = FindComponent<OpenSim::Point>(model->getModel(), pointAPath);
            if (!pointA)
            {
                log_error("point A's component path (%s) does not exist in the model", pointAPath.c_str());
                return false;
            }

            auto const* pointB = FindComponent<OpenSim::Point>(model->getModel(), pointBPath);
            if (!pointB)
            {
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
        std::shared_ptr<UndoableModelStatePair> const& model,
        Edge const& firstEdge,
        ModelEditorViewerPanelRightClickEvent const& sourceEvent)
    {
        auto* const visualizer = editor.getPanelManager()->tryUpdPanelByNameT<ModelEditorViewerPanel>(sourceEvent.sourcePanelName);
        if (!visualizer)
        {
            return;  // can't figure out which visualizer to push the layer to
        }

        ChooseComponentsEditorLayerParameters options;
        options.popupHeaderText = "choose other edge";
        options.canChooseItem = IsEdge;
        options.componentsBeingAssignedTo = {firstEdge.getAbsolutePathString()};
        options.numComponentsUserMustChoose = 1;
        options.onUserFinishedChoosing = [model, edgeAPath = firstEdge.getAbsolutePathString()](std::unordered_set<std::string> const& choices) -> bool
        {
            if (choices.empty())
            {
                log_error("user selections from the 'choose components' layer was empty: this bug should be reported");
                return false;
            }
            if (choices.size() > 1)
            {
                log_warn("number of user selections from 'choose components' layer was greater than expected: this bug should be reported");
            }
            std::string const& edgeBPath = *choices.begin();

            auto const* edgeA = FindComponent<Edge>(model->getModel(), edgeAPath);
            if (!edgeA)
            {
                log_error("edge A's component path (%s) does not exist in the model", edgeAPath.c_str());
                return false;
            }

            auto const* edgeB = FindComponent<Edge>(model->getModel(), edgeBPath);
            if (!edgeB)
            {
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
        std::shared_ptr<UndoableModelStatePair> const& model,
        std::string const& firstEdgeAbsPath,
        MaybeNegatedAxis firstEdgeAxis,
        std::string const& secondEdgeAbsPath)
    {
        ChooseComponentsEditorLayerParameters options;
        options.popupHeaderText = "choose frame origin";
        options.canChooseItem = IsPoint;
        options.numComponentsUserMustChoose = 1;
        options.onUserFinishedChoosing = [
            model,
            firstEdgeAbsPath = firstEdgeAbsPath,
            firstEdgeAxis,
            secondEdgeAbsPath
        ](std::unordered_set<std::string> const& choices) -> bool
        {
            if (choices.empty())
            {
                log_error("user selections from the 'choose components' layer was empty: this bug should be reported");
                return false;
            }
            if (choices.size() > 1)
            {
                log_warn("number of user selections from 'choose components' layer was greater than expected: this bug should be reported");
            }
            std::string const& originPath = *choices.begin();

            auto const* firstEdge = FindComponent<Edge>(model->getModel(), firstEdgeAbsPath);
            if (!firstEdge)
            {
                log_error("the first edge's component path (%s) does not exist in the model", firstEdgeAbsPath.c_str());
                return false;
            }

            auto const* otherEdge = FindComponent<Edge>(model->getModel(), secondEdgeAbsPath);
            if (!otherEdge)
            {
                log_error("the second edge's component path (%s) does not exist in the model", secondEdgeAbsPath.c_str());
                return false;
            }

            auto const* originPoint = FindComponent<OpenSim::Point>(model->getModel(), originPath);
            if (!originPoint)
            {
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
        std::shared_ptr<UndoableModelStatePair> const& model,
        Edge const& firstEdge,
        MaybeNegatedAxis firstEdgeAxis)
    {
        ChooseComponentsEditorLayerParameters options;
        options.popupHeaderText = "choose other edge";
        options.canChooseItem = IsEdge;
        options.componentsBeingAssignedTo = {firstEdge.getAbsolutePathString()};
        options.numComponentsUserMustChoose = 1;
        options.onUserFinishedChoosing = [
            visualizerPtr = &visualizer,  // TODO: implement weak_ptr for panel lookup
                model,
                firstEdgeAbsPath = firstEdge.getAbsolutePathString(),
                firstEdgeAxis
        ](std::unordered_set<std::string> const& choices) -> bool
        {
            // go into "pick origin" state

            if (choices.empty())
            {
                log_error("user selections from the 'choose components' layer was empty: this bug should be reported");
                return false;
            }
            std::string const& otherEdgePath = *choices.begin();

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
        std::shared_ptr<UndoableModelStatePair> const& model,
        Edge const& firstEdge,
        MaybeNegatedAxis firstEdgeAxis,
        std::optional<ModelEditorViewerPanelRightClickEvent> const& maybeSourceEvent)
    {
        if (!maybeSourceEvent)
        {
            return;  // there is no way to figure out which visualizer to push the layer to
        }

        auto* const visualizer = editor.getPanelManager()->tryUpdPanelByNameT<ModelEditorViewerPanel>(maybeSourceEvent->sourcePanelName);
        if (!visualizer)
        {
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
        std::shared_ptr<UndoableModelStatePair> const& model,
        OpenSim::ComponentPath const& frameAbsPath,
        OpenSim::ComponentPath const& meshAbsPath,
        OpenSim::ComponentPath const& jointFrameAbsPath)
    {
        ChooseComponentsEditorLayerParameters options;
        options.popupHeaderText = "choose parent frame";
        options.canChooseItem = [bodyFrame = FindComponent(model->getModel(), frameAbsPath)](OpenSim::Component const& c)
        {
            return
                IsPhysicalFrame(c) &&
                &c != bodyFrame &&
                !IsChildOfA<OpenSim::ComponentSet>(c) &&
                (
                    dynamic_cast<OpenSim::Ground const*>(&c) != nullptr ||
                    IsChildOfA<OpenSim::BodySet>(c)
                );
        };
        options.numComponentsUserMustChoose = 1;
        options.onUserFinishedChoosing = [
            model,
            frameAbsPath,
            meshAbsPath,
            jointFrameAbsPath
        ](std::unordered_set<std::string> const& choices) -> bool
        {
            if (choices.empty())
            {
                log_error("user selections from the 'choose components' layer was empty: this bug should be reported");
                return false;
            }

            auto const* const parentFrame = FindComponent<OpenSim::PhysicalFrame>(model->getModel(), *choices.begin());
            if (!parentFrame)
            {
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
        std::shared_ptr<UndoableModelStatePair> const& model,
        OpenSim::ComponentPath const& frameAbsPath,
        OpenSim::ComponentPath const& meshAbsPath)
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
        ](std::unordered_set<std::string> const& choices) -> bool
        {
            if (choices.empty())
            {
                log_error("user selections from the 'choose components' layer was empty: this bug should be reported");
                return false;
            }

            auto const* const jointFrame = FindComponent<OpenSim::Frame>(model->getModel(), *choices.begin());
            if (!jointFrame)
            {
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
        std::shared_ptr<UndoableModelStatePair> const& model,
        OpenSim::Frame const& frame)
    {
        ChooseComponentsEditorLayerParameters options;
        options.popupHeaderText = "choose mesh to attach the body to";
        options.canChooseItem = [](OpenSim::Component const& c) { return IsMesh(c) && !IsChildOfA<OpenSim::Body>(c); };
        options.numComponentsUserMustChoose = 1;
        options.onUserFinishedChoosing = [
            visualizerPtr = &visualizer,  // TODO: implement weak_ptr for panel lookup
            model,
            frameAbsPath = frame.getAbsolutePath()
        ](std::unordered_set<std::string> const& choices) -> bool
        {
            if (choices.empty())
            {
                log_error("user selections from the 'choose components' layer was empty: this bug should be reported");
                return false;
            }

            auto const* const mesh = FindComponent<OpenSim::Mesh>(model->getModel(), *choices.begin());
            if (!mesh)
            {
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
        std::shared_ptr<UndoableModelStatePair> const& model,
        std::optional<ModelEditorViewerPanelRightClickEvent> const& maybeSourceEvent,
        OpenSim::Frame const& frame)
    {
        if (!maybeSourceEvent)
        {
            return;  // there is no way to figure out which visualizer to push the layer to
        }

        auto* const visualizer = editor.getPanelManager()->tryUpdPanelByNameT<ModelEditorViewerPanel>(maybeSourceEvent->sourcePanelName);
        if (!visualizer)
        {
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
        OpenSim::Component const& root,
        SimTK::State const& state,
        Edge const& edge)
    {
        if (ui::BeginMenu(ICON_FA_CALCULATOR " Calculate"))
        {
            if (ui::BeginMenu("Start Point"))
            {
                auto const onFrameMenuOpened = [&state, &edge](OpenSim::Frame const& frame)
                {
                    DrawPointTranslationInformationWithRespectTo(
                        frame,
                        state,
                        ToVec3(edge.getStartLocationInGround(state))
                    );
                };
                DrawWithRespectToMenuContainingMenuPerFrame(root, onFrameMenuOpened, nullptr);
                ui::EndMenu();
            }

            if (ui::BeginMenu("End Point"))
            {
                auto const onFrameMenuOpened = [&state, &edge](OpenSim::Frame const& frame)
                {
                    DrawPointTranslationInformationWithRespectTo(
                        frame,
                        state,
                        ToVec3(edge.getEndLocationInGround(state))
                    );
                };

                DrawWithRespectToMenuContainingMenuPerFrame(root, onFrameMenuOpened, nullptr);
                ui::EndMenu();
            }

            if (ui::BeginMenu("Direction"))
            {
                auto const onFrameMenuOpened = [&state, &edge](OpenSim::Frame const& frame)
                {
                    DrawDirectionInformationWithRepsectTo(
                        frame,
                        state,
                        ToVec3(CalcDirection(edge.getLocationsInGround(state)))
                    );
                };

                DrawWithRespectToMenuContainingMenuPerFrame(root, onFrameMenuOpened, nullptr);
                ui::EndMenu();
            }

            ui::EndMenu();
        }
    }

    void DrawFocusCameraMenu(
        IEditorAPI& editor,
        std::shared_ptr<UndoableModelStatePair> const&,
        std::optional<ModelEditorViewerPanelRightClickEvent> const& maybeSourceEvent,
        OpenSim::Component const&)
    {
        if (maybeSourceEvent && ui::BeginMenu(ICON_FA_CAMERA " Focus Camera"))
        {
            if (ui::MenuItem("on Ground"))
            {
                auto* visualizer = editor.getPanelManager()->tryUpdPanelByNameT<ModelEditorViewerPanel>(maybeSourceEvent->sourcePanelName);
                if (visualizer)
                {
                    visualizer->focusOn({});
                }
            }

            if (maybeSourceEvent->maybeClickPositionInGround &&
                ui::MenuItem("on Click Position"))
            {
                auto* visualizer = editor.getPanelManager()->tryUpdPanelByNameT<ModelEditorViewerPanel>(maybeSourceEvent->sourcePanelName);
                if (visualizer)
                {
                    visualizer->focusOn(*maybeSourceEvent->maybeClickPositionInGround);
                }
            }

            ui::EndMenu();
        }
    }

    void DrawEdgeAddContextMenuItems(
        IEditorAPI& editor,
        std::shared_ptr<UndoableModelStatePair> const& model,
        std::optional<ModelEditorViewerPanelRightClickEvent> const& maybeSourceEvent,
        Edge const& edge)
    {
        if (maybeSourceEvent && ui::MenuItem(ICON_FA_TIMES " Cross Product Edge"))
        {
            PushCreateCrossProductEdgeLayer(editor, model, edge, *maybeSourceEvent);
        }

        if (maybeSourceEvent && ui::BeginMenu(ICON_FA_ARROWS_ALT " Frame With This Edge as"))
        {
            ui::PushStyleColor(ImGuiCol_Text, Color::muted_red());
            if (ui::MenuItem("+x"))
            {
                ActionPushCreateFrameLayer(
                    editor,
                    model,
                    edge,
                    MaybeNegatedAxis{AxisIndex::X, false},
                    maybeSourceEvent
                );
            }
            ui::PopStyleColor();

            ui::PushStyleColor(ImGuiCol_Text, Color::muted_green());
            if (ui::MenuItem("+y"))
            {
                ActionPushCreateFrameLayer(
                    editor,
                    model,
                    edge,
                    MaybeNegatedAxis{AxisIndex::Y, false},
                    maybeSourceEvent
                );
            }
            ui::PopStyleColor();

            ui::PushStyleColor(ImGuiCol_Text, Color::muted_blue());
            if (ui::MenuItem("+z"))
            {
                ActionPushCreateFrameLayer(
                    editor,
                    model,
                    edge,
                    MaybeNegatedAxis{AxisIndex::Z, false},
                    maybeSourceEvent
                );
            }
            ui::PopStyleColor();

            ui::Separator();

            ui::PushStyleColor(ImGuiCol_Text, Color::muted_red());
            if (ui::MenuItem("-x"))
            {
                ActionPushCreateFrameLayer(
                    editor,
                    model,
                    edge,
                    MaybeNegatedAxis{AxisIndex::X, true},
                    maybeSourceEvent
                );
            }
            ui::PopStyleColor();

            ui::PushStyleColor(ImGuiCol_Text, Color::muted_green());
            if (ui::MenuItem("-y"))
            {
                ActionPushCreateFrameLayer(
                    editor,
                    model,
                    edge,
                    MaybeNegatedAxis{AxisIndex::Y, true},
                    maybeSourceEvent
                );
            }
            ui::PopStyleColor();

            ui::PushStyleColor(ImGuiCol_Text, Color::muted_blue());
            if (ui::MenuItem("-z"))
            {
                ActionPushCreateFrameLayer(
                    editor,
                    model,
                    edge,
                    MaybeNegatedAxis{AxisIndex::Z, true},
                    maybeSourceEvent
                );
            }
            ui::PopStyleColor();

            ui::EndMenu();
        }
    }

    void DrawCreateBodyMenuItem(
        IEditorAPI& editor,
        std::shared_ptr<UndoableModelStatePair> const& model,
        std::optional<ModelEditorViewerPanelRightClickEvent> const& maybeSourceEvent,
        OpenSim::Frame const& frame)
    {
        OpenSim::Component const* groundOrExistingBody = dynamic_cast<OpenSim::Ground const*>(&frame);
        if (!groundOrExistingBody)
        {
            groundOrExistingBody = FindFirstDescendentOfType<OpenSim::Body>(frame);
        }

        if (ui::MenuItem(ICON_FA_WEIGHT " Body From This", {}, false, groundOrExistingBody == nullptr))
        {
            ActionCreateBodyFromFrame(editor, model, maybeSourceEvent, frame);
        }
        if (groundOrExistingBody && ui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
        {
            std::stringstream ss;
            ss << "Cannot create a body from this frame: it is already the frame of " << groundOrExistingBody->getName();
            ui::DrawTooltipBodyOnly(std::move(ss).str());
        }
    }
    void DrawMeshAddContextMenuItems(
        std::shared_ptr<UndoableModelStatePair> const& model,
        std::optional<ModelEditorViewerPanelRightClickEvent> const& maybeSourceEvent,
        OpenSim::Mesh const& mesh)
    {
        if (ui::MenuItem(ICON_FA_CIRCLE " Sphere Landmark"))
        {
            ActionAddSphereInMeshFrame(
                *model,
                mesh,
                maybeSourceEvent ? maybeSourceEvent->maybeClickPositionInGround : std::nullopt
            );
        }
        if (ui::MenuItem(ICON_FA_ARROWS_ALT " Custom (Offset) Frame"))
        {
            ActionAddOffsetFrameInMeshFrame(
                *model,
                mesh,
                maybeSourceEvent ? maybeSourceEvent->maybeClickPositionInGround : std::nullopt
            );
        }
    }

    void DrawPointAddContextMenuItems(
        IEditorAPI& editor,
        std::shared_ptr<UndoableModelStatePair> const& model,
        std::optional<ModelEditorViewerPanelRightClickEvent> const& maybeSourceEvent,
        OpenSim::Point const& point)
    {
        if (maybeSourceEvent && ui::MenuItem(ICON_FA_GRIP_LINES " Edge"))
        {
            PushCreateEdgeToOtherPointLayer(editor, model, point, *maybeSourceEvent);
        }
        if (maybeSourceEvent && ui::MenuItem(ICON_FA_DOT_CIRCLE " Midpoint"))
        {
            PushCreateMidpointToAnotherPointLayer(editor, model, point, *maybeSourceEvent);
        }
    }

    void DrawRightClickedNothingContextMenu(
        UndoableModelStatePair& model)
    {
        DrawNothingRightClickedContextMenuHeader();
        DrawContextMenuSeparator();

        if (ui::BeginMenu(ICON_FA_PLUS " Add"))
        {
            if (ui::MenuItem(ICON_FA_CUBES " Meshes"))
            {
                ActionPromptUserToAddMeshFiles(model);
            }
            ui::EndMenu();
        }
    }

    void DrawRightClickedMeshContextMenu(
        IEditorAPI& editor,
        std::shared_ptr<UndoableModelStatePair> const& model,
        std::optional<ModelEditorViewerPanelRightClickEvent> const& maybeSourceEvent,
        OpenSim::Mesh const& mesh)
    {
        DrawRightClickedComponentContextMenuHeader(mesh);
        DrawContextMenuSeparator();

        if (ui::BeginMenu(ICON_FA_PLUS " Add"))
        {
            DrawMeshAddContextMenuItems(model, maybeSourceEvent, mesh);
            ui::EndMenu();
        }
        if (ui::BeginMenu(ICON_FA_FILE_EXPORT " Export"))
        {
            DrawMeshExportContextMenuContent(*model, mesh);
            ui::EndMenu();
        }
        DrawFocusCameraMenu(editor, model, maybeSourceEvent, mesh);
    }

    void DrawRightClickedPointContextMenu(
        IEditorAPI& editor,
        std::shared_ptr<UndoableModelStatePair> const& model,
        std::optional<ModelEditorViewerPanelRightClickEvent> const& maybeSourceEvent,
        OpenSim::Point const& point)
    {
        DrawRightClickedComponentContextMenuHeader(point);
        DrawContextMenuSeparator();

        if (ui::BeginMenu(ICON_FA_PLUS " Add"))
        {
            DrawPointAddContextMenuItems(editor, model, maybeSourceEvent, point);
            ui::EndMenu();
        }
        osc::DrawCalculateMenu(model->getModel(), model->getState(), point);
        DrawFocusCameraMenu(editor, model, maybeSourceEvent, point);
    }

    void DrawRightClickedPointToPointEdgeContextMenu(
        IEditorAPI& editor,
        std::shared_ptr<UndoableModelStatePair> const& model,
        std::optional<ModelEditorViewerPanelRightClickEvent> const& maybeSourceEvent,
        PointToPointEdge const& edge)
    {
        DrawRightClickedComponentContextMenuHeader(edge);
        DrawContextMenuSeparator();

        if (ui::BeginMenu(ICON_FA_PLUS " Add"))
        {
            DrawEdgeAddContextMenuItems(editor, model, maybeSourceEvent, edge);
            ui::EndMenu();
        }
        if (ui::MenuItem(ICON_FA_RECYCLE " Swap Direction"))
        {
            ActionSwapPointToPointEdgeEnds(*model, edge);
        }
        DrawCalculateMenu(model->getModel(), model->getState(), edge);
        DrawFocusCameraMenu(editor, model, maybeSourceEvent, edge);
    }

    void DrawRightClickedCrossProductEdgeContextMenu(
        IEditorAPI& editor,
        std::shared_ptr<UndoableModelStatePair> const& model,
        std::optional<ModelEditorViewerPanelRightClickEvent> const& maybeSourceEvent,
        CrossProductEdge const& edge)
    {
        DrawRightClickedComponentContextMenuHeader(edge);
        DrawContextMenuSeparator();

        if (ui::BeginMenu(ICON_FA_PLUS " Add"))
        {
            DrawEdgeAddContextMenuItems(editor, model, maybeSourceEvent, edge);
            ui::EndMenu();
        }
        if (ui::MenuItem(ICON_FA_RECYCLE " Swap Operands"))
        {
            ActionSwapCrossProductEdgeOperands(*model, edge);
        }
        DrawCalculateMenu(model->getModel(), model->getState(), edge);
        DrawFocusCameraMenu(editor, model, maybeSourceEvent, edge);
    }

    void DrawRightClickedFrameContextMenu(
        IEditorAPI& editor,
        std::shared_ptr<UndoableModelStatePair> const& model,
        std::optional<ModelEditorViewerPanelRightClickEvent> const& maybeSourceEvent,
        OpenSim::Frame const& frame)
    {
        DrawRightClickedComponentContextMenuHeader(frame);
        DrawContextMenuSeparator();

        if (ui::BeginMenu(ICON_FA_PLUS " Add"))
        {
            DrawCreateBodyMenuItem(editor, model, maybeSourceEvent, frame);
            ui::EndMenu();
        }
        osc::DrawCalculateMenu(model->getModel(), model->getState(), frame);
        DrawFocusCameraMenu(editor, model, maybeSourceEvent, frame);
    }

    void DrawRightClickedUnknownComponentContextMenu(
        IEditorAPI& editor,
        std::shared_ptr<UndoableModelStatePair> const& model,
        std::optional<ModelEditorViewerPanelRightClickEvent> const& maybeSourceEvent,
        OpenSim::Component const& component)
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
            std::shared_ptr<UndoableModelStatePair> model_,
            OpenSim::ComponentPath componentPath_,
            std::optional<ModelEditorViewerPanelRightClickEvent> maybeSourceVisualizerEvent_ = std::nullopt) :

            StandardPopup{popupName_, {10.0f, 10.0f}, ImGuiWindowFlags_NoMove},
            m_EditorAPI{editorAPI_},
            m_Model{std::move(model_)},
            m_ComponentPath{std::move(componentPath_)},
            m_MaybeSourceVisualizerEvent{std::move(maybeSourceVisualizerEvent_)}
        {
            OSC_ASSERT(m_EditorAPI != nullptr);
            OSC_ASSERT(m_Model != nullptr);

            setModal(false);
        }

    private:
        void implDrawContent() final
        {
            OpenSim::Component const* const maybeComponent = FindComponent(m_Model->getModel(), m_ComponentPath);
            if (!maybeComponent)
            {
                DrawRightClickedNothingContextMenu(*m_Model);
            }
            else if (auto const* maybeMesh = dynamic_cast<OpenSim::Mesh const*>(maybeComponent))
            {
                DrawRightClickedMeshContextMenu(*m_EditorAPI, m_Model, m_MaybeSourceVisualizerEvent, *maybeMesh);
            }
            else if (auto const* maybePoint = dynamic_cast<OpenSim::Point const*>(maybeComponent))
            {
                DrawRightClickedPointContextMenu(*m_EditorAPI, m_Model, m_MaybeSourceVisualizerEvent, *maybePoint);
            }
            else if (auto const* maybeFrame = dynamic_cast<OpenSim::Frame const*>(maybeComponent))
            {
                DrawRightClickedFrameContextMenu(*m_EditorAPI, m_Model, m_MaybeSourceVisualizerEvent, *maybeFrame);
            }
            else if (auto const* maybeP2PEdge = dynamic_cast<PointToPointEdge const*>(maybeComponent))
            {
                DrawRightClickedPointToPointEdgeContextMenu(*m_EditorAPI, m_Model, m_MaybeSourceVisualizerEvent, *maybeP2PEdge);
            }
            else if (auto const* maybeCPEdge = dynamic_cast<CrossProductEdge const*>(maybeComponent))
            {
                DrawRightClickedCrossProductEdgeContextMenu(*m_EditorAPI, m_Model, m_MaybeSourceVisualizerEvent, *maybeCPEdge);
            }
            else
            {
                DrawRightClickedUnknownComponentContextMenu(*m_EditorAPI, m_Model, m_MaybeSourceVisualizerEvent, *maybeComponent);
            }
        }

        IEditorAPI* m_EditorAPI;
        std::shared_ptr<UndoableModelStatePair> m_Model;
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
            ParentPtr<ITabHost> tabHost_,
            std::shared_ptr<UndoableModelStatePair> model_,
            std::shared_ptr<PanelManager> panelManager_) :

            m_TabHost{std::move(tabHost_)},
            m_Model{std::move(model_)},
            m_WindowMenu{std::move(panelManager_)}
        {
        }

        void onDraw()
        {
            drawEditMenu();
            m_WindowMenu.onDraw();
            m_AboutMenu.onDraw();
        }

    private:
        void drawEditMenu()
        {
            if (ui::BeginMenu("Edit"))
            {
                if (ui::MenuItem(ICON_FA_UNDO " Undo", {}, false, m_Model->canUndo()))
                {
                    ActionUndoCurrentlyEditedModel(*m_Model);
                }

                if (ui::MenuItem(ICON_FA_REDO " Redo", {}, false, m_Model->canRedo()))
                {
                    ActionRedoCurrentlyEditedModel(*m_Model);
                }
                ui::EndMenu();
            }
        }

        ParentPtr<ITabHost> m_TabHost;
        std::shared_ptr<UndoableModelStatePair> m_Model;
        WindowMenu m_WindowMenu;
        MainMenuAboutTab m_AboutMenu;
    };
}

class osc::FrameDefinitionTab::Impl final : public IEditorAPI {
public:

    explicit Impl(ParentPtr<ITabHost> const& parent_) :
        m_Parent{parent_}
    {
        m_PanelManager->registerToggleablePanel(
            "Navigator",
            [this](std::string_view panelName)
            {
                return std::make_shared<NavigatorPanel>(
                    panelName,
                    m_Model,
                    [this](OpenSim::ComponentPath const& rightClickedPath)
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
        m_PanelManager->registerToggleablePanel(
            "Properties",
            [this](std::string_view panelName)
            {
                return std::make_shared<PropertiesPanel>(panelName, this, m_Model);
            }
        );
        m_PanelManager->registerToggleablePanel(
            "Log",
            [](std::string_view panelName)
            {
                return std::make_shared<LogViewerPanel>(panelName);
            }
        );
        m_PanelManager->registerToggleablePanel(
            "Performance",
            [](std::string_view panelName)
            {
                return std::make_shared<PerfPanel>(panelName);
            }
        );
        m_PanelManager->registerSpawnablePanel(
            "framedef_viewer",
            [this](std::string_view panelName)
            {
                ModelEditorViewerPanelParameters panelParams
                {
                    m_Model,
                    [this](ModelEditorViewerPanelRightClickEvent const& e)
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

    UID getID() const
    {
        return m_TabID;
    }

    CStringView getName() const
    {
        return c_TabStringID;
    }

    void onMount()
    {
        App::upd().makeMainEventLoopWaiting();
        m_PanelManager->onMount();
        m_PopupManager.onMount();
    }

    void onUnmount()
    {
        m_PanelManager->onUnmount();
        App::upd().makeMainEventLoopPolling();
    }

    bool onEvent(SDL_Event const& e)
    {
        if (e.type == SDL_KEYDOWN)
        {
            return onKeydownEvent(e.key);
        }
        else
        {
            return false;
        }
    }

    void onTick()
    {
        m_PanelManager->onTick();
    }

    void onDrawMainMenu()
    {
        m_MainMenu.onDraw();
    }

    void onDraw()
    {
        ui::DockSpaceOverViewport(
            ui::GetMainViewport(),
            ImGuiDockNodeFlags_PassthruCentralNode
        );
        m_Toolbar.onDraw();
        m_PanelManager->onDraw();
        m_PopupManager.onDraw();
    }

private:
    bool onKeydownEvent(SDL_KeyboardEvent const& e)
    {
        bool const ctrlOrSuperDown = ui::IsCtrlOrSuperDown();

        if (ctrlOrSuperDown && e.keysym.mod & KMOD_SHIFT && e.keysym.sym == SDLK_z)
        {
            // Ctrl+Shift+Z: redo
            ActionRedoCurrentlyEditedModel(*m_Model);
            return true;
        }
        else if (ctrlOrSuperDown && e.keysym.sym == SDLK_z)
        {
            // Ctrl+Z: undo
            ActionUndoCurrentlyEditedModel(*m_Model);
            return true;
        }
        else if (e.keysym.sym == SDLK_BACKSPACE || e.keysym.sym == SDLK_DELETE)
        {
            // BACKSPACE/DELETE: delete selection
            ActionTryDeleteSelectionFromEditedModel(*m_Model);
            return true;
        }
        else
        {
            return false;
        }
    }

    void implPushComponentContextMenuPopup(OpenSim::ComponentPath const& componentPath) final
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

    void implAddMusclePlot(OpenSim::Coordinate const&, OpenSim::Muscle const&) final
    {
        // ignore: not applicable in this tab
    }

    std::shared_ptr<PanelManager> implGetPanelManager() final
    {
        return m_PanelManager;
    }

    UID m_TabID;
    ParentPtr<ITabHost> m_Parent;

    std::shared_ptr<UndoableModelStatePair> m_Model = MakeSharedUndoableFrameDefinitionModel();
    std::shared_ptr<PanelManager> m_PanelManager = std::make_shared<PanelManager>();
    PopupManager m_PopupManager;
    FrameDefinitionTabMainMenu m_MainMenu{m_Parent, m_Model, m_PanelManager};
    FrameDefinitionTabToolbar m_Toolbar{"##FrameDefinitionToolbar", m_Parent, m_Model};
};


// public API (PIMPL)

CStringView osc::FrameDefinitionTab::id()
{
    return c_TabStringID;
}

osc::FrameDefinitionTab::FrameDefinitionTab(ParentPtr<ITabHost> const& parent_) :
    m_Impl{std::make_unique<Impl>(parent_)}
{
}

osc::FrameDefinitionTab::FrameDefinitionTab(FrameDefinitionTab&&) noexcept = default;
osc::FrameDefinitionTab& osc::FrameDefinitionTab::operator=(FrameDefinitionTab&&) noexcept = default;
osc::FrameDefinitionTab::~FrameDefinitionTab() noexcept = default;

UID osc::FrameDefinitionTab::implGetID() const
{
    return m_Impl->getID();
}

CStringView osc::FrameDefinitionTab::implGetName() const
{
    return m_Impl->getName();
}

void osc::FrameDefinitionTab::implOnMount()
{
    m_Impl->onMount();
}

void osc::FrameDefinitionTab::implOnUnmount()
{
    m_Impl->onUnmount();
}

bool osc::FrameDefinitionTab::implOnEvent(SDL_Event const& e)
{
    return m_Impl->onEvent(e);
}

void osc::FrameDefinitionTab::implOnTick()
{
    m_Impl->onTick();
}

void osc::FrameDefinitionTab::implOnDrawMainMenu()
{
    m_Impl->onDrawMainMenu();
}

void osc::FrameDefinitionTab::implOnDraw()
{
    m_Impl->onDraw();
}
