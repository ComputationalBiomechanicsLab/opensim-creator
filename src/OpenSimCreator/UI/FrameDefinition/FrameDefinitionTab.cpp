#include "FrameDefinitionTab.hpp"

#include <OpenSimCreator/Documents/FrameDefinition/AxisIndex.hpp>
#include <OpenSimCreator/Documents/FrameDefinition/EdgePoints.hpp>
#include <OpenSimCreator/Documents/FrameDefinition/FDCrossProductEdge.hpp>
#include <OpenSimCreator/Documents/FrameDefinition/FDPointToPointEdge.hpp>
#include <OpenSimCreator/Documents/FrameDefinition/FDVirtualEdge.hpp>
#include <OpenSimCreator/Documents/FrameDefinition/FrameDefinitionHelpers.hpp>
#include <OpenSimCreator/Documents/FrameDefinition/LandmarkDefinedFrame.hpp>
#include <OpenSimCreator/Documents/FrameDefinition/MaybeNegatedAxis.hpp>
#include <OpenSimCreator/Documents/FrameDefinition/MidpointLandmark.hpp>
#include <OpenSimCreator/Documents/FrameDefinition/SphereLandmark.hpp>
#include <OpenSimCreator/Documents/Model/UndoableModelActions.hpp>
#include <OpenSimCreator/Documents/Model/UndoableModelStatePair.hpp>
#include <OpenSimCreator/Graphics/CustomRenderingOptions.hpp>
#include <OpenSimCreator/Graphics/OpenSimDecorationOptions.hpp>
#include <OpenSimCreator/Graphics/OpenSimDecorationGenerator.hpp>
#include <OpenSimCreator/Graphics/OverlayDecorationGenerator.hpp>
#include <OpenSimCreator/Graphics/OpenSimGraphicsHelpers.hpp>
#include <OpenSimCreator/Graphics/SimTKMeshLoader.hpp>
#include <OpenSimCreator/UI/ModelEditor/IEditorAPI.hpp>
#include <OpenSimCreator/UI/ModelEditor/ModelEditorTab.hpp>
#include <OpenSimCreator/UI/Shared/BasicWidgets.hpp>
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

using osc::Vec2;
using osc::Vec3;
using namespace osc::fd;

// general (not layer-system-dependent) user-enactable actions
namespace
{
    void ActionPromptUserToAddMeshFiles(osc::UndoableModelStatePair& model)
    {
        std::vector<std::filesystem::path> const meshPaths =
            osc::PromptUserForFiles(osc::GetCommaDelimitedListOfSupportedSimTKMeshFormats());
        if (meshPaths.empty())
        {
            return;  // user didn't select anything
        }

        // create a human-readable commit message
        std::string const commitMessage = [&meshPaths]()
        {
            if (meshPaths.size() == 1)
            {
                return GenerateAddedSomethingCommitMessage(meshPaths.front().filename().string());
            }
            else
            {
                std::stringstream ss;
                ss << "added " << meshPaths.size() << " meshes";
                return std::move(ss).str();
            }
        }();

        // perform the model mutation
        OpenSim::Model& mutableModel = model.updModel();
        for (std::filesystem::path const& meshPath : meshPaths)
        {
            std::string const meshName = meshPath.filename().replace_extension().string();

            // add an offset frame that is connected to ground - this will become
            // the mesh's offset frame
            auto meshPhysicalOffsetFrame = std::make_unique<OpenSim::PhysicalOffsetFrame>();
            meshPhysicalOffsetFrame->setParentFrame(model.getModel().getGround());
            meshPhysicalOffsetFrame->setName(meshName + "_offset");

            // attach the mesh to the frame
            {
                auto mesh = std::make_unique<OpenSim::Mesh>(meshPath.string());
                mesh->setName(meshName);
                osc::AttachGeometry(*meshPhysicalOffsetFrame, std::move(mesh));
            }

            // add it to the model and select it (i.e. always select the last mesh)
            OpenSim::PhysicalOffsetFrame const& pofRef = osc::AddModelComponent(mutableModel, std::move(meshPhysicalOffsetFrame));
            osc::FinalizeConnections(mutableModel);
            model.setSelected(&pofRef);
        }

        model.commit(commitMessage);
        osc::InitializeModel(mutableModel);
        osc::InitializeState(mutableModel);
    }

