#include "FrameDefinitionTab.hpp"

#include "OpenSimCreator/Graphics/CustomRenderingOptions.hpp"
#include "OpenSimCreator/Graphics/OpenSimDecorationOptions.hpp"
#include "OpenSimCreator/Graphics/OpenSimDecorationGenerator.hpp"
#include "OpenSimCreator/Graphics/OverlayDecorationGenerator.hpp"
#include "OpenSimCreator/Graphics/OpenSimGraphicsHelpers.hpp"
#include "OpenSimCreator/Graphics/SimTKMeshLoader.hpp"
#include "OpenSimCreator/MiddlewareAPIs/EditorAPI.hpp"
#include "OpenSimCreator/Panels/ModelEditorViewerPanel.hpp"
#include "OpenSimCreator/Panels/ModelEditorViewerPanelLayer.hpp"
#include "OpenSimCreator/Panels/ModelEditorViewerPanelParameters.hpp"
#include "OpenSimCreator/Panels/ModelEditorViewerPanelRightClickEvent.hpp"
#include "OpenSimCreator/Panels/ModelEditorViewerPanelState.hpp"
#include "OpenSimCreator/Panels/NavigatorPanel.hpp"
#include "OpenSimCreator/Panels/PropertiesPanel.hpp"
#include "OpenSimCreator/Widgets/MainMenu.hpp"
#include "OpenSimCreator/ActionFunctions.hpp"
#include "OpenSimCreator/OpenSimHelpers.hpp"
#include "OpenSimCreator/SimTKHelpers.hpp"
#include "OpenSimCreator/UndoableModelStatePair.hpp"
#include "OpenSimCreator/VirtualConstModelStatePair.hpp"

#include <oscar/Bindings/ImGuiHelpers.hpp>
#include <oscar/Graphics/Color.hpp>
#include <oscar/Graphics/GraphicsHelpers.hpp>
#include <oscar/Graphics/MeshCache.hpp>
#include <oscar/Graphics/SceneDecoration.hpp>
#include <oscar/Graphics/SceneRenderer.hpp>
#include <oscar/Graphics/SceneRendererParams.hpp>
#include <oscar/Graphics/ShaderCache.hpp>
#include <oscar/Maths/BVH.hpp>
#include <oscar/Maths/MathHelpers.hpp>
#include <oscar/Maths/Rect.hpp>
#include <oscar/Panels/LogViewerPanel.hpp>
#include <oscar/Panels/Panel.hpp>
#include <oscar/Panels/PanelManager.hpp>
#include <oscar/Panels/StandardPanel.hpp>
#include <oscar/Platform/App.hpp>
#include <oscar/Platform/Log.hpp>
#include <oscar/Platform/os.hpp>
#include <oscar/Utils/Assertions.hpp>
#include <oscar/Utils/CStringView.hpp>
#include <oscar/Utils/FilesystemHelpers.hpp>
#include <oscar/Utils/UID.hpp>
#include <oscar/Widgets/Popup.hpp>
#include <oscar/Widgets/PopupManager.hpp>
#include <oscar/Widgets/StandardPopup.hpp>
#include <oscar/Widgets/WindowMenu.hpp>

#include <IconsFontAwesome5.h>
#include <imgui.h>
#include <OpenSim/Common/Component.h>
#include <OpenSim/Common/ComponentPath.h>
#include <OpenSim/Simulation/Model/Model.h>
#include <OpenSim/Simulation/Model/PhysicalOffsetFrame.h>
#include <SDL_events.h>

#include <atomic>
#include <cstdint>
#include <cstddef>
#include <filesystem>
#include <functional>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_set>
#include <utility>
#include <vector>

namespace
{
    constexpr osc::CStringView c_TabStringID = "OpenSim/Experimental/FrameDefinition";

    // custom helper that customizes the OpenSim model defaults to be more
    // suitable for the frame definition UI
    std::shared_ptr<osc::UndoableModelStatePair> MakeSharedUndoableFrameDefinitionModel()
    {
        auto model = std::make_unique<OpenSim::Model>();
        model->updDisplayHints().set_show_frames(false);
        return std::make_shared<osc::UndoableModelStatePair>(std::move(model));
    }

