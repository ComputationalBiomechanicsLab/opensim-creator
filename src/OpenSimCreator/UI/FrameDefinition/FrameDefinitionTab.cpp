#include "FrameDefinitionTab.hpp"

#include <OpenSimCreator/Documents/FrameDefinition/AxisIndex.hpp>
#include <OpenSimCreator/Documents/FrameDefinition/CrossProductDefinedFrame.hpp>
#include <OpenSimCreator/Documents/FrameDefinition/CrossProductEdge.hpp>
#include <OpenSimCreator/Documents/FrameDefinition/Edge.hpp>
#include <OpenSimCreator/Documents/FrameDefinition/EdgePoints.hpp>
#include <OpenSimCreator/Documents/FrameDefinition/FrameDefinitionActions.hpp>
#include <OpenSimCreator/Documents/FrameDefinition/FrameDefinitionHelpers.hpp>
#include <OpenSimCreator/Documents/FrameDefinition/MaybeNegatedAxis.hpp>
#include <OpenSimCreator/Documents/FrameDefinition/MidpointLandmark.hpp>
#include <OpenSimCreator/Documents/FrameDefinition/PointToPointEdge.hpp>
#include <OpenSimCreator/Documents/FrameDefinition/SphereLandmark.hpp>
#include <OpenSimCreator/Documents/Model/UndoableModelActions.hpp>
#include <OpenSimCreator/Documents/Model/UndoableModelStatePair.hpp>
#include <OpenSimCreator/Graphics/CustomRenderingOptions.hpp>
#include <OpenSimCreator/Graphics/OpenSimDecorationOptions.hpp>
#include <OpenSimCreator/Graphics/OpenSimDecorationGenerator.hpp>
#include <OpenSimCreator/Graphics/OverlayDecorationGenerator.hpp>
#include <OpenSimCreator/Graphics/OpenSimGraphicsHelpers.hpp>
#include <OpenSimCreator/Graphics/SimTKMeshLoader.hpp>
#include <OpenSimCreator/UI/FrameDefinition/FrameDefinitionTabToolbar.hpp>
#include <OpenSimCreator/UI/FrameDefinition/FrameDefinitionUIHelpers.hpp>
#include <OpenSimCreator/UI/ModelEditor/IEditorAPI.hpp>
#include <OpenSimCreator/UI/ModelEditor/ModelEditorTab.hpp>
#include <OpenSimCreator/UI/Shared/BasicWidgets.hpp>
#include <OpenSimCreator/UI/Shared/ChooseComponentsEditorLayer.hpp>
#include <OpenSimCreator/UI/Shared/ChooseComponentsEditorLayerParameters.hpp>
#include <OpenSimCreator/UI/Shared/MainMenu.hpp>
#include <OpenSimCreator/UI/Shared/ModelEditorViewerPanel.hpp>
#include <OpenSimCreator/UI/Shared/ModelEditorViewerPanelLayer.hpp>
#include <OpenSimCreator/UI/Shared/ModelEditorViewerPanelParameters.hpp>
#include <OpenSimCreator/UI/Shared/ModelEditorViewerPanelRightClickEvent.hpp>
#include <OpenSimCreator/UI/Shared/ModelEditorViewerPanelState.hpp>
#include <OpenSimCreator/UI/Shared/NavigatorPanel.hpp>
#include <OpenSimCreator/UI/Shared/PropertiesPanel.hpp>
#include <OpenSimCreator/UI/IMainUIStateAPI.hpp>
#include <OpenSimCreator/Utils/OpenSimHelpers.hpp>
#include <OpenSimCreator/Utils/SimTKHelpers.hpp>