    void ActionAddSphereInMeshFrame(
        osc::UndoableModelStatePair& model,
        OpenSim::Mesh const& mesh,
        std::optional<Vec3> const& maybeClickPosInGround)
    {
        // if the caller requests a location via a click, set the position accordingly
        SimTK::Vec3 const locationInMeshFrame = maybeClickPosInGround ?
            CalcLocationInFrame(mesh.getFrame(), model.getState(), *maybeClickPosInGround) :
            SimTK::Vec3{0.0, 0.0, 0.0};

        std::string const sphereName = GenerateSceneElementName("sphere_");
        std::string const commitMessage = GenerateAddedSomethingCommitMessage(sphereName);

        // create sphere component
        std::unique_ptr<SphereLandmark> sphere = [&mesh, &locationInMeshFrame, &sphereName]()
        {
            auto rv = std::make_unique<SphereLandmark>();
            rv->setName(sphereName);
            rv->set_location(locationInMeshFrame);
            rv->connectSocket_parent_frame(mesh.getFrame());
            return rv;
        }();

        // perform the model mutation
        {
            OpenSim::Model& mutableModel = model.updModel();

            SphereLandmark const& sphereRef = osc::AddModelComponent(mutableModel, std::move(sphere));
            osc::FinalizeConnections(mutableModel);
            osc::InitializeModel(mutableModel);
            osc::InitializeState(mutableModel);
            model.setSelected(&sphereRef);
            model.commit(commitMessage);
        }
    }

    void ActionAddOffsetFrameInMeshFrame(
        osc::UndoableModelStatePair& model,
        OpenSim::Mesh const& mesh,
        std::optional<Vec3> const& maybeClickPosInGround)
    {
        // if the caller requests a location via a click, set the position accordingly
        SimTK::Vec3 const locationInMeshFrame = maybeClickPosInGround ?
            CalcLocationInFrame(mesh.getFrame(), model.getState(), *maybeClickPosInGround) :
            SimTK::Vec3{0.0, 0.0, 0.0};

        std::string const pofName = GenerateSceneElementName("pof_");
        std::string const commitMessage = GenerateAddedSomethingCommitMessage(pofName);

        // create physical offset frame
        std::unique_ptr<OpenSim::PhysicalOffsetFrame> pof = [&mesh, &locationInMeshFrame, &pofName]()
        {
            auto rv = std::make_unique<OpenSim::PhysicalOffsetFrame>();
            rv->setName(pofName);
            rv->set_translation(locationInMeshFrame);
            rv->connectSocket_parent(mesh.getFrame());
            return rv;
        }();

        // perform model mutation
        {
            OpenSim::Model& mutableModel = model.updModel();

            OpenSim::PhysicalOffsetFrame const& pofRef = osc::AddModelComponent(mutableModel, std::move(pof));
            osc::FinalizeConnections(mutableModel);
            osc::InitializeModel(mutableModel);
            osc::InitializeState(mutableModel);
            model.setSelected(&pofRef);
            model.commit(commitMessage);
        }
    }

    void ActionAddPointToPointEdge(
        osc::UndoableModelStatePair& model,
        OpenSim::Point const& pointA,
        OpenSim::Point const& pointB)
    {
        std::string const edgeName = GenerateSceneElementName("edge_");
        std::string const commitMessage = GenerateAddedSomethingCommitMessage(edgeName);

        // create edge
        auto edge = std::make_unique<FDPointToPointEdge>();
        edge->connectSocket_pointA(pointA);
        edge->connectSocket_pointB(pointB);

        // perform model mutation
        {
            OpenSim::Model& mutableModel = model.updModel();

            FDPointToPointEdge const& edgeRef = osc::AddModelComponent(mutableModel, std::move(edge));
            osc::FinalizeConnections(mutableModel);
            osc::InitializeModel(mutableModel);
            osc::InitializeState(mutableModel);
            model.setSelected(&edgeRef);
            model.commit(commitMessage);
        }
    }

    void ActionAddMidpoint(
        osc::UndoableModelStatePair& model,
        OpenSim::Point const& pointA,
        OpenSim::Point const& pointB)
    {
        std::string const midpointName = GenerateSceneElementName("midpoint_");
        std::string const commitMessage = GenerateAddedSomethingCommitMessage(midpointName);

        // create midpoint component
        auto midpoint = std::make_unique<MidpointLandmark>();
        midpoint->connectSocket_pointA(pointA);
        midpoint->connectSocket_pointB(pointB);

        // perform model mutation
        {
            OpenSim::Model& mutableModel = model.updModel();

            MidpointLandmark const& midpointRef = osc::AddModelComponent(mutableModel, std::move(midpoint));
            osc::FinalizeConnections(mutableModel);
            osc::InitializeModel(mutableModel);
            osc::InitializeState(mutableModel);
            model.setSelected(&midpointRef);
            model.commit(commitMessage);
        }
    }