    // gets the next unique suffix numer for geometry
    int32_t GetNextGlobalGeometrySuffix()
    {
        static std::atomic<int32_t> s_GeometryCounter = 0;
        return s_GeometryCounter++;
    }
}

// choose component modal flow
namespace
{
    // parameters used to create a "choose components" layer
    struct ChooseComponentsEditorLayerParameters final {
        std::string popupHeaderText = "choose something";

        bool userCanChoosePoints = false;

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

        std::shared_ptr<osc::MeshCache> meshCache = osc::App::singleton<osc::MeshCache>();
        std::shared_ptr<osc::UndoableModelStatePair> model;
        ChooseComponentsEditorLayerParameters popupParams;
        osc::ModelRendererParams renderParams;
        OpenSim::ComponentPath hoveredComponent;
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

        auto const onModelDecoration = [&out](OpenSim::Component const& component, osc::SceneDecoration&& decoration)
        {
            // TODO: change highlighting/selection based on the panel state

            decoration.color = osc::Color::red();
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
            m_Renderer{osc::App::get().config(), *osc::App::singleton<osc::MeshCache>(), *osc::App::singleton<osc::ShaderCache>()}
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
            m_IsHandlingMouseInputs = true;

            // try updating the camera (mouse panning, etc.)
            bool rv = osc::UpdatePolarCameraFromImGuiMouseInputs(
                osc::Dimensions(state.viewportRect),
                params.updRenderParams().camera
            );

            if (osc::IsDraggingWithAnyMouseButtonDown())
            {
                m_State.hoveredComponent = {};
            }

            if (m_IsLeftClickReleasedWithoutDragging)
            {
                // TODO: possibly toggle selection
                rv = true;
            }

            return rv;
        }

        void implOnDraw(
            osc::ModelEditorViewerPanelParameters& panelParams,
            osc::ModelEditorViewerPanelState& panelState) final
        {
            // update this layer's state from provided state
            m_State.renderParams = panelParams.getRenderParams();
            if (ImGui::IsKeyReleased(ImGuiKey_Escape))
            {
                m_State.shouldClosePopup = true;
            }

            // generate decorations + rendering params
            GenerateChooseComponentsDecorations(m_State, m_Decorations);
            osc::SceneRendererParams const rendererParameters = osc::CalcSceneRendererParams(
                m_State.renderParams,
                osc::Dimensions(panelState.viewportRect),
                osc::App::get().getMSXAASamplesRecommended(),
                m_State.model->getFixupScaleFactor()
            );

            // render to a texture (no caching)
            m_Renderer.draw(m_Decorations.decorations, rendererParameters);

            // blit texture as ImGui image
            osc::DrawTextureAsImGuiImage(
                m_Renderer.updRenderTexture(),
                osc::Dimensions(panelState.viewportRect)
            );
            m_IsLeftClickReleasedWithoutDragging = osc::IsMouseReleasedWithoutDragging(ImGuiMouseButton_Left);
            m_IsRightClickReleasedWithoutDragging = osc::IsMouseReleasedWithoutDragging(ImGuiMouseButton_Right);

            // generate decoarations
            ImGui::SetCursorScreenPos(panelState.viewportRect.p1);
            ImGui::Text("TODO: draw edge definition popup content");
            ImGui::Text("TODO: - should be a 3D render");
            ImGui::Text("TODO: - with a camera that is identical to the source camera");
            ImGui::Text("TODO: - that shows non-selectable objects in a faded way");
            ImGui::Text("TODO: - and shows selectable objects in a solid non-faded color");
            ImGui::Text("TODO: - and has hover logic");
            ImGui::Text("TODO: - and highlights selected objects");
        }

        bool implShouldClose() const final
        {
            return m_State.shouldClosePopup;
        }

        ChooseComponentsEditorLayerSharedState m_State;
        BVHedDecorations m_Decorations;
        osc::SceneRenderer m_Renderer;
        bool m_IsHandlingMouseInputs = false;
        bool m_IsLeftClickReleasedWithoutDragging = false;
        bool m_IsRightClickReleasedWithoutDragging = false;
    };
}