#include <IconsFontAwesome5.h>
#include <imgui.h>
#include <OpenSim/Common/Component.h>
#include <OpenSim/Common/ComponentPath.h>
#include <OpenSim/Common/ComponentSocket.h>
#include <OpenSim/Simulation/SimbodyEngine/FreeJoint.h>
#include <OpenSim/Simulation/Model/Ground.h>
#include <OpenSim/Simulation/Model/Model.h>
#include <OpenSim/Simulation/Model/ModelComponent.h>
#include <OpenSim/Simulation/Model/PhysicalOffsetFrame.h>
#include <oscar/Formats/OBJ.hpp>
#include <oscar/Formats/STL.hpp>
#include <oscar/Graphics/Color.hpp>
#include <oscar/Graphics/Mesh.hpp>
#include <oscar/Graphics/ShaderCache.hpp>
#include <oscar/Maths/BVH.hpp>
#include <oscar/Maths/MathHelpers.hpp>
#include <oscar/Maths/Rect.hpp>
#include <oscar/Maths/Vec2.hpp>
#include <oscar/Maths/Vec3.hpp>
#include <oscar/Platform/App.hpp>
#include <oscar/Platform/AppMetadata.hpp>
#include <oscar/Platform/Log.hpp>
#include <oscar/Platform/os.hpp>
#include <oscar/Scene/SceneCache.hpp>
#include <oscar/Scene/SceneDecoration.hpp>
#include <oscar/Scene/SceneHelpers.hpp>
#include <oscar/Scene/SceneRenderer.hpp>
#include <oscar/Scene/SceneRendererParams.hpp>
#include <oscar/UI/Panels/IPanel.hpp>
#include <oscar/UI/Panels/LogViewerPanel.hpp>
#include <oscar/UI/Panels/PanelManager.hpp>
#include <oscar/UI/Panels/PerfPanel.hpp>
#include <oscar/UI/Panels/StandardPanelImpl.hpp>
#include <oscar/UI/Widgets/IPopup.hpp>
#include <oscar/UI/Widgets/PopupManager.hpp>
#include <oscar/UI/Widgets/StandardPopup.hpp>
#include <oscar/UI/Widgets/WindowMenu.hpp>
#include <oscar/UI/ImGuiHelpers.hpp>
#include <oscar/Utils/Assertions.hpp>
#include <oscar/Utils/CStringView.hpp>
#include <oscar/Utils/EnumHelpers.hpp>
#include <oscar/Utils/ParentPtr.hpp>
#include <oscar/Utils/SetHelpers.hpp>
#include <oscar/Utils/UID.hpp>
#include <SDL_events.h>
#include <SimTKcommon/internal/DecorativeGeometry.h>

#include <array>
#include <atomic>
#include <cstdint>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <functional>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_set>
#include <utility>
#include <vector>

using osc::ChooseComponentsEditorLayer;
using osc::ChooseComponentsEditorLayerParameters;
using osc::Vec2;
using osc::Vec3;
using namespace osc::fd;

// choose `n` components UI flow
namespace
{
    /////
    // layer pushing routines
    /////

    void PushCreateEdgeToOtherPointLayer(
        osc::IEditorAPI& editor,
        std::shared_ptr<osc::UndoableModelStatePair> const& model,
        OpenSim::Point const& point,
        osc::ModelEditorViewerPanelRightClickEvent const& sourceEvent)
    {
        auto* const visualizer = editor.getPanelManager()->tryUpdPanelByNameT<osc::ModelEditorViewerPanel>(sourceEvent.sourcePanelName);
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
                osc::log::error("user selections from the 'choose components' layer was empty: this bug should be reported");
                return false;
            }
            if (choices.size() > 1)
            {
                osc::log::warn("number of user selections from 'choose components' layer was greater than expected: this bug should be reported");
            }
            std::string const& pointBPath = *choices.begin();

            auto const* pointA = osc::FindComponent<OpenSim::Point>(model->getModel(), pointAPath);
            if (!pointA)
            {
                osc::log::error("point A's component path (%s) does not exist in the model", pointAPath.c_str());
                return false;
            }

            auto const* pointB = osc::FindComponent<OpenSim::Point>(model->getModel(), pointBPath);
            if (!pointB)
            {
                osc::log::error("point B's component path (%s) does not exist in the model", pointBPath.c_str());
                return false;
            }