    void ActionAddCrossProductEdge(
        osc::UndoableModelStatePair& model,
        FDVirtualEdge const& edgeA,
        FDVirtualEdge const& edgeB)
    {
        std::string const edgeName = GenerateSceneElementName("crossproduct_");
        std::string const commitMessage = GenerateAddedSomethingCommitMessage(edgeName);

        // create cross product edge component
        auto edge = std::make_unique<FDCrossProductEdge>();
        edge->connectSocket_edgeA(edgeA);
        edge->connectSocket_edgeB(edgeB);

        // perform model mutation
        {
            OpenSim::Model& mutableModel = model.updModel();

            FDCrossProductEdge const& edgeRef = osc::AddModelComponent(mutableModel, std::move(edge));
            osc::FinalizeConnections(mutableModel);
            osc::InitializeModel(mutableModel);
            osc::InitializeState(mutableModel);
            model.setSelected(&edgeRef);
            model.commit(commitMessage);
        }
    }

    void ActionSwapSocketAssignments(
        osc::UndoableModelStatePair& model,
        OpenSim::ComponentPath componentAbsPath,
        std::string firstSocketName,
        std::string secondSocketName)
    {
        // create commit message
        std::string const commitMessage = [&componentAbsPath, &firstSocketName, &secondSocketName]()
        {
            std::stringstream ss;
            ss << "swapped socket '" << firstSocketName << "' with socket '" << secondSocketName << " in " << componentAbsPath.getComponentName().c_str();
            return std::move(ss).str();
        }();

        // look things up in the mutable model
        OpenSim::Model& mutModel = model.updModel();
        OpenSim::Component* const component = osc::FindComponentMut(mutModel, componentAbsPath);
        if (!component)
        {
            osc::log::error("failed to find %s in model, skipping action", componentAbsPath.toString().c_str());
            return;
        }

        OpenSim::AbstractSocket* const firstSocket = osc::FindSocketMut(*component, firstSocketName);
        if (!firstSocket)
        {
            osc::log::error("failed to find socket %s in %s, skipping action", firstSocketName.c_str(), component->getName().c_str());
            return;
        }

        OpenSim::AbstractSocket* const secondSocket = osc::FindSocketMut(*component, secondSocketName);
        if (!secondSocket)
        {
            osc::log::error("failed to find socket %s in %s, skipping action", secondSocketName.c_str(), component->getName().c_str());
            return;
        }

        // perform swap
        std::string const firstSocketPath = firstSocket->getConnecteePath();
        firstSocket->setConnecteePath(secondSocket->getConnecteePath());
        secondSocket->setConnecteePath(firstSocketPath);

        // finalize and commit
        osc::InitializeModel(mutModel);
        osc::InitializeState(mutModel);
        model.commit(commitMessage);
    }

    void ActionSwapPointToPointEdgeEnds(
        osc::UndoableModelStatePair& model,
        FDPointToPointEdge const& edge)
    {
        ActionSwapSocketAssignments(model, edge.getAbsolutePath(), "pointA", "pointB");
    }

    void ActionSwapCrossProductEdgeOperands(
        osc::UndoableModelStatePair& model,
        FDCrossProductEdge const& edge)
    {
        ActionSwapSocketAssignments(model, edge.getAbsolutePath(), "edgeA", "edgeB");
    }

    void ActionAddFrame(
        std::shared_ptr<osc::UndoableModelStatePair> const& model,
        FDVirtualEdge const& firstEdge,
        MaybeNegatedAxis firstEdgeAxis,
        FDVirtualEdge const& otherEdge,
        OpenSim::Point const& origin)
    {
        std::string const frameName = GenerateSceneElementName("frame_");
        std::string const commitMessage = GenerateAddedSomethingCommitMessage(frameName);

        // create the frame
        auto frame = std::make_unique<LandmarkDefinedFrame>();
        frame->set_axisEdgeDimension(ToString(firstEdgeAxis));
        frame->set_secondAxisDimension(ToString(Next(firstEdgeAxis)));
        frame->connectSocket_axisEdge(firstEdge);
        frame->connectSocket_otherEdge(otherEdge);
        frame->connectSocket_origin(origin);

        // perform model mutation
        {
            OpenSim::Model& mutModel = model->updModel();

            LandmarkDefinedFrame const& frameRef = osc::AddModelComponent(mutModel, std::move(frame));
            osc::FinalizeConnections(mutModel);
            osc::InitializeModel(mutModel);
            osc::InitializeState(mutModel);
            model->setSelected(&frameRef);
            model->commit(commitMessage);
        }
    }