// user-enactable actions
namespace
{
    void ActionPromptUserToAddMeshFile(osc::UndoableModelStatePair& model)
    {
        std::optional<std::filesystem::path> const maybeMeshPath =
            osc::PromptUserForFile(osc::GetCommaDelimitedListOfSupportedSimTKMeshFormats());
        if (!maybeMeshPath)
        {
            return;  // user didn't select anything
        }
        std::filesystem::path const& meshPath = *maybeMeshPath;
        std::string const meshName = osc::FileNameWithoutExtension(meshPath);

        OpenSim::Model const& immutableModel = model.getModel();

        // add an offset frame that is connected to ground - this will become
        // the mesh's offset frame
        auto meshPhysicalOffsetFrame = std::make_unique<OpenSim::PhysicalOffsetFrame>();
        meshPhysicalOffsetFrame->setParentFrame(immutableModel.getGround());
        meshPhysicalOffsetFrame->setName(meshName + "_offset");

        // attach the mesh to the frame
        {
            auto mesh = std::make_unique<OpenSim::Mesh>(meshPath.string());
            mesh->setName(meshName);
            meshPhysicalOffsetFrame->attachGeometry(mesh.release());
        }

        // create a human-readable commit message
        std::string const commitMessage = [&meshPath]()
        {
            std::stringstream ss;
            ss << "added " << meshPath.filename();
            return std::move(ss).str();
        }();

        // finally, perform the model mutation
        {
            OpenSim::Model& mutableModel = model.updModel();
            mutableModel.addComponent(meshPhysicalOffsetFrame.release());
            mutableModel.finalizeConnections();

            osc::InitializeModel(mutableModel);
            osc::InitializeState(mutableModel);
            model.commit(commitMessage);
        }
    }

    void ActionAddSphereInMeshFrame(
        osc::UndoableModelStatePair& model,
        OpenSim::Mesh const& mesh,
        std::optional<glm::vec3> const& maybeClickPosInGround)
    {
        // if the caller requests that the sphere is placed at a particular
        // location in ground, then place it in the correct location w.r.t.
        // the mesh frame
        SimTK::Vec3 translationInMeshFrame = {0.0, 0.0, 0.0};
        if (maybeClickPosInGround)
        {
            SimTK::Transform const mesh2ground = mesh.getFrame().getTransformInGround(model.getState());
            SimTK::Transform const ground2mesh = mesh2ground.invert();
            SimTK::Vec3 const translationInGround = osc::ToSimTKVec3(*maybeClickPosInGround);

            translationInMeshFrame = ground2mesh * translationInGround;
        }

        // generate sphere name
        std::string const sphereName = []()
        {
            std::stringstream ss;
            ss << "sphere_" << GetNextGlobalGeometrySuffix();
            return std::move(ss).str();
        }();

        OpenSim::Model const& immutableModel = model.getModel();

        // add an offset frame to the mesh: this is how the sphere can be
        // freely moved in the scene
        auto meshPhysicalOffsetFrame = std::make_unique<OpenSim::PhysicalOffsetFrame>();
        meshPhysicalOffsetFrame->setParentFrame(dynamic_cast<OpenSim::PhysicalFrame const&>(mesh.getFrame()));
        meshPhysicalOffsetFrame->setName(sphereName + "_offset");
        meshPhysicalOffsetFrame->set_translation(translationInMeshFrame);

        // attach the sphere to the frame
        OpenSim::Sphere const* const spherePtr = [&sphereName, &meshPhysicalOffsetFrame]()
        {
            auto sphere = std::make_unique<OpenSim::Sphere>();
            sphere->setName(sphereName);
            sphere->set_radius(0.01);  // 1 cm radius - like a small marker
            sphere->upd_Appearance().set_color({0.1, 1.0, 0.1});  // green(ish) - so it's visually distinct from a mesh
            OpenSim::Sphere const* ptr = sphere.get();
            meshPhysicalOffsetFrame->attachGeometry(sphere.release());
            return ptr;
        }();

        // create a human-readable commit message
        std::string const commitMessage = [&sphereName]()
        {
            std::stringstream ss;
            ss << "added " << sphereName;
            return std::move(ss).str();
        }();

        // finally, perform the model mutation
        {
            OpenSim::Model& mutableModel = model.updModel();
            mutableModel.addComponent(meshPhysicalOffsetFrame.release());
            mutableModel.finalizeConnections();
            osc::InitializeModel(mutableModel);
            osc::InitializeState(mutableModel);

            model.setSelected(spherePtr);
            model.commit(commitMessage);
        }
    }
}