            ActionAddPointToPointEdge(*model, *pointA, *pointB);
            return true;
        };

        visualizer->pushLayer(std::make_unique<ChooseComponentsEditorLayer>(model, options));
    }

    void PushCreateMidpointToAnotherPointLayer(
        osc::IEditorAPI& editor,
        std::shared_ptr<osc::UndoableModelStatePair> const& model,
        OpenSim::Point const& point,
        osc::ModelEditorViewerPanelRightClickEvent const& sourceEvent)
    {
        auto* const visualizer = editor.getPanelManager()->tryUpdPanelByNameT<osc::ModelEditorViewerPanel>(sourceEvent.sourcePanelName);
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
                osc::log::error("user selections from the 'choose components' layer was empty: this bug should be reported");
                return false;
            }
            if (choices.size() > 1)
            {
                osc::log::warn("number of user selections from 'choose components' layer was greater than expected: this bug should be reported");
            }
            std::string const& pointBPath = *choices.begin();

            auto const* pointA = osc::FindComponent<OpenSim::Point>(model->getModel(), pointAPath);
            if (!pointA)
            {
                osc::log::error("point A's component path (%s) does not exist in the model", pointAPath.c_str());
                return false;
            }

            auto const* pointB = osc::FindComponent<OpenSim::Point>(model->getModel(), pointBPath);
            if (!pointB)
            {
                osc::log::error("point B's component path (%s) does not exist in the model", pointBPath.c_str());
                return false;
            }

            ActionAddMidpoint(*model, *pointA, *pointB);
            return true;
        };

        visualizer->pushLayer(std::make_unique<ChooseComponentsEditorLayer>(model, options));
    }

    void PushCreateCrossProductEdgeLayer(
        osc::IEditorAPI& editor,
        std::shared_ptr<osc::UndoableModelStatePair> const& model,
        Edge const& firstEdge,
        osc::ModelEditorViewerPanelRightClickEvent const& sourceEvent)
    {
        auto* const visualizer = editor.getPanelManager()->tryUpdPanelByNameT<osc::ModelEditorViewerPanel>(sourceEvent.sourcePanelName);
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
                osc::log::error("user selections from the 'choose components' layer was empty: this bug should be reported");
                return false;
            }
            if (choices.size() > 1)
            {
                osc::log::warn("number of user selections from 'choose components' layer was greater than expected: this bug should be reported");
            }
            std::string const& edgeBPath = *choices.begin();

            auto const* edgeA = osc::FindComponent<Edge>(model->getModel(), edgeAPath);
            if (!edgeA)
            {
                osc::log::error("edge A's component path (%s) does not exist in the model", edgeAPath.c_str());
                return false;
            }

            auto const* edgeB = osc::FindComponent<Edge>(model->getModel(), edgeBPath);
            if (!edgeB)
            {
                osc::log::error("point B's component path (%s) does not exist in the model", edgeBPath.c_str());
                return false;
            }

            ActionAddCrossProductEdge(*model, *edgeA, *edgeB);
            return true;
        };

        visualizer->pushLayer(std::make_unique<ChooseComponentsEditorLayer>(model, options));
    }

    void PushPickOriginForFrameDefinitionLayer(
        osc::ModelEditorViewerPanel& visualizer,
        std::shared_ptr<osc::UndoableModelStatePair> const& model,
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
                osc::log::error("user selections from the 'choose components' layer was empty: this bug should be reported");
                return false;
            }
            if (choices.size() > 1)
            {
                osc::log::warn("number of user selections from 'choose components' layer was greater than expected: this bug should be reported");
            }
            std::string const& originPath = *choices.begin();

            auto const* firstEdge = osc::FindComponent<Edge>(model->getModel(), firstEdgeAbsPath);
            if (!firstEdge)
            {
                osc::log::error("the first edge's component path (%s) does not exist in the model", firstEdgeAbsPath.c_str());
                return false;
            }

            auto const* otherEdge = osc::FindComponent<Edge>(model->getModel(), secondEdgeAbsPath);
            if (!otherEdge)
            {
                osc::log::error("the second edge's component path (%s) does not exist in the model", secondEdgeAbsPath.c_str());
                return false;
            }

            auto const* originPoint = osc::FindComponent<OpenSim::Point>(model->getModel(), originPath);
            if (!originPoint)
            {
                osc::log::error("the origin's component path (%s) does not exist in the model", originPath.c_str());
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
        osc::ModelEditorViewerPanel& visualizer,
        std::shared_ptr<osc::UndoableModelStatePair> const& model,
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
                osc::log::error("user selections from the 'choose components' layer was empty: this bug should be reported");
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
        osc::IEditorAPI& editor,
        std::shared_ptr<osc::UndoableModelStatePair> const& model,
        Edge const& firstEdge,
        MaybeNegatedAxis firstEdgeAxis,
        std::optional<osc::ModelEditorViewerPanelRightClickEvent> const& maybeSourceEvent)
    {
        if (!maybeSourceEvent)
        {
            return;  // there is no way to figure out which visualizer to push the layer to
        }

        auto* const visualizer = editor.getPanelManager()->tryUpdPanelByNameT<osc::ModelEditorViewerPanel>(maybeSourceEvent->sourcePanelName);
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
        osc::ModelEditorViewerPanel& visualizer,
        std::shared_ptr<osc::UndoableModelStatePair> const& model,
        OpenSim::ComponentPath const& frameAbsPath,
        OpenSim::ComponentPath const& meshAbsPath,
        OpenSim::ComponentPath const& jointFrameAbsPath)
    {
        ChooseComponentsEditorLayerParameters options;
        options.popupHeaderText = "choose parent frame";
        options.canChooseItem = [bodyFrame = osc::FindComponent(model->getModel(), frameAbsPath)](OpenSim::Component const& c)
        {
            return
                IsPhysicalFrame(c) &&
                &c != bodyFrame &&
                !osc::IsChildOfA<OpenSim::ComponentSet>(c) &&
                (
                    dynamic_cast<OpenSim::Ground const*>(&c) != nullptr ||
                    osc::IsChildOfA<OpenSim::BodySet>(c)
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
                osc::log::error("user selections from the 'choose components' layer was empty: this bug should be reported");
                return false;
            }

            auto const* const parentFrame = osc::FindComponent<OpenSim::PhysicalFrame>(model->getModel(), *choices.begin());
            if (!parentFrame)
            {
                osc::log::error("user selection from 'choose components' layer did not select a frame: this shouldn't happen?");
                return false;
            }

            ActionCreateBodyFromFrame(
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
        osc::ModelEditorViewerPanel& visualizer,
        std::shared_ptr<osc::UndoableModelStatePair> const& model,
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
                osc::log::error("user selections from the 'choose components' layer was empty: this bug should be reported");
                return false;
            }

            auto const* const jointFrame = osc::FindComponent<OpenSim::Frame>(model->getModel(), *choices.begin());
            if (!jointFrame)
            {
                osc::log::error("user selection from 'choose components' layer did not select a frame: this shouldn't happen?");
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
        osc::ModelEditorViewerPanel& visualizer,
        std::shared_ptr<osc::UndoableModelStatePair> const& model,
        OpenSim::Frame const& frame)
    {
        ChooseComponentsEditorLayerParameters options;
        options.popupHeaderText = "choose mesh to attach the body to";
        options.canChooseItem = [](OpenSim::Component const& c) { return IsMesh(c) && !osc::IsChildOfA<OpenSim::Body>(c); };
        options.numComponentsUserMustChoose = 1;
        options.onUserFinishedChoosing = [
            visualizerPtr = &visualizer,  // TODO: implement weak_ptr for panel lookup
            model,
            frameAbsPath = frame.getAbsolutePath()
        ](std::unordered_set<std::string> const& choices) -> bool
        {
            if (choices.empty())
            {
                osc::log::error("user selections from the 'choose components' layer was empty: this bug should be reported");
                return false;
            }

            auto const* const mesh = osc::FindComponent<OpenSim::Mesh>(model->getModel(), *choices.begin());
            if (!mesh)
            {
                osc::log::error("user selection from 'choose components' layer did not select a mesh: this shouldn't happen?");
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
        osc::IEditorAPI& editor,
        std::shared_ptr<osc::UndoableModelStatePair> const& model,
        std::optional<osc::ModelEditorViewerPanelRightClickEvent> const& maybeSourceEvent,
        OpenSim::Frame const& frame)
    {
        if (!maybeSourceEvent)
        {
            return;  // there is no way to figure out which visualizer to push the layer to
        }

        auto* const visualizer = editor.getPanelManager()->tryUpdPanelByNameT<osc::ModelEditorViewerPanel>(maybeSourceEvent->sourcePanelName);
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
        if (ImGui::BeginMenu(ICON_FA_CALCULATOR " Calculate"))
        {
            if (ImGui::BeginMenu("Start Point"))
            {
                auto const onFrameMenuOpened = [&state, &edge](OpenSim::Frame const& frame)
                {
                    osc::DrawPointTranslationInformationWithRespectTo(
                        frame,
                        state,
                        osc::ToVec3(edge.getStartLocationInGround(state))
                    );
                };
                osc::DrawWithRespectToMenuContainingMenuPerFrame(root, onFrameMenuOpened);
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("End Point"))
            {
                auto const onFrameMenuOpened = [&state, &edge](OpenSim::Frame const& frame)
                {
                    osc::DrawPointTranslationInformationWithRespectTo(
                        frame,
                        state,
                        osc::ToVec3(edge.getEndLocationInGround(state))
                    );
                };

                osc::DrawWithRespectToMenuContainingMenuPerFrame(root, onFrameMenuOpened);
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Direction"))
            {
                auto const onFrameMenuOpened = [&state, &edge](OpenSim::Frame const& frame)
                {
                    osc::DrawDirectionInformationWithRepsectTo(
                        frame,
                        state,
                        osc::ToVec3(CalcDirection(edge.getLocationsInGround(state)))
                    );
                };

                osc::DrawWithRespectToMenuContainingMenuPerFrame(root, onFrameMenuOpened);
                ImGui::EndMenu();
            }

            ImGui::EndMenu();
        }
    }

    void DrawFocusCameraMenu(
        osc::IEditorAPI& editor,
        std::shared_ptr<osc::UndoableModelStatePair> const&,
        std::optional<osc::ModelEditorViewerPanelRightClickEvent> const& maybeSourceEvent,
        OpenSim::Component const&)
    {
        if (maybeSourceEvent && ImGui::BeginMenu(ICON_FA_CAMERA " Focus Camera"))
        {
            if (ImGui::MenuItem("on Ground"))
            {
                auto* visualizer = editor.getPanelManager()->tryUpdPanelByNameT<osc::ModelEditorViewerPanel>(maybeSourceEvent->sourcePanelName);
                if (visualizer)
                {
                    visualizer->focusOn({});
                }
            }

            if (maybeSourceEvent->maybeClickPositionInGround &&
                ImGui::MenuItem("on Click Position"))
            {
                auto* visualizer = editor.getPanelManager()->tryUpdPanelByNameT<osc::ModelEditorViewerPanel>(maybeSourceEvent->sourcePanelName);
                if (visualizer)
                {
                    visualizer->focusOn(*maybeSourceEvent->maybeClickPositionInGround);
                }
            }

            ImGui::EndMenu();
        }
    }

    void DrawEdgeAddContextMenuItems(
        osc::IEditorAPI& editor,
        std::shared_ptr<osc::UndoableModelStatePair> const& model,
        std::optional<osc::ModelEditorViewerPanelRightClickEvent> const& maybeSourceEvent,
        Edge const& edge)
    {
        if (maybeSourceEvent && ImGui::MenuItem(ICON_FA_TIMES " Cross Product Edge"))
        {
            PushCreateCrossProductEdgeLayer(editor, model, edge, *maybeSourceEvent);
        }

        if (maybeSourceEvent && ImGui::BeginMenu(ICON_FA_ARROWS_ALT " Frame With This Edge as"))
        {
            osc::PushStyleColor(ImGuiCol_Text, osc::Color::mutedRed());
            if (ImGui::MenuItem("+x"))
            {
                ActionPushCreateFrameLayer(
                    editor,
                    model,
                    edge,
                    MaybeNegatedAxis{AxisIndex::X, false},
                    maybeSourceEvent
                );
            }
            osc::PopStyleColor();

            osc::PushStyleColor(ImGuiCol_Text, osc::Color::mutedGreen());
            if (ImGui::MenuItem("+y"))
            {
                ActionPushCreateFrameLayer(
                    editor,
                    model,
                    edge,
                    MaybeNegatedAxis{AxisIndex::Y, false},
                    maybeSourceEvent
                );
            }
            osc::PopStyleColor();

            osc::PushStyleColor(ImGuiCol_Text, osc::Color::mutedBlue());
            if (ImGui::MenuItem("+z"))
            {
                ActionPushCreateFrameLayer(
                    editor,
                    model,
                    edge,
                    MaybeNegatedAxis{AxisIndex::Z, false},
                    maybeSourceEvent
                );
            }
            osc::PopStyleColor();

            ImGui::Separator();

            osc::PushStyleColor(ImGuiCol_Text, osc::Color::mutedRed());
            if (ImGui::MenuItem("-x"))
            {
                ActionPushCreateFrameLayer(
                    editor,
                    model,
                    edge,
                    MaybeNegatedAxis{AxisIndex::X, true},
                    maybeSourceEvent
                );
            }
            osc::PopStyleColor();

            osc::PushStyleColor(ImGuiCol_Text, osc::Color::mutedGreen());
            if (ImGui::MenuItem("-y"))
            {
                ActionPushCreateFrameLayer(
                    editor,
                    model,
                    edge,
                    MaybeNegatedAxis{AxisIndex::Y, true},
                    maybeSourceEvent
                );
            }
            osc::PopStyleColor();

            osc::PushStyleColor(ImGuiCol_Text, osc::Color::mutedBlue());
            if (ImGui::MenuItem("-z"))
            {
                ActionPushCreateFrameLayer(
                    editor,
                    model,
                    edge,
                    MaybeNegatedAxis{AxisIndex::Z, true},
                    maybeSourceEvent
                );
            }
            osc::PopStyleColor();

            ImGui::EndMenu();
        }
    }

    void DrawCreateBodyMenuItem(
        osc::IEditorAPI& editor,
        std::shared_ptr<osc::UndoableModelStatePair> const& model,
        std::optional<osc::ModelEditorViewerPanelRightClickEvent> const& maybeSourceEvent,
        OpenSim::Frame const& frame)
    {
        OpenSim::Component const* groundOrExistingBody = dynamic_cast<OpenSim::Ground const*>(&frame);
        if (!groundOrExistingBody)
        {
            groundOrExistingBody = osc::FindFirstDescendentOfType<OpenSim::Body>(frame);
        }

        if (ImGui::MenuItem(ICON_FA_WEIGHT " Body From This", nullptr, false, groundOrExistingBody == nullptr))
        {
            ActionCreateBodyFromFrame(editor, model, maybeSourceEvent, frame);
        }
        if (groundOrExistingBody && ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
        {
            std::stringstream ss;
            ss << "Cannot create a body from this frame: it is already the frame of " << groundOrExistingBody->getName();
            osc::DrawTooltipBodyOnly(std::move(ss).str());
        }
    }
    void DrawMeshAddContextMenuItems(
        std::shared_ptr<osc::UndoableModelStatePair> const& model,
        std::optional<osc::ModelEditorViewerPanelRightClickEvent> const& maybeSourceEvent,
        OpenSim::Mesh const& mesh)
    {
        if (ImGui::MenuItem(ICON_FA_CIRCLE " Sphere Landmark"))
        {
            ActionAddSphereInMeshFrame(
                *model,
                mesh,
                maybeSourceEvent ? maybeSourceEvent->maybeClickPositionInGround : std::nullopt
            );
        }
        if (ImGui::MenuItem(ICON_FA_ARROWS_ALT " Custom (Offset) Frame"))
        {
            ActionAddOffsetFrameInMeshFrame(
                *model,
                mesh,
                maybeSourceEvent ? maybeSourceEvent->maybeClickPositionInGround : std::nullopt
            );
        }
    }

    void DrawPointAddContextMenuItems(
        osc::IEditorAPI& editor,
        std::shared_ptr<osc::UndoableModelStatePair> const& model,
        std::optional<osc::ModelEditorViewerPanelRightClickEvent> const& maybeSourceEvent,
        OpenSim::Point const& point)
    {
        if (maybeSourceEvent && ImGui::MenuItem(ICON_FA_GRIP_LINES " Edge"))
        {
            PushCreateEdgeToOtherPointLayer(editor, model, point, *maybeSourceEvent);
        }
        if (maybeSourceEvent && ImGui::MenuItem(ICON_FA_DOT_CIRCLE " Midpoint"))
        {
            PushCreateMidpointToAnotherPointLayer(editor, model, point, *maybeSourceEvent);
        }
    }

    void DrawRightClickedNothingContextMenu(
        osc::UndoableModelStatePair& model)
    {
        osc::DrawNothingRightClickedContextMenuHeader();
        osc::DrawContextMenuSeparator();

        if (ImGui::BeginMenu(ICON_FA_PLUS " Add"))
        {
            if (ImGui::MenuItem(ICON_FA_CUBES " Meshes"))
            {
                ActionPromptUserToAddMeshFiles(model);
            }
            ImGui::EndMenu();
        }
    }

    void DrawRightClickedMeshContextMenu(
        osc::IEditorAPI& editor,
        std::shared_ptr<osc::UndoableModelStatePair> const& model,
        std::optional<osc::ModelEditorViewerPanelRightClickEvent> const& maybeSourceEvent,
        OpenSim::Mesh const& mesh)
    {
        osc::DrawRightClickedComponentContextMenuHeader(mesh);
        osc::DrawContextMenuSeparator();

        if (ImGui::BeginMenu(ICON_FA_PLUS " Add"))
        {
            DrawMeshAddContextMenuItems(model, maybeSourceEvent, mesh);
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu(ICON_FA_FILE_EXPORT " Export"))
        {
            DrawMeshExportContextMenuContent(*model, mesh);
            ImGui::EndMenu();
        }
        DrawFocusCameraMenu(editor, model, maybeSourceEvent, mesh);
    }

    void DrawRightClickedPointContextMenu(
        osc::IEditorAPI& editor,
        std::shared_ptr<osc::UndoableModelStatePair> const& model,
        std::optional<osc::ModelEditorViewerPanelRightClickEvent> const& maybeSourceEvent,
        OpenSim::Point const& point)
    {
        osc::DrawRightClickedComponentContextMenuHeader(point);
        osc::DrawContextMenuSeparator();

        if (ImGui::BeginMenu(ICON_FA_PLUS " Add"))
        {
            DrawPointAddContextMenuItems(editor, model, maybeSourceEvent, point);
            ImGui::EndMenu();
        }
        osc::DrawCalculateMenu(model->getModel(), model->getState(), point);
        DrawFocusCameraMenu(editor, model, maybeSourceEvent, point);
    }

    void DrawRightClickedPointToPointEdgeContextMenu(
        osc::IEditorAPI& editor,
        std::shared_ptr<osc::UndoableModelStatePair> const& model,
        std::optional<osc::ModelEditorViewerPanelRightClickEvent> const& maybeSourceEvent,
        PointToPointEdge const& edge)
    {
        osc::DrawRightClickedComponentContextMenuHeader(edge);
        osc::DrawContextMenuSeparator();

        if (ImGui::BeginMenu(ICON_FA_PLUS " Add"))
        {
            DrawEdgeAddContextMenuItems(editor, model, maybeSourceEvent, edge);
            ImGui::EndMenu();
        }
        if (ImGui::MenuItem(ICON_FA_RECYCLE " Swap Direction"))
        {
            ActionSwapPointToPointEdgeEnds(*model, edge);
        }
        DrawCalculateMenu(model->getModel(), model->getState(), edge);
        DrawFocusCameraMenu(editor, model, maybeSourceEvent, edge);
    }

    void DrawRightClickedCrossProductEdgeContextMenu(
        osc::IEditorAPI& editor,
        std::shared_ptr<osc::UndoableModelStatePair> const& model,
        std::optional<osc::ModelEditorViewerPanelRightClickEvent> const& maybeSourceEvent,
        CrossProductEdge const& edge)
    {
        osc::DrawRightClickedComponentContextMenuHeader(edge);
        osc::DrawContextMenuSeparator();

        if (ImGui::BeginMenu(ICON_FA_PLUS " Add"))
        {
            DrawEdgeAddContextMenuItems(editor, model, maybeSourceEvent, edge);
            ImGui::EndMenu();
        }
        if (ImGui::MenuItem(ICON_FA_RECYCLE " Swap Operands"))
        {
            ActionSwapCrossProductEdgeOperands(*model, edge);
        }
        DrawCalculateMenu(model->getModel(), model->getState(), edge);
        DrawFocusCameraMenu(editor, model, maybeSourceEvent, edge);
    }

    void DrawRightClickedFrameContextMenu(
        osc::IEditorAPI& editor,
        std::shared_ptr<osc::UndoableModelStatePair> const& model,
        std::optional<osc::ModelEditorViewerPanelRightClickEvent> const& maybeSourceEvent,
        OpenSim::Frame const& frame)
    {
        osc::DrawRightClickedComponentContextMenuHeader(frame);
        osc::DrawContextMenuSeparator();

        if (ImGui::BeginMenu(ICON_FA_PLUS " Add"))
        {
            DrawCreateBodyMenuItem(editor, model, maybeSourceEvent, frame);
            ImGui::EndMenu();
        }
        osc::DrawCalculateMenu(model->getModel(), model->getState(), frame);
        DrawFocusCameraMenu(editor, model, maybeSourceEvent, frame);
    }

    void DrawRightClickedUnknownComponentContextMenu(
        osc::IEditorAPI& editor,
        std::shared_ptr<osc::UndoableModelStatePair> const& model,
        std::optional<osc::ModelEditorViewerPanelRightClickEvent> const& maybeSourceEvent,
        OpenSim::Component const& component)
    {
        osc::DrawRightClickedComponentContextMenuHeader(component);
        osc::DrawContextMenuSeparator();

        DrawFocusCameraMenu(editor, model, maybeSourceEvent, component);
    }

    // popup state for the frame definition tab's general context menu
    class FrameDefinitionContextMenu final : public osc::StandardPopup {
    public:
        FrameDefinitionContextMenu(
            std::string_view popupName_,
            osc::IEditorAPI* editorAPI_,
            std::shared_ptr<osc::UndoableModelStatePair> model_,
            OpenSim::ComponentPath componentPath_,
            std::optional<osc::ModelEditorViewerPanelRightClickEvent> maybeSourceVisualizerEvent_ = std::nullopt) :

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
            OpenSim::Component const* const maybeComponent = osc::FindComponent(m_Model->getModel(), m_ComponentPath);
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

        osc::IEditorAPI* m_EditorAPI;
        std::shared_ptr<osc::UndoableModelStatePair> m_Model;
        OpenSim::ComponentPath m_ComponentPath;
        std::optional<osc::ModelEditorViewerPanelRightClickEvent> m_MaybeSourceVisualizerEvent;
    };
}

// other panels/widgets
namespace
{
    class FrameDefinitionTabMainMenu final {
    public:
        explicit FrameDefinitionTabMainMenu(
            osc::ParentPtr<osc::ITabHost> tabHost_,
            std::shared_ptr<osc::UndoableModelStatePair> model_,
            std::shared_ptr<osc::PanelManager> panelManager_) :

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
            if (ImGui::BeginMenu("Edit"))
            {
                if (ImGui::MenuItem(ICON_FA_UNDO " Undo", nullptr, false, m_Model->canUndo()))
                {
                    osc::ActionUndoCurrentlyEditedModel(*m_Model);
                }

                if (ImGui::MenuItem(ICON_FA_REDO " Redo", nullptr, false, m_Model->canRedo()))
                {
                    osc::ActionRedoCurrentlyEditedModel(*m_Model);
                }
                ImGui::EndMenu();
            }
        }

        osc::ParentPtr<osc::ITabHost> m_TabHost;
        std::shared_ptr<osc::UndoableModelStatePair> m_Model;
        osc::WindowMenu m_WindowMenu;
        osc::MainMenuAboutTab m_AboutMenu;
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
        ImGui::DockSpaceOverViewport(
            ImGui::GetMainViewport(),
            ImGuiDockNodeFlags_PassthruCentralNode
        );
        m_Toolbar.onDraw();
        m_PanelManager->onDraw();
        m_PopupManager.onDraw();
    }

private:
    bool onKeydownEvent(SDL_KeyboardEvent const& e)
    {
        bool const ctrlOrSuperDown = osc::IsCtrlOrSuperDown();

        if (ctrlOrSuperDown && e.keysym.mod & KMOD_SHIFT && e.keysym.sym == SDLK_z)
        {
            // Ctrl+Shift+Z: redo
            osc::ActionRedoCurrentlyEditedModel(*m_Model);
            return true;
        }
        else if (ctrlOrSuperDown && e.keysym.sym == SDLK_z)
        {
            // Ctrl+Z: undo
            osc::ActionUndoCurrentlyEditedModel(*m_Model);
            return true;
        }
        else if (e.keysym.sym == SDLK_BACKSPACE || e.keysym.sym == SDLK_DELETE)
        {
            // BACKSPACE/DELETE: delete selection
            osc::ActionTryDeleteSelectionFromEditedModel(*m_Model);
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

osc::CStringView osc::FrameDefinitionTab::id()
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

osc::UID osc::FrameDefinitionTab::implGetID() const
{
    return m_Impl->getID();
}

osc::CStringView osc::FrameDefinitionTab::implGetName() const
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