    std::unique_ptr<osc::UndoableModelStatePair> MakeUndoableModelFromSceneModel(
        osc::UndoableModelStatePair const& sceneModel)
    {
        auto modelCopy = std::make_unique<OpenSim::Model>(sceneModel.getModel());
        modelCopy->upd_ComponentSet().clearAndDestroy();
        return std::make_unique<osc::UndoableModelStatePair>(std::move(modelCopy));
    }

    void ActionExportFrameDefinitionSceneModelToEditorTab(
        osc::ParentPtr<osc::ITabHost> const& tabHost,
        osc::UndoableModelStatePair const& model)
    {
        auto maybeMainUIStateAPI = osc::DynamicParentCast<osc::IMainUIStateAPI>(tabHost);
        if (!maybeMainUIStateAPI)
        {
            osc::log::error("Tried to export frame definition scene to an OpenSim model but there is no MainUIStateAPI data");
            return;
        }

        (*maybeMainUIStateAPI)->addAndSelectTab<osc::ModelEditorTab>(
            *maybeMainUIStateAPI,
            MakeUndoableModelFromSceneModel(model)
        );
    }
}

// choose `n` components UI flow
namespace
{
    // parameters used to create a "choose components" layer
    struct ChooseComponentsEditorLayerParameters final {
        std::string popupHeaderText = "Choose Something";

        // predicate that is used to test whether the element is choose-able
        std::function<bool(OpenSim::Component const&)> canChooseItem = [](OpenSim::Component const&) { return true; };

        // (maybe) the components that the user has already chosen, or is
        // assigning to (and, therefore, should maybe be highlighted but
        // non-selectable)
        std::unordered_set<std::string> componentsBeingAssignedTo;

        size_t numComponentsUserMustChoose = 1;

        std::function<bool(std::unordered_set<std::string> const&)> onUserFinishedChoosing = [](std::unordered_set<std::string> const&)
        {
            return true;
        };
    };

    // top-level shared state for the "choose components" layer
    struct ChooseComponentsEditorLayerSharedState final {

        explicit ChooseComponentsEditorLayerSharedState(
            std::shared_ptr<osc::UndoableModelStatePair> model_,
            ChooseComponentsEditorLayerParameters parameters_) :

            model{std::move(model_)},
            popupParams{std::move(parameters_)}
        {
        }

        std::shared_ptr<osc::SceneCache> meshCache = osc::App::singleton<osc::SceneCache>();
        std::shared_ptr<osc::UndoableModelStatePair> model;
        ChooseComponentsEditorLayerParameters popupParams;
        osc::ModelRendererParams renderParams;
        std::string hoveredComponent;
        std::unordered_set<std::string> alreadyChosenComponents;
        bool shouldClosePopup = false;
    };

    // grouping of scene (3D) decorations and an associated scene BVH
    struct BVHedDecorations final {
        void clear()
        {
            decorations.clear();
            bvh.clear();
        }

        std::vector<osc::SceneDecoration> decorations;
        osc::BVH bvh;
    };

    // generates scene decorations for the "choose components" layer
    void GenerateChooseComponentsDecorations(
        ChooseComponentsEditorLayerSharedState const& state,
        BVHedDecorations& out)
    {
        out.clear();

        auto const onModelDecoration = [&state, &out](OpenSim::Component const& component, osc::SceneDecoration&& decoration)
        {
            // update flags based on path
            std::string const absPath = osc::GetAbsolutePathString(component);
            if (osc::Contains(state.popupParams.componentsBeingAssignedTo, absPath))
            {
                decoration.flags |= osc::SceneDecorationFlags::IsSelected;
            }
            if (osc::Contains(state.alreadyChosenComponents, absPath))
            {
                decoration.flags |= osc::SceneDecorationFlags::IsSelected;
            }
            if (absPath == state.hoveredComponent)
            {
                decoration.flags |= osc::SceneDecorationFlags::IsHovered;
            }

            if (state.popupParams.canChooseItem(component))
            {
                decoration.id = absPath;
            }
            else
            {
                decoration.color.a *= 0.2f;  // fade non-selectable objects
            }

            out.decorations.push_back(std::move(decoration));
        };

        osc::GenerateModelDecorations(
            *state.meshCache,
            state.model->getModel(),
            state.model->getState(),
            state.renderParams.decorationOptions,
            state.model->getFixupScaleFactor(),
            onModelDecoration
        );

        osc::UpdateSceneBVH(out.decorations, out.bvh);

        auto const onOverlayDecoration = [&](osc::SceneDecoration&& decoration)
        {
            out.decorations.push_back(std::move(decoration));
        };

        osc::GenerateOverlayDecorations(
            *state.meshCache,
            state.renderParams.overlayOptions,
            out.bvh,
            onOverlayDecoration
        );
    }