// context menu
namespace
{
    void DrawRightClickedNothingContextMenu(osc::UndoableModelStatePair& model)
    {
        if (ImGui::MenuItem("Add Mesh"))
        {
            ActionPromptUserToAddMeshFile(model);
        }
    }

    void DrawRightClickedMeshContextMenu(
        osc::UndoableModelStatePair& model,
        OpenSim::Mesh const& mesh,
        std::optional<glm::vec3> const& maybeClickPosInGround)
    {
        if (ImGui::MenuItem("add sphere"))
        {
            ActionAddSphereInMeshFrame(model, mesh, maybeClickPosInGround);
        }
    }

    void DrawRightClickedSphereContextMenu(
        osc::EditorAPI& editor,
        std::shared_ptr<osc::UndoableModelStatePair> model,
        OpenSim::Sphere const& sphere,
        std::optional<osc::ModelEditorViewerPanelRightClickEvent> const& maybeSourceEvent)
    {
        if (maybeSourceEvent && ImGui::MenuItem("create edge"))
        {
            osc::PanelManager& panelManager = *editor.getPanelManager();

            if (osc::ModelEditorViewerPanel* visualizer = panelManager.tryUpdPanelByNameT<osc::ModelEditorViewerPanel>(maybeSourceEvent->sourcePanelName))
            {
                ChooseComponentsEditorLayerParameters options;
                options.popupHeaderText = "choose other point";
                options.componentsBeingAssignedTo = {sphere.getAbsolutePathString()};
                options.numComponentsUserMustChoose = 1;
                options.onUserFinishedChoosing = [model, spherePath = sphere.getAbsolutePathString()](std::unordered_set<std::string> const& choices) -> bool
                {
                    // TODO: figure out if the spherePath+choices is enough to add
                    // a new edge into the model and then add the new edge to the model
                    throw std::runtime_error{"todo: handle completion"};
                };

                visualizer->pushLayer(std::make_unique<ChooseComponentsEditorLayer>(model, options));
            }
        }
    }

    void DrawRightClickedUnknownComponentContextMenu(
        osc::UndoableModelStatePair& model,
        OpenSim::Component const& component)
    {
        ImGui::TextDisabled("Unknown component type");
    }

    // popup state for the frame definition tab's general context menu
    class FrameDefinitionContextMenu final : public osc::StandardPopup {
    public:
        FrameDefinitionContextMenu(
            std::string_view popupName_,
            osc::EditorAPI* editorAPI_,
            std::shared_ptr<osc::UndoableModelStatePair> model_,
            OpenSim::ComponentPath componentPath_,
            std::optional<osc::ModelEditorViewerPanelRightClickEvent> maybeSourceVisualizerEvent_ = std::nullopt) :

            StandardPopup{popupName_, {10.0f, 10.0f}, ImGuiWindowFlags_NoMove},
            m_EditorAPI{editorAPI_},
            m_Model{std::move(model_)},
            m_ComponentPath{std::move(componentPath_)},
            m_MaybeSourceVisualizerEvent{maybeSourceVisualizerEvent_}
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
            else if (OpenSim::Mesh const* maybeMesh = dynamic_cast<OpenSim::Mesh const*>(maybeComponent))
            {
                DrawRightClickedMeshContextMenu(*m_Model, *maybeMesh, m_MaybeSourceVisualizerEvent ? m_MaybeSourceVisualizerEvent->maybeClickPositionInGround : std::nullopt);
            }
            else if (OpenSim::Sphere const* maybeSphere = dynamic_cast<OpenSim::Sphere const*>(maybeComponent))
            {
                DrawRightClickedSphereContextMenu(*m_EditorAPI, m_Model, *maybeSphere, m_MaybeSourceVisualizerEvent);
            }
            else
            {
                DrawRightClickedUnknownComponentContextMenu(*m_Model, *maybeComponent);
            }
        }

        osc::EditorAPI* m_EditorAPI;
        std::shared_ptr<osc::UndoableModelStatePair> m_Model;
        OpenSim::ComponentPath m_ComponentPath;
        std::optional<osc::ModelEditorViewerPanelRightClickEvent> m_MaybeSourceVisualizerEvent;
    };
}