    // modal popup that prompts the user to select components in the model (e.g.
    // to define an edge, or a frame)
    class ChooseComponentsEditorLayer final : public osc::ModelEditorViewerPanelLayer {
    public:
        ChooseComponentsEditorLayer(
            std::shared_ptr<osc::UndoableModelStatePair> model_,
            ChooseComponentsEditorLayerParameters parameters_) :

            m_State{std::move(model_), std::move(parameters_)},
            m_Renderer{osc::App::config(), *osc::App::singleton<osc::SceneCache>(), *osc::App::singleton<osc::ShaderCache>()}
        {
        }

    private:
        bool implHandleKeyboardInputs(
            osc::ModelEditorViewerPanelParameters& params,
            osc::ModelEditorViewerPanelState& state) final
        {
            return osc::UpdatePolarCameraFromImGuiKeyboardInputs(
                params.updRenderParams().camera,
                state.viewportRect,
                m_Decorations.bvh.getRootAABB()
            );
        }

        bool implHandleMouseInputs(
            osc::ModelEditorViewerPanelParameters& params,
            osc::ModelEditorViewerPanelState& state) final
        {
            bool rv = osc::UpdatePolarCameraFromImGuiMouseInputs(
                params.updRenderParams().camera,
                osc::Dimensions(state.viewportRect)
            );

            if (osc::IsDraggingWithAnyMouseButtonDown())
            {
                m_State.hoveredComponent = {};
            }

            if (m_IsLeftClickReleasedWithoutDragging)
            {
                rv = tryToggleHover() || rv;
            }

            return rv;
        }

        void implOnDraw(
            osc::ModelEditorViewerPanelParameters& panelParams,
            osc::ModelEditorViewerPanelState& panelState) final
        {
            bool const layerIsHovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_RootAndChildWindows);

            // update this layer's state from provided state
            m_State.renderParams = panelParams.getRenderParams();
            m_IsLeftClickReleasedWithoutDragging = osc::IsMouseReleasedWithoutDragging(ImGuiMouseButton_Left);
            m_IsRightClickReleasedWithoutDragging = osc::IsMouseReleasedWithoutDragging(ImGuiMouseButton_Right);
            if (ImGui::IsKeyReleased(ImGuiKey_Escape))
            {
                m_State.shouldClosePopup = true;
            }

            // generate decorations + rendering params
            GenerateChooseComponentsDecorations(m_State, m_Decorations);
            osc::SceneRendererParams const rendererParameters = osc::CalcSceneRendererParams(
                m_State.renderParams,
                osc::Dimensions(panelState.viewportRect),
                osc::App::get().getCurrentAntiAliasingLevel(),
                m_State.model->getFixupScaleFactor()
            );

            // render to a texture (no caching)
            m_Renderer.render(m_Decorations.decorations, rendererParameters);

            // blit texture as ImGui image
            osc::DrawTextureAsImGuiImage(
                m_Renderer.updRenderTexture(),
                osc::Dimensions(panelState.viewportRect)
            );

            // do hovertest
            if (layerIsHovered)
            {
                std::optional<osc::SceneCollision> const collision = osc::GetClosestCollision(
                    m_Decorations.bvh,
                    *m_State.meshCache,
                    m_Decorations.decorations,
                    m_State.renderParams.camera,
                    ImGui::GetMousePos(),
                    panelState.viewportRect
                );
                if (collision)
                {
                    m_State.hoveredComponent = collision->decorationID;
                }
                else
                {
                    m_State.hoveredComponent = {};
                }
            }

            // show tooltip
            if (OpenSim::Component const* c = osc::FindComponent(m_State.model->getModel(), m_State.hoveredComponent))
            {
                osc::DrawComponentHoverTooltip(*c);
            }

            // show header
            ImGui::SetCursorScreenPos(panelState.viewportRect.p1 + Vec2{10.0f, 10.0f});
            ImGui::Text("%s (ESC to cancel)", m_State.popupParams.popupHeaderText.c_str());

            // handle completion state (i.e. user selected enough components)
            if (m_State.alreadyChosenComponents.size() == m_State.popupParams.numComponentsUserMustChoose)
            {
                m_State.popupParams.onUserFinishedChoosing(m_State.alreadyChosenComponents);
                m_State.shouldClosePopup = true;
            }

            // draw cancellation button
            {
                ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, {10.0f, 10.0f});

                constexpr osc::CStringView cancellationButtonText = ICON_FA_ARROW_LEFT " Cancel (ESC)";
                Vec2 const margin = {25.0f, 25.0f};
                Vec2 const buttonDims = osc::CalcButtonSize(cancellationButtonText);
                Vec2 const buttonTopLeft = panelState.viewportRect.p2 - (buttonDims + margin);
                ImGui::SetCursorScreenPos(buttonTopLeft);
                if (ImGui::Button(cancellationButtonText.c_str()))
                {
                    m_State.shouldClosePopup = true;
                }

                ImGui::PopStyleVar();
            }
        }

        float implGetBackgroundAlpha() const final
        {
            return 1.0f;
        }

        bool implShouldClose() const final
        {
            return m_State.shouldClosePopup;
        }

        bool tryToggleHover()
        {
            std::string const& absPath = m_State.hoveredComponent;
            OpenSim::Component const* component = osc::FindComponent(m_State.model->getModel(), absPath);

            if (!component)
            {
                return false;  // nothing hovered
            }
            else if (osc::Contains(m_State.popupParams.componentsBeingAssignedTo, absPath))
            {
                return false;  // cannot be selected
            }
            else if (auto it = m_State.alreadyChosenComponents.find(absPath); it != m_State.alreadyChosenComponents.end())
            {
                m_State.alreadyChosenComponents.erase(it);
                return true;   // de-selected
            }
            else if (
                m_State.alreadyChosenComponents.size() < m_State.popupParams.numComponentsUserMustChoose &&
                m_State.popupParams.canChooseItem(*component)
            )
            {
                m_State.alreadyChosenComponents.insert(absPath);
                return true;   // selected
            }
            else
            {
                return false;  // don't know how to handle
            }
        }

        ChooseComponentsEditorLayerSharedState m_State;
        BVHedDecorations m_Decorations;
        osc::SceneRenderer m_Renderer;
        bool m_IsLeftClickReleasedWithoutDragging = false;
        bool m_IsRightClickReleasedWithoutDragging = false;
    };

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
        FDVirtualEdge const& firstEdge,
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

            auto const* edgeA = osc::FindComponent<FDVirtualEdge>(model->getModel(), edgeAPath);
            if (!edgeA)
            {
                osc::log::error("edge A's component path (%s) does not exist in the model", edgeAPath.c_str());
                return false;
            }

            auto const* edgeB = osc::FindComponent<FDVirtualEdge>(model->getModel(), edgeBPath);
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

            auto const* firstEdge = osc::FindComponent<FDVirtualEdge>(model->getModel(), firstEdgeAbsPath);
            if (!firstEdge)
            {
                osc::log::error("the first edge's component path (%s) does not exist in the model", firstEdgeAbsPath.c_str());
                return false;
            }

            auto const* otherEdge = osc::FindComponent<FDVirtualEdge>(model->getModel(), secondEdgeAbsPath);
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
        FDVirtualEdge const& firstEdge,
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
        FDVirtualEdge const& firstEdge,
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

    void ActionCreateBodyFromFrame(
        std::shared_ptr<osc::UndoableModelStatePair> const& model,
        OpenSim::ComponentPath const& frameAbsPath,
        OpenSim::ComponentPath const& meshAbsPath,
        OpenSim::ComponentPath const& jointFrameAbsPath,
        OpenSim::ComponentPath const& parentFrameAbsPath)
    {
        // validate external inputs

        osc::log::debug("validate external inputs");
        auto const* const meshFrame = osc::FindComponent<OpenSim::PhysicalFrame>(model->getModel(), frameAbsPath);
        if (!meshFrame)
        {
            osc::log::error("%s: cannot find frame: skipping body creation", frameAbsPath.toString().c_str());
            return;
        }

        auto const* const mesh = osc::FindComponent<OpenSim::Mesh>(model->getModel(), meshAbsPath);
        if (!mesh)
        {
            osc::log::error("%s: cannot find mesh: skipping body creation", meshAbsPath.toString().c_str());
            return;
        }

        auto const* const jointFrame = osc::FindComponent<OpenSim::PhysicalFrame>(model->getModel(), jointFrameAbsPath);
        if (!jointFrame)
        {
            osc::log::error("%s: cannot find joint frame: skipping body creation", jointFrameAbsPath.toString().c_str());
            return;
        }

        auto const* const parentFrame = osc::FindComponent<OpenSim::PhysicalFrame>(model->getModel(), parentFrameAbsPath);
        if (!parentFrame)
        {
            osc::log::error("%s: cannot find parent frame: skipping body creation", parentFrameAbsPath.toString().c_str());
            return;
        }

        // create body
        osc::log::debug("create body");
        std::string const bodyName = meshFrame->getName() + "_body";
        double const bodyMass = 1.0;
        SimTK::Vec3 const bodyCenterOfMass = {0.0, 0.0, 0.0};
        SimTK::Inertia const bodyInertia = {1.0, 1.0, 1.0};
        auto body = std::make_unique<OpenSim::Body>(
            bodyName,
            bodyMass,
            bodyCenterOfMass,
            bodyInertia
        );

        // create joint (centered using offset frames)
        osc::log::debug("create joint");
        auto joint = std::make_unique<OpenSim::FreeJoint>();
        joint->setName(meshFrame->getName() + "_joint");
        {
            auto jointParentPof = std::make_unique<OpenSim::PhysicalOffsetFrame>();
            jointParentPof->setParentFrame(*parentFrame);
            jointParentPof->setName(meshFrame->getName() + "_parent_offset");
            jointParentPof->setOffsetTransform(jointFrame->findTransformBetween(model->getState(), *parentFrame));

            // care: ownership change happens here (#642)
            OpenSim::PhysicalOffsetFrame& pof = osc::AddFrame(*joint, std::move(jointParentPof));
            joint->connectSocket_parent_frame(pof);
        }
        {
            auto jointChildPof = std::make_unique<OpenSim::PhysicalOffsetFrame>();
            jointChildPof->setParentFrame(*body);
            jointChildPof->setName(meshFrame->getName() + "_child_offset");
            jointChildPof->setOffsetTransform(jointFrame->findTransformBetween(model->getState(), *meshFrame));

            // care: ownership change happens here (#642)
            OpenSim::PhysicalOffsetFrame& pof = osc::AddFrame(*joint, std::move(jointChildPof));
            joint->connectSocket_child_frame(pof);
        }

        // create PoF for the mesh
        osc::log::debug("create pof");
        auto meshPof = std::make_unique<OpenSim::PhysicalOffsetFrame>();
        meshPof->setParentFrame(*body);
        meshPof->setName(mesh->getFrame().getName());
        meshPof->setOffsetTransform(mesh->getFrame().findTransformBetween(model->getState(), *meshFrame));

        // create commit message
        std::string const commitMessage = [&body]()
        {
            std::stringstream ss;
            ss << "created " << body->getName();
            return std::move(ss).str();
        }();

        // start mutating the model
        osc::log::debug("start model mutation");
        try
        {
            // CARE: store mesh path before mutatingthe model, because the mesh reference
            // may become invalidated by other model mutations
            OpenSim::ComponentPath const meshPath = osc::GetAbsolutePath(*mesh);

            OpenSim::Model& mutModel = model->updModel();

            OpenSim::PhysicalOffsetFrame& meshPofRef = osc::AddComponent(*body, std::move(meshPof));
            osc::AddJoint(mutModel, std::move(joint));
            OpenSim::Body& bodyRef = osc::AddBody(mutModel, std::move(body));

            // attach copy of source mesh to mesh PoF
            //
            // (must be done after adding body etc. to model and finalizing - #325)
            osc::FinalizeConnections(mutModel);
            osc::AttachGeometry<OpenSim::Mesh>(meshPofRef, *mesh);

            // ensure model is in a valid, initialized, state before moving
            // and reassigning things around
            osc::FinalizeConnections(mutModel);
            osc::InitializeModel(mutModel);
            osc::InitializeState(mutModel);

            // if the mesh's PoF was only used by the mesh then reassign
            // everything to the new PoF and delete the old one
            if (auto const* pof = osc::GetOwner<OpenSim::PhysicalOffsetFrame>(*mesh);
                pof && osc::GetNumChildren(*pof) == 3)  // mesh+frame geom+wrap object set
            {
                osc::log::debug("reassign sockets");
                osc::RecursivelyReassignAllSockets(mutModel, *pof, meshPofRef);
                osc::FinalizeConnections(mutModel);

                if (auto* mutPof = osc::FindComponentMut<OpenSim::PhysicalOffsetFrame>(mutModel, osc::GetAbsolutePathOrEmpty(pof)))
                {
                    osc::log::debug("delete old pof");
                    osc::TryDeleteComponentFromModel(mutModel, *mutPof);
                    osc::InitializeModel(mutModel);
                    osc::InitializeState(mutModel);

                    // care: `pof` is now dead
                }
            }

            // delete old mesh
            if (auto* mutMesh = osc::FindComponentMut<OpenSim::Mesh>(mutModel, meshAbsPath))
            {
                osc::log::debug("delete old mesh");
                osc::TryDeleteComponentFromModel(mutModel, *mutMesh);
                osc::InitializeModel(mutModel);
                osc::InitializeState(mutModel);
            }

            osc::InitializeModel(mutModel);
            osc::InitializeState(mutModel);
            model->setSelected(&bodyRef);
            model->commit(commitMessage);
        }
        catch (std::exception const& ex)
        {
            osc::log::error("error detected while trying to add a body to the model: %s", ex.what());
            model->rollback();
        }
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

// "calculate" context menu
namespace
{
    // draws the calculate menu for an edge
    void DrawCalculateMenu(
        OpenSim::Component const& root,
        SimTK::State const& state,
        FDVirtualEdge const& edge)
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
                        osc::ToVec3(edge.getEdgePointsInGround(state).start)
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
                        osc::ToVec3(edge.getEdgePointsInGround(state).end)
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
                        osc::ToVec3(CalcDirection(edge.getEdgePointsInGround(state)))
                    );
                };

                osc::DrawWithRespectToMenuContainingMenuPerFrame(root, onFrameMenuOpened);
                ImGui::EndMenu();
            }

            ImGui::EndMenu();
        }
    }
}