// other panels/widgets
namespace
{
    class FrameDefinitionTabNavigatorPanel final : public osc::StandardPanel {
    public:
        FrameDefinitionTabNavigatorPanel(std::string_view panelName_) :
            StandardPanel{panelName_}
        {
        }
    private:
        void implDrawContent() final
        {
            ImGui::Text("TODO: draw navigator content");
        }
    };

    class FrameDefinitionTabMainMenu final {
    public:
        explicit FrameDefinitionTabMainMenu(
            std::shared_ptr<osc::UndoableModelStatePair> model_,
            std::shared_ptr<osc::PanelManager> panelManager_) :
            m_Model{std::move(model_)},
            m_WindowMenu{std::move(panelManager_)}
        {
        }

        void draw()
        {
            drawEditMenu();
            m_WindowMenu.draw();
            m_AboutMenu.draw();
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

        std::shared_ptr<osc::UndoableModelStatePair> m_Model;
        osc::WindowMenu m_WindowMenu;
        osc::MainMenuAboutTab m_AboutMenu;
    };
}

class osc::FrameDefinitionTab::Impl final : public EditorAPI {
public:

    Impl(std::weak_ptr<TabHost> parent_) :
        m_Parent{std::move(parent_)}
    {
        m_PanelManager->registerToggleablePanel(
            "Navigator",
            [this](std::string_view panelName)
            {
                return std::make_shared<FrameDefinitionTabNavigatorPanel>(
                    panelName
                );
            }
        );
        m_PanelManager->registerToggleablePanel(
            "Navigator (legacy)",
            [this](std::string_view panelName)
            {
                return std::make_shared<NavigatorPanel>(
                    panelName,
                    m_Model
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
            [this](std::string_view panelName)
            {
                return std::make_shared<LogViewerPanel>(panelName);
            }
        );
        m_PanelManager->registerSpawnablePanel(
            "viewer",
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
                panelParams.updRenderParams().renderingOptions.setDrawFloor(false);
                panelParams.updRenderParams().overlayOptions.setDrawXZGrid(true);
                panelParams.updRenderParams().backgroundColor = {48.0f/255.0f, 48.0f/255.0f, 48.0f/255.0f, 1.0f};

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

    bool onEvent(SDL_Event const&)
    {
        return false;
    }

    void onTick()
    {
        m_PanelManager->onTick();
    }

    void onDrawMainMenu()
    {
        m_MainMenu.draw();
    }

    void onDraw()
    {
        ImGui::DockSpaceOverViewport(
            ImGui::GetMainViewport(),
            ImGuiDockNodeFlags_PassthruCentralNode
        );
        m_PanelManager->onDraw();
        m_PopupManager.draw();
    }

private:
    void implPushComponentContextMenuPopup(OpenSim::ComponentPath const& componentPath) final
    {
        pushPopup(std::make_unique<FrameDefinitionContextMenu>(
            "##ContextMenu",
            this,
            m_Model,
            componentPath
        ));
    }

    void implPushPopup(std::unique_ptr<Popup> popup) final
    {
        popup->open();
        m_PopupManager.push_back(std::move(popup));
    }

    void implAddMusclePlot(OpenSim::Coordinate const&, OpenSim::Muscle const&)
    {
        // ignore: not applicable in this tab
    }

    std::shared_ptr<PanelManager> implGetPanelManager()
    {
        return m_PanelManager;
    }

    UID m_TabID;
    std::weak_ptr<TabHost> m_Parent;

    std::shared_ptr<UndoableModelStatePair> m_Model = MakeSharedUndoableFrameDefinitionModel();
    std::shared_ptr<PanelManager> m_PanelManager = std::make_shared<PanelManager>();
    PopupManager m_PopupManager;

    FrameDefinitionTabMainMenu m_MainMenu{m_Model, m_PanelManager};
};


// public API (PIMPL)

osc::CStringView osc::FrameDefinitionTab::id() noexcept
{
    return c_TabStringID;
}

osc::FrameDefinitionTab::FrameDefinitionTab(std::weak_ptr<TabHost> parent_) :
    m_Impl{std::make_unique<Impl>(std::move(parent_))}
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