// context menu
namespace
{
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
        FDVirtualEdge const& edge)
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
        FDPointToPointEdge const& edge)
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
        FDCrossProductEdge const& edge)
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
            else if (auto const* maybeP2PEdge = dynamic_cast<FDPointToPointEdge const*>(maybeComponent))
            {
                DrawRightClickedPointToPointEdgeContextMenu(*m_EditorAPI, m_Model, m_MaybeSourceVisualizerEvent, *maybeP2PEdge);
            }
            else if (auto const* maybeCPEdge = dynamic_cast<FDCrossProductEdge const*>(maybeComponent))
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

    class FrameDefinitionTabToolbar final {
    public:
        FrameDefinitionTabToolbar(
            std::string_view label_,
            osc::ParentPtr<osc::ITabHost> tabHost_,
            std::shared_ptr<osc::UndoableModelStatePair> model_) :

            m_Label{label_},
            m_TabHost{std::move(tabHost_)},
            m_Model{std::move(model_)}
        {
        }

        void onDraw()
        {
            if (osc::BeginToolbar(m_Label, Vec2{5.0f, 5.0f}))
            {
                drawContent();
            }
            ImGui::End();
        }
    private:
        void drawContent()
        {
            osc::DrawUndoAndRedoButtons(*m_Model);
            osc::SameLineWithVerticalSeperator();
            osc::DrawSceneScaleFactorEditorControls(*m_Model);
            osc::SameLineWithVerticalSeperator();
            drawExportToOpenSimButton();
        }

        void drawExportToOpenSimButton()
        {
            size_t const numBodies = osc::GetNumChildren(m_Model->getModel().getBodySet());

            if (numBodies == 0)
            {
                ImGui::BeginDisabled();
            }
            if (ImGui::Button(ICON_FA_FILE_EXPORT " Export to OpenSim"))
            {
                ActionExportFrameDefinitionSceneModelToEditorTab(m_TabHost, *m_Model);
            }
            if (numBodies == 0)
            {
                ImGui::EndDisabled();
            }
            if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
            {
                drawExportToOpenSimTooltipContent(numBodies);
            }
        }

        void drawExportToOpenSimTooltipContent(size_t numBodies)
        {
            osc::BeginTooltip();
            osc::TooltipHeaderText("Export to OpenSim");
            osc::TooltipDescriptionSpacer();
            osc::TooltipDescriptionText("Exports the frame definition scene to opensim.");
            if (numBodies == 0)
            {
                ImGui::Separator();
                osc::TextWarning("Warning:");
                ImGui::SameLine();
                ImGui::Text("You currently have %zu bodies defined. Use the 'Add > Body from This' feature on a frame in your scene to add a new body", numBodies);
            }
            osc::EndTooltip();
        }

        std::string m_Label;
        osc::ParentPtr<osc::ITabHost> m_TabHost;
        std::shared_ptr<osc::UndoableModelStatePair> m_Model;
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
