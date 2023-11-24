#include "MeshWarpingTab.hpp"

#include <OpenSimCreator/Documents/MeshWarp/TPSDocument.hpp>
#include <OpenSimCreator/Documents/MeshWarp/TPSDocumentElementID.hpp>
#include <OpenSimCreator/Documents/MeshWarp/TPSDocumentHelpers.hpp>
#include <OpenSimCreator/Documents/MeshWarp/TPSDocumentInputElementType.hpp>
#include <OpenSimCreator/Documents/MeshWarp/TPSDocumentInputIdentifier.hpp>
#include <OpenSimCreator/Documents/MeshWarp/TPSResultCache.hpp>
#include <OpenSimCreator/Documents/MeshWarp/UndoableTPSDocument.hpp>
#include <OpenSimCreator/Documents/MeshWarp/UndoableTPSDocumentActions.hpp>
#include <OpenSimCreator/Graphics/SimTKMeshLoader.hpp>
#include <OpenSimCreator/UI/Widgets/BasicWidgets.hpp>
#include <OpenSimCreator/UI/Widgets/MainMenu.hpp>
#include <OpenSimCreator/Utils/TPS3D.hpp>

#include <IconsFontAwesome5.h>
#include <imgui.h>
#include <imgui_internal.h>
#include <oscar/Bindings/ImGuiHelpers.hpp>
#include <oscar/Formats/CSV.hpp>
#include <oscar/Formats/OBJ.hpp>
#include <oscar/Formats/STL.hpp>
#include <oscar/Graphics/Camera.hpp>
#include <oscar/Graphics/Color.hpp>
#include <oscar/Graphics/Graphics.hpp>
#include <oscar/Graphics/Material.hpp>
#include <oscar/Graphics/Mesh.hpp>
#include <oscar/Graphics/MeshGenerators.hpp>
#include <oscar/Graphics/ShaderCache.hpp>
#include <oscar/Maths/CollisionTests.hpp>
#include <oscar/Maths/MathHelpers.hpp>
#include <oscar/Maths/Segment.hpp>
#include <oscar/Maths/PolarPerspectiveCamera.hpp>
#include <oscar/Maths/Vec2.hpp>
#include <oscar/Maths/Vec3.hpp>
#include <oscar/Maths/Vec4.hpp>
#include <oscar/Platform/App.hpp>
#include <oscar/Platform/AppMetadata.hpp>
#include <oscar/Platform/Log.hpp>
#include <oscar/Platform/os.hpp>
#include <oscar/Scene/CachedSceneRenderer.hpp>
#include <oscar/Scene/SceneCache.hpp>
#include <oscar/Scene/SceneDecoration.hpp>
#include <oscar/Scene/SceneHelpers.hpp>
#include <oscar/Scene/SceneRendererParams.hpp>
#include <oscar/UI/Panels/LogViewerPanel.hpp>
#include <oscar/UI/Panels/Panel.hpp>
#include <oscar/UI/Panels/PanelManager.hpp>
#include <oscar/UI/Panels/PerfPanel.hpp>
#include <oscar/UI/Panels/StandardPanel.hpp>
#include <oscar/UI/Panels/ToggleablePanelFlags.hpp>
#include <oscar/UI/Panels/UndoRedoPanel.hpp>
#include <oscar/UI/Tabs/TabHost.hpp>
#include <oscar/UI/Widgets/Popup.hpp>
#include <oscar/UI/Widgets/PopupManager.hpp>
#include <oscar/UI/Widgets/RedoButton.hpp>
#include <oscar/UI/Widgets/StandardPopup.hpp>
#include <oscar/UI/Widgets/UndoButton.hpp>
#include <oscar/UI/Widgets/WindowMenu.hpp>
#include <oscar/Utils/CStringView.hpp>
#include <oscar/Utils/EnumHelpers.hpp>
#include <oscar/Utils/HashHelpers.hpp>
#include <oscar/Utils/ParentPtr.hpp>
#include <oscar/Utils/Perf.hpp>
#include <oscar/Utils/UID.hpp>
#include <SDL_events.h>
#include <Simbody.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <functional>
#include <future>
#include <iostream>
#include <limits>
#include <numbers>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <sstream>
#include <type_traits>
#include <unordered_set>
#include <utility>
#include <variant>
#include <vector>

using osc::Color;
using osc::TPSDocumentElementID;
using osc::TPSDocumentInputIdentifier;
using osc::TPSDocumentInputElementType;
using osc::TPSDocumentLandmarkPair;
using osc::TPSDocument;
using osc::Vec2;
using osc::Vec3;
using osc::Vec4;

using namespace osc;

// constants
namespace
{
    constexpr Vec2 c_OverlayPadding = {10.0f, 10.0f};
    constexpr Color c_PairedLandmarkColor = Color::green();
    constexpr Color c_UnpairedLandmarkColor = Color::red();
}

// UI: top-level datastructures that are shared between panels etc.
namespace
{
    // a mouse hovertest result
    struct TPSUIViewportHover final {

        explicit TPSUIViewportHover(Vec3 const& worldspaceLocation_) :
            worldspaceLocation{worldspaceLocation_}
        {
        }

        TPSUIViewportHover(
            TPSDocumentElementID sceneElementID_,
            Vec3 const& worldspaceLocation_) :

            maybeSceneElementID{std::move(sceneElementID_)},
            worldspaceLocation{worldspaceLocation_}
        {
        }

        std::optional<TPSDocumentElementID> maybeSceneElementID = std::nullopt;
        Vec3 worldspaceLocation;
    };

    // the user's current selection
    struct TPSUIUserSelection final {

        void clear()
        {
            m_SelectedSceneElements.clear();
        }

        void select(TPSDocumentElementID el)
        {
            m_SelectedSceneElements.insert(std::move(el));
        }

        bool contains(TPSDocumentElementID const& el) const
        {
            return m_SelectedSceneElements.find(el) != m_SelectedSceneElements.end();
        }

        std::unordered_set<TPSDocumentElementID> const& getUnderlyingSet() const
        {
            return m_SelectedSceneElements;
        }

    private:
        std::unordered_set<TPSDocumentElementID> m_SelectedSceneElements;
    };

    // top-level UI state that is shared by all UI panels
    struct TPSUISharedState final {

        TPSUISharedState(
            osc::UID tabID_,
            osc::ParentPtr<osc::TabHost> parent_) :

            tabID{tabID_},
            tabHost{std::move(parent_)}
        {
        }

        // ID of the top-level TPS3D tab
        osc::UID tabID;

        // handle to the screen that owns the TPS3D tab
        osc::ParentPtr<osc::TabHost> tabHost;

        // cached TPS3D algorithm result (to prevent recomputing it each frame)
        TPSResultCache meshResultCache;

        // the document that the user is editing
        std::shared_ptr<UndoableTPSDocument> editedDocument = std::make_shared<UndoableTPSDocument>();

        // `true` if the user wants the cameras to be linked
        bool linkCameras = true;

        // `true` if `LinkCameras` should only link the rotational parts of the cameras
        bool onlyLinkRotation = false;

        // shared linked camera
        osc::PolarPerspectiveCamera linkedCameraBase = CreateCameraFocusedOn(editedDocument->getScratch().sourceMesh.getBounds());

        // wireframe material, used to draw scene elements in a wireframe style
        osc::Material wireframeMaterial = osc::CreateWireframeOverlayMaterial(
            osc::App::config(),
            *osc::App::singleton<osc::ShaderCache>()
        );

        // shared sphere mesh (used by rendering code)
        osc::Mesh landmarkSphere = osc::App::singleton<osc::SceneCache>()->getSphereMesh();

        // current user selection
        TPSUIUserSelection userSelection;

        // current user hover: reset per-frame
        std::optional<TPSUIViewportHover> currentHover;

        // currently active tab-wide popups
        osc::PopupManager popupManager;

        // shared mesh cache
        std::shared_ptr<osc::SceneCache> meshCache = osc::App::singleton<osc::SceneCache>();
    };

    TPSDocument const& getScratch(TPSUISharedState const& state)
    {
        return state.editedDocument->getScratch();
    }

    osc::Mesh const& GetScratchMesh(TPSUISharedState& state, TPSDocumentInputIdentifier which)
    {
        return GetMesh(getScratch(state), which);
    }

    osc::BVH const& GetScratchMeshBVH(TPSUISharedState& state, TPSDocumentInputIdentifier which)
    {
        osc::Mesh const& mesh = GetScratchMesh(state, which);
        return state.meshCache->getBVH(mesh);
    }

    // returns a (potentially cached) post-TPS-warp mesh
    osc::Mesh const& GetResultMesh(TPSUISharedState& state)
    {
        return state.meshResultCache.getWarpedMesh(state.editedDocument->getScratch());
    }

    std::span<Vec3 const> GetResultNonParticipatingLandmarks(TPSUISharedState& state)
    {
        return state.meshResultCache.getWarpedNonParticipatingLandmarks(state.editedDocument->getScratch());
    }

    // append decorations that are common to all panels to the given output vector
    void AppendCommonDecorations(
        TPSUISharedState const& sharedState,
        osc::Mesh const& tpsSourceOrDestinationMesh,
        bool wireframeMode,
        std::function<void(osc::SceneDecoration&&)> const& out,
        osc::Color meshColor = osc::Color::white())
    {
        // draw the mesh
        {
            osc::SceneDecoration dec{tpsSourceOrDestinationMesh};
            dec.color = meshColor;
            out(std::move(dec));
        }

        // if requested, also draw wireframe overlays for the mesh
        if (wireframeMode)
        {
            osc::SceneDecoration dec{tpsSourceOrDestinationMesh};
            dec.maybeMaterial = sharedState.wireframeMaterial;
            out(std::move(dec));
        }

        // add grid decorations
        DrawXZGrid(*sharedState.meshCache, out);
        DrawXZFloorLines(*sharedState.meshCache, out, 100.0f);
    }

    void AppendNonParticipatingLandmark(
        osc::Mesh const& landmarkSphereMesh,
        float baseLandmarkRadius,
        Vec3 const& nonParticipatingLandmarkPos,
        std::function<void(osc::SceneDecoration&&)> const& out)
    {
        osc::Transform transform{};
        transform.scale *= 0.75f*baseLandmarkRadius;
        transform.position = nonParticipatingLandmarkPos;

        out(osc::SceneDecoration{landmarkSphereMesh, transform, osc::Color::purple()});
    }
}


// UI: widgets that appear within panels in the UI
namespace
{
    // the top toolbar (contains icons for new, save, open, undo, redo, etc.)
    class TPS3DToolbar final {
    public:
        TPS3DToolbar(
            std::string_view label,
            std::shared_ptr<TPSUISharedState> tabState_) :

            m_Label{label},
            m_State{std::move(tabState_)}
        {
        }

        void onDraw()
        {
            if (osc::BeginToolbar(m_Label))
            {
                drawContent();
            }
            ImGui::End();
        }

    private:
        void drawContent()
        {
            // document-related stuff
            drawNewDocumentButton();
            ImGui::SameLine();
            drawOpenDocumentButton();
            ImGui::SameLine();
            drawSaveLandmarksButton();
            ImGui::SameLine();

            ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
            ImGui::SameLine();

            // undo/redo-related stuff
            m_UndoButton.onDraw();
            ImGui::SameLine();
            m_RedoButton.onDraw();
            ImGui::SameLine();

            ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
            ImGui::SameLine();

            // camera stuff
            drawCameraLockCheckbox();
            ImGui::SameLine();

            ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
            ImGui::SameLine();

            // landmark stuff
            drawResetLandmarksButton();
            ImGui::SameLine();

            ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
            ImGui::SameLine();

            drawResetNonParticipatingLandmarksButton();
        }

        void drawNewDocumentButton()
        {
            if (ImGui::Button(ICON_FA_FILE))
            {
                ActionCreateNewDocument(*m_State->editedDocument);
            }
            osc::DrawTooltipIfItemHovered(
                "Create New Document",
                "Creates the default scene (undoable)"
            );
        }

        void drawOpenDocumentButton()
        {
            ImGui::Button(ICON_FA_FOLDER_OPEN);
            if (ImGui::BeginPopupContextItem("##OpenFolder", ImGuiPopupFlags_MouseButtonLeft))
            {
                if (ImGui::MenuItem("Load Source Mesh"))
                {
                    ActionBrowseForNewMesh(*m_State->editedDocument, TPSDocumentInputIdentifier::Source);
                }
                if (ImGui::MenuItem("Load Destination Mesh"))
                {
                    ActionBrowseForNewMesh(*m_State->editedDocument, TPSDocumentInputIdentifier::Destination);
                }
                ImGui::EndPopup();
            }
            osc::DrawTooltipIfItemHovered(
                "Open File",
                "Open Source/Destination data"
            );
        }

        void drawSaveLandmarksButton()
        {
            if (ImGui::Button(ICON_FA_SAVE))
            {
                ActionSaveLandmarksToPairedCSV(getScratch(*m_State));
            }
            osc::DrawTooltipIfItemHovered(
                "Save Landmarks to CSV",
                "Saves all pair-able landmarks to a CSV file, for external processing"
            );
        }

        void drawCameraLockCheckbox()
        {
            ImGui::Checkbox("link cameras", &m_State->linkCameras);
            ImGui::SameLine();
            ImGui::Checkbox("only link rotation", &m_State->onlyLinkRotation);
        }

        void drawResetLandmarksButton()
        {
            bool const hasLandmarks =
                !m_State->editedDocument->getScratch().landmarkPairs.empty();

            if (!hasLandmarks)
            {
                ImGui::BeginDisabled();
            }

            if (ImGui::Button(ICON_FA_ERASER " clear landmarks"))
            {
                ActionClearAllLandmarks(*m_State->editedDocument);
            }

            if (!hasLandmarks)
            {
                ImGui::EndDisabled();
            }
        }

        void drawResetNonParticipatingLandmarksButton()
        {
            bool const hasNonParticipatingLandmarks =
                !m_State->editedDocument->getScratch().nonParticipatingLandmarks.empty();

            if (!hasNonParticipatingLandmarks)
            {
                ImGui::BeginDisabled();
            }

            if (ImGui::Button(ICON_FA_ERASER " clear non-participating landmarks"))
            {
                ActionClearNonParticipatingLandmarks(*m_State->editedDocument);
            }

            if (!hasNonParticipatingLandmarks)
            {
                ImGui::EndDisabled();
            }
        }

        std::string m_Label;
        std::shared_ptr<TPSUISharedState> m_State;
        osc::UndoButton m_UndoButton{m_State->editedDocument};
        osc::RedoButton m_RedoButton{m_State->editedDocument};
    };

    // widget: bottom status bar (shows status messages, hover information, etc.)
    class TPS3DStatusBar final {
    public:
        TPS3DStatusBar(
            std::string_view label,
            std::shared_ptr<TPSUISharedState> tabState_) :

            m_Label{label},
            m_State{std::move(tabState_)}
        {
        }

        void onDraw()
        {
            if (osc::BeginMainViewportBottomBar(m_Label))
            {
                drawContent();
            }
            ImGui::End();
        }

    private:
        void drawContent()
        {
            if (m_State->currentHover)
            {
                drawCurrentHoverInfo(*m_State->currentHover);
            }
            else
            {
                ImGui::TextDisabled("(nothing hovered)");
            }
        }

        void drawCurrentHoverInfo(TPSUIViewportHover const& hover)
        {
            drawColorCodedXYZ(hover.worldspaceLocation);
            ImGui::SameLine();
            if (hover.maybeSceneElementID)
            {
                ImGui::TextDisabled("(left-click to select %s)", hover.maybeSceneElementID->elementID.c_str());
            }
            else
            {
                ImGui::TextDisabled("(left-click to add a landmark)");
            }
        }

        void drawColorCodedXYZ(Vec3 pos)
        {
            ImGui::TextUnformatted("(");
            ImGui::SameLine();
            for (int i = 0; i < 3; ++i)
            {
                osc::Color color = {0.5f, 0.5f, 0.5f, 1.0f};
                color[i] = 1.0f;

                osc::PushStyleColor(ImGuiCol_Text, color);
                ImGui::Text("%f", pos[i]);
                ImGui::SameLine();
                osc::PopStyleColor();
            }
            ImGui::TextUnformatted(")");
        }

        std::string m_Label;
        std::shared_ptr<TPSUISharedState> m_State;
    };

    // widget: the 'file' menu (a sub menu of the main menu)
    class TPS3DFileMenu final {
    public:
        explicit TPS3DFileMenu(std::shared_ptr<TPSUISharedState> tabState_) :
            m_State{std::move(tabState_)}
        {
        }

        void onDraw()
        {
            if (ImGui::BeginMenu("File"))
            {
                drawContent();
                ImGui::EndMenu();
            }
        }
    private:
        void drawContent()
        {
            if (ImGui::MenuItem(ICON_FA_FILE " New"))
            {
                ActionCreateNewDocument(*m_State->editedDocument);
            }

            if (ImGui::BeginMenu(ICON_FA_FILE_IMPORT " Import"))
            {
                drawImportMenuContent();
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu(ICON_FA_FILE_EXPORT " Export"))
            {
                drawExportMenuContent();
                ImGui::EndMenu();
            }

            if (ImGui::MenuItem(ICON_FA_TIMES " Close"))
            {
                m_State->tabHost->closeTab(m_State->tabID);
            }

            if (ImGui::MenuItem(ICON_FA_TIMES_CIRCLE " Quit"))
            {
                osc::App::upd().requestQuit();
            }
        }

        void drawImportMenuContent()
        {
            if (ImGui::MenuItem("Source Mesh"))
            {
                ActionBrowseForNewMesh(*m_State->editedDocument, TPSDocumentInputIdentifier::Source);
            }
            if (ImGui::MenuItem("Destination Mesh"))
            {
                ActionBrowseForNewMesh(*m_State->editedDocument, TPSDocumentInputIdentifier::Destination);
            }
            if (ImGui::MenuItem("Source Landmarks from CSV"))
            {
                ActionLoadLandmarksCSV(*m_State->editedDocument, TPSDocumentInputIdentifier::Source);
            }
            if (ImGui::MenuItem("Destination Landmarks from CSV"))
            {
                ActionLoadLandmarksCSV(*m_State->editedDocument, TPSDocumentInputIdentifier::Destination);
            }
            if (ImGui::MenuItem("Non-Participating Landmarks from CSV"))
            {
                ActionLoadNonParticipatingPointsCSV(*m_State->editedDocument);
            }
        }

        void drawExportMenuContent()
        {
            if (ImGui::MenuItem("Source Landmarks to CSV"))
            {
                ActionSaveLandmarksToCSV(getScratch(*m_State), TPSDocumentInputIdentifier::Source);
            }
            if (ImGui::MenuItem("Destination Landmarks to CSV"))
            {
                ActionSaveLandmarksToCSV(getScratch(*m_State), TPSDocumentInputIdentifier::Destination);
            }
            if (ImGui::MenuItem("Landmark Pairs to CSV"))
            {
                ActionSaveLandmarksToPairedCSV(getScratch(*m_State));
            }
        }

        std::shared_ptr<TPSUISharedState> m_State;
    };

    // widget: the 'edit' menu (a sub menu of the main menu)
    class TPS3DEditMenu final {
    public:
        explicit TPS3DEditMenu(std::shared_ptr<TPSUISharedState> tabState_) :
            m_State{std::move(tabState_)}
        {
        }

        void onDraw()
        {
            if (ImGui::BeginMenu("Edit"))
            {
                drawContent();
                ImGui::EndMenu();
            }
        }

    private:

        void drawContent()
        {
            if (ImGui::MenuItem("Undo", nullptr, nullptr, m_State->editedDocument->canUndo()))
            {
                ActionUndo(*m_State->editedDocument);
            }
            if (ImGui::MenuItem("Redo", nullptr, nullptr, m_State->editedDocument->canRedo()))
            {
                ActionRedo(*m_State->editedDocument);
            }
        }

        std::shared_ptr<TPSUISharedState> m_State;
    };

    // widget: the main menu (contains multiple submenus: 'file', 'edit', 'about', etc.)
    class TPS3DMainMenu final {
    public:
        explicit TPS3DMainMenu(
            std::shared_ptr<TPSUISharedState> const& tabState_,
            std::shared_ptr<osc::PanelManager> const& panelManager_) :
            m_FileMenu{tabState_},
            m_EditMenu{tabState_},
            m_WindowMenu{panelManager_}
        {
        }

        void onDraw()
        {
            m_FileMenu.onDraw();
            m_EditMenu.onDraw();
            m_WindowMenu.onDraw();
            m_AboutTab.onDraw();
        }
    private:
        TPS3DFileMenu m_FileMenu;
        TPS3DEditMenu m_EditMenu;
        osc::WindowMenu m_WindowMenu;
        osc::MainMenuAboutTab m_AboutTab;
    };
}

// TPS3D UI panel implementations
//
// implementation code for each panel shown in the UI
namespace
{
    // generic base class for the panels shown in the TPS3D tab
    class MeshWarpingTabPanel : public osc::StandardPanel {
    public:
        using osc::StandardPanel::StandardPanel;

    private:
        void implBeforeImGuiBegin() final
        {
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {0.0f, 0.0f});
        }
        void implAfterImGuiBegin() final
        {
            ImGui::PopStyleVar();
        }
    };

    // an "input" panel (i.e. source or destination mesh, before warping)
    class TPS3DInputPanel final : public MeshWarpingTabPanel {
    public:
        TPS3DInputPanel(
            std::string_view panelName_,
            std::shared_ptr<TPSUISharedState> state_,
            TPSDocumentInputIdentifier documentIdentifier_) :

            MeshWarpingTabPanel{panelName_, ImGuiDockNodeFlags_PassthruCentralNode},
            m_State{std::move(state_)},
            m_DocumentIdentifier{documentIdentifier_}
        {
            OSC_ASSERT(m_State != nullptr && "the input panel requires a valid sharedState state");
        }

    private:
        // draws all of the panel's content
        void implDrawContent() final
        {
            // compute top-level UI variables (render rect, mouse pos, etc.)
            osc::Rect const contentRect = osc::ContentRegionAvailScreenRect();
            Vec2 const contentRectDims = osc::Dimensions(contentRect);
            Vec2 const mousePos = ImGui::GetMousePos();
            osc::Line const cameraRay = m_Camera.unprojectTopLeftPosToWorldRay(mousePos - contentRect.p1, osc::Dimensions(contentRect));

            // mesh hittest: compute whether the user is hovering over the mesh (affects rendering)
            osc::Mesh const& inputMesh = GetScratchMesh(*m_State, m_DocumentIdentifier);
            osc::BVH const& inputMeshBVH = GetScratchMeshBVH(*m_State, m_DocumentIdentifier);
            std::optional<osc::RayCollision> const meshCollision = m_LastTextureHittestResult.isHovered ?
                osc::GetClosestWorldspaceRayCollision(inputMesh, inputMeshBVH, osc::Transform{}, cameraRay) :
                std::nullopt;

            // landmark hittest: compute whether the user is hovering over a landmark
            std::optional<TPSUIViewportHover> landmarkCollision = m_LastTextureHittestResult.isHovered ?
                getMouseLandmarkCollisions(cameraRay) :
                std::nullopt;

            // hover state: update central hover state
            if (landmarkCollision)
            {
                // update central state to tell it that there's a new hover
                m_State->currentHover = landmarkCollision;
            }
            else if (meshCollision)
            {
                m_State->currentHover.emplace(meshCollision->position);
            }

            // ensure the camera is updated *before* rendering; otherwise, it'll be one frame late
            updateCamera();

            // render: draw the scene into the content rect and hittest it
            osc::RenderTexture& renderTexture = renderScene(contentRectDims, meshCollision, landmarkCollision);
            osc::DrawTextureAsImGuiImage(renderTexture);
            m_LastTextureHittestResult = osc::HittestLastImguiItem();

            // handle any events due to hovering over, clicking, etc.
            handleInputAndHoverEvents(m_LastTextureHittestResult, meshCollision, landmarkCollision);

            // draw any 2D ImGui overlays
            drawOverlays(m_LastTextureHittestResult.rect);
        }

        void updateCamera()
        {
            // if the cameras are linked together, ensure this camera is updated from the linked camera
            if (m_State->linkCameras && m_Camera != m_State->linkedCameraBase)
            {
                if (m_State->onlyLinkRotation)
                {
                    m_Camera.phi = m_State->linkedCameraBase.phi;
                    m_Camera.theta = m_State->linkedCameraBase.theta;
                }
                else
                {
                    m_Camera = m_State->linkedCameraBase;
                }
            }

            // if the user interacts with the render, update the camera as necessary
            if (m_LastTextureHittestResult.isHovered)
            {
                if (osc::UpdatePolarCameraFromImGuiMouseInputs(m_Camera, osc::Dimensions(m_LastTextureHittestResult.rect)))
                {
                    m_State->linkedCameraBase = m_Camera;  // reflects latest modification
                }
            }
        }

        // returns the closest collision, if any, between the provided camera ray and a landmark
        std::optional<TPSUIViewportHover> getMouseLandmarkCollisions(osc::Line const& cameraRay) const
        {
            std::optional<TPSUIViewportHover> rv;
            for (TPSDocumentLandmarkPair const& p : getScratch(*m_State).landmarkPairs)
            {
                std::optional<Vec3> const maybePos = GetLocation(p, m_DocumentIdentifier);

                if (!maybePos)
                {
                    // doesn't have a source/destination landmark
                    continue;
                }
                // else: hittest the landmark as a sphere

                std::optional<osc::RayCollision> const coll = osc::GetRayCollisionSphere(cameraRay, osc::Sphere{*maybePos, m_LandmarkRadius});
                if (coll)
                {
                    if (!rv || osc::Length(rv->worldspaceLocation - cameraRay.origin) > coll->distance)
                    {
                        TPSDocumentElementID fullID{m_DocumentIdentifier, TPSDocumentInputElementType::Landmark, p.id};
                        rv.emplace(std::move(fullID), *maybePos);
                    }
                }
            }
            return rv;
        }

        void handleInputAndHoverEvents(
            osc::ImGuiItemHittestResult const& htResult,
            std::optional<osc::RayCollision> const& meshCollision,
            std::optional<TPSUIViewportHover> const& landmarkCollision)
        {
            // event: if the user left-clicks and something is hovered, select it; otherwise, add a landmark
            if (htResult.isLeftClickReleasedWithoutDragging)
            {
                if (landmarkCollision && landmarkCollision->maybeSceneElementID)
                {
                    if (!osc::IsShiftDown())
                    {
                        m_State->userSelection.clear();
                    }
                    m_State->userSelection.select(*landmarkCollision->maybeSceneElementID);
                }
                else if (meshCollision)
                {
                    ActionAddLandmarkTo(
                        *m_State->editedDocument,
                        m_DocumentIdentifier,
                        meshCollision->position
                    );
                }
            }

            // event: if the user is hovering the render while something is selected and the user
            // presses delete then the landmarks should be deleted
            if (htResult.isHovered && osc::IsAnyKeyPressed({ImGuiKey_Delete, ImGuiKey_Backspace}))
            {
                ActionDeleteSceneElementsByID(
                    *m_State->editedDocument,
                    m_State->userSelection.getUnderlyingSet()
                );
                m_State->userSelection.clear();
            }
        }

        // draws 2D ImGui overlays over the scene render
        void drawOverlays(osc::Rect const& renderRect)
        {
            ImGui::SetCursorScreenPos(renderRect.p1 + c_OverlayPadding);

            drawInformationIcon();
            ImGui::SameLine();
            drawImportButton();
            ImGui::SameLine();
            drawExportButton();
            ImGui::SameLine();
            drawAutoFitCameraButton();
            ImGui::SameLine();
            drawLandmarkRadiusSlider();
        }

        // draws a information icon that shows basic mesh info when hovered
        void drawInformationIcon()
        {
            osc::ButtonNoBg(ICON_FA_INFO_CIRCLE);
            if (ImGui::IsItemHovered())
            {
                osc::BeginTooltip();

                ImGui::TextDisabled("Input Information:");
                drawInformationTable();

                osc::EndTooltip();
            }
        }

        // draws a table containing useful input information (handy for debugging)
        void drawInformationTable()
        {
            if (ImGui::BeginTable("##inputinfo", 2))
            {
                ImGui::TableSetupColumn("Name");
                ImGui::TableSetupColumn("Value");

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("# landmarks");
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("%zu", CountNumLandmarksForInput(getScratch(*m_State), m_DocumentIdentifier));

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("# verts");
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("%zu", GetScratchMesh(*m_State, m_DocumentIdentifier).getVerts().size());

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("# triangles");
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("%zu", GetScratchMesh(*m_State, m_DocumentIdentifier).getIndices().size()/3);

                ImGui::EndTable();
            }
        }

        // draws an import button that enables the user to import things for this input
        void drawImportButton()
        {
            ImGui::Button(ICON_FA_FILE_IMPORT " import" ICON_FA_CARET_DOWN);
            if (ImGui::BeginPopupContextItem("##importcontextmenu", ImGuiPopupFlags_MouseButtonLeft))
            {
                if (ImGui::MenuItem("Mesh"))
                {
                    ActionBrowseForNewMesh(*m_State->editedDocument, m_DocumentIdentifier);
                }
                if (ImGui::MenuItem("Landmarks from CSV"))
                {
                    ActionLoadLandmarksCSV(*m_State->editedDocument, m_DocumentIdentifier);
                }
                if (m_DocumentIdentifier == TPSDocumentInputIdentifier::Source &&
                    ImGui::MenuItem("Non-Participating Landmarks from CSV"))
                {
                    ActionLoadNonParticipatingPointsCSV(*m_State->editedDocument);
                }
                ImGui::EndPopup();
            }
        }

        // draws an export button that enables the user to export things from this input
        void drawExportButton()
        {
            ImGui::Button(ICON_FA_FILE_EXPORT " export" ICON_FA_CARET_DOWN);
            if (ImGui::BeginPopupContextItem("##exportcontextmenu", ImGuiPopupFlags_MouseButtonLeft))
            {
                if (ImGui::MenuItem("Mesh to OBJ"))
                {
                    ActionTrySaveMeshToObj(GetScratchMesh(*m_State, m_DocumentIdentifier));
                }
                if (ImGui::MenuItem("Mesh to STL"))
                {
                    ActionTrySaveMeshToStl(GetScratchMesh(*m_State, m_DocumentIdentifier));
                }
                if (ImGui::MenuItem("Landmarks to CSV"))
                {
                    ActionSaveLandmarksToCSV(getScratch(*m_State), m_DocumentIdentifier);
                }
                ImGui::EndPopup();
            }
        }

        // draws a button that auto-fits the camera to the 3D scene
        void drawAutoFitCameraButton()
        {
            if (ImGui::Button(ICON_FA_EXPAND_ARROWS_ALT))
            {
                osc::AutoFocus(m_Camera, GetScratchMesh(*m_State, m_DocumentIdentifier).getBounds(), osc::AspectRatio(m_LastTextureHittestResult.rect));
                m_State->linkedCameraBase = m_Camera;
            }
            osc::DrawTooltipIfItemHovered("Autoscale Scene", "Zooms camera to try and fit everything in the scene into the viewer");
        }

        // draws a slider that lets the user edit how large the landmarks are
        void drawLandmarkRadiusSlider()
        {
            // note: log scale is important: some users have meshes that
            // are in different scales (e.g. millimeters)
            ImGuiSliderFlags const flags = ImGuiSliderFlags_Logarithmic;

            osc::CStringView const label = "landmark radius";
            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - ImGui::CalcTextSize(label.c_str()).x - ImGui::GetStyle().ItemInnerSpacing.x - c_OverlayPadding.x);
            ImGui::SliderFloat(label.c_str(), &m_LandmarkRadius, 0.0001f, 100.0f, "%.4f", flags);
        }

        // renders this panel's 3D scene to a texture
        osc::RenderTexture& renderScene(
            Vec2 dims,
            std::optional<osc::RayCollision> const& maybeMeshCollision,
            std::optional<TPSUIViewportHover> const& maybeLandmarkCollision)
        {
            osc::SceneRendererParams const params = CalcStandardDarkSceneRenderParams(
                m_Camera,
                osc::App::get().getCurrentAntiAliasingLevel(),
                dims
            );
            std::vector<osc::SceneDecoration> const decorations = generateDecorations(maybeMeshCollision, maybeLandmarkCollision);
            return m_CachedRenderer.render(decorations, params);
        }

        // returns a fresh list of 3D decorations for this panel's 3D render
        std::vector<osc::SceneDecoration> generateDecorations(
            std::optional<osc::RayCollision> const& maybeMeshCollision,
            std::optional<TPSUIViewportHover> const& maybeLandmarkCollision) const
        {
            // generate in-scene 3D decorations
            std::vector<osc::SceneDecoration> decorations;
            decorations.reserve(6 + CountNumLandmarksForInput(getScratch(*m_State), m_DocumentIdentifier));  // likely guess

            std::function<void(osc::SceneDecoration&&)> const decorationConsumer =
                [&decorations](osc::SceneDecoration&& dec) { decorations.push_back(std::move(dec)); };

            AppendCommonDecorations(
                *m_State,
                GetScratchMesh(*m_State, m_DocumentIdentifier),
                m_WireframeMode,
                decorationConsumer
            );

            // append each landmark as a sphere
            for (TPSDocumentLandmarkPair const& p : getScratch(*m_State).landmarkPairs)
            {
                std::optional<Vec3> const maybeLocation = GetLocation(p, m_DocumentIdentifier);

                if (!maybeLocation)
                {
                    continue;  // no source/destination location for the landmark
                }

                TPSDocumentElementID fullID{m_DocumentIdentifier, TPSDocumentInputElementType::Landmark, p.id};

                osc::Transform transform{};
                transform.scale *= m_LandmarkRadius;
                transform.position = *maybeLocation;

                osc::Color const& color = IsFullyPaired(p) ? c_PairedLandmarkColor : c_UnpairedLandmarkColor;

                osc::SceneDecoration& decoration = decorations.emplace_back(m_State->landmarkSphere, transform, color);

                if (m_State->userSelection.contains(fullID))
                {
                    Vec4 tmpColor = decoration.color;
                    tmpColor += Vec4{0.25f, 0.25f, 0.25f, 0.0f};
                    tmpColor = osc::Clamp(tmpColor, 0.0f, 1.0f);

                    decoration.color = osc::Color{tmpColor};
                    decoration.flags = osc::SceneDecorationFlags::IsSelected;
                }
                else if (m_State->currentHover && m_State->currentHover->maybeSceneElementID == fullID)
                {
                    Vec4 tmpColor = decoration.color;
                    tmpColor += Vec4{0.15f, 0.15f, 0.15f, 0.0f};
                    tmpColor = osc::Clamp(tmpColor, 0.0f, 1.0f);

                    decoration.color = osc::Color{tmpColor};
                    decoration.flags = osc::SceneDecorationFlags::IsHovered;
                }
            }

            // append non-participating landmarks as non-user-selctable purple spheres
            if (m_DocumentIdentifier == TPSDocumentInputIdentifier::Source)
            {
                for (Vec3 const& nonParticipatingLandmarkLocation : getScratch(*m_State).nonParticipatingLandmarks)
                {
                    AppendNonParticipatingLandmark(
                        m_State->landmarkSphere,
                        m_LandmarkRadius,
                        nonParticipatingLandmarkLocation,
                        decorationConsumer
                    );
                }
            }

            // if applicable, show mouse-to-mesh collision as faded landmark as a placement hint for user
            if (maybeMeshCollision && !maybeLandmarkCollision)
            {
                osc::Transform transform{};
                transform.scale *= m_LandmarkRadius;
                transform.position = maybeMeshCollision->position;

                osc::Color color = c_UnpairedLandmarkColor;
                color.a *= 0.25f;

                decorations.emplace_back(m_State->landmarkSphere, transform, color);
            }

            return decorations;
        }

        std::shared_ptr<TPSUISharedState> m_State;
        TPSDocumentInputIdentifier m_DocumentIdentifier;
        osc::PolarPerspectiveCamera m_Camera = CreateCameraFocusedOn(GetScratchMesh(*m_State, m_DocumentIdentifier).getBounds());
        osc::CachedSceneRenderer m_CachedRenderer
        {
            osc::App::config(),
            *osc::App::singleton<osc::SceneCache>(),
            *osc::App::singleton<osc::ShaderCache>(),
        };
        osc::ImGuiItemHittestResult m_LastTextureHittestResult;
        bool m_WireframeMode = true;
        float m_LandmarkRadius = 0.05f;
    };

    // a "result" panel (i.e. after applying a warp to the source)
    class TPS3DResultPanel final : public MeshWarpingTabPanel {
    public:

        TPS3DResultPanel(
            std::string_view panelName_,
            std::shared_ptr<TPSUISharedState> state_) :

            MeshWarpingTabPanel{panelName_},
            m_State{std::move(state_)}
        {
            OSC_ASSERT(m_State != nullptr && "the input panel requires a valid sharedState state");
        }

    private:
        void implDrawContent() final
        {
            // fill the entire available region with the render
            Vec2 const dims = ImGui::GetContentRegionAvail();

            updateCamera();

            // render it via ImGui and hittest it
            osc::RenderTexture& renderTexture = renderScene(dims);
            osc::DrawTextureAsImGuiImage(renderTexture);
            m_LastTextureHittestResult = osc::HittestLastImguiItem();

            drawOverlays(m_LastTextureHittestResult.rect);
        }

        void updateCamera()
        {
            // if cameras are linked together, ensure all cameras match the "base" camera
            if (m_State->linkCameras && m_Camera != m_State->linkedCameraBase)
            {
                if (m_State->onlyLinkRotation)
                {
                    m_Camera.phi = m_State->linkedCameraBase.phi;
                    m_Camera.theta = m_State->linkedCameraBase.theta;
                }
                else
                {
                    m_Camera = m_State->linkedCameraBase;
                }
            }

            // update camera if user drags it around etc.
            if (m_LastTextureHittestResult.isHovered)
            {
                if (osc::UpdatePolarCameraFromImGuiMouseInputs(m_Camera, osc::Dimensions(m_LastTextureHittestResult.rect)))
                {
                    m_State->linkedCameraBase = m_Camera;  // reflects latest modification
                }
            }
        }

        // draw ImGui overlays over a result panel
        void drawOverlays(osc::Rect const& renderRect)
        {
            // ImGui: set cursor to draw over the top-right of the render texture (with padding)
            ImGui::SetCursorScreenPos(renderRect.p1 + m_OverlayPadding);

            drawInformationIcon();
            ImGui::SameLine();
            drawExportButton();
            ImGui::SameLine();
            drawAutoFitCameraButton();
            ImGui::SameLine();
            ImGui::Checkbox("show destination", &m_ShowDestinationMesh);
            ImGui::SameLine();
            drawLandmarkRadiusSlider();
            drawBlendingFactorSlider();
        }

        // draws a information icon that shows basic mesh info when hovered
        void drawInformationIcon()
        {
            osc::ButtonNoBg(ICON_FA_INFO_CIRCLE);
            if (ImGui::IsItemHovered())
            {
                osc::BeginTooltip();

                ImGui::TextDisabled("Result Information:");
                drawInformationTable();

                osc::EndTooltip();
            }
        }

        // draws a table containing useful input information (handy for debugging)
        void drawInformationTable()
        {
            if (ImGui::BeginTable("##inputinfo", 2))
            {
                ImGui::TableSetupColumn("Name");
                ImGui::TableSetupColumn("Value");

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("# verts");
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("%zu", GetResultMesh(*m_State).getVerts().size());

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("# triangles");
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("%zu", GetResultMesh(*m_State).getIndices().size()/3);

                ImGui::EndTable();
            }
        }

        // draws an export button that enables the user to export things from this input
        void drawExportButton()
        {
            m_CursorXAtExportButton = ImGui::GetCursorPos().x;  // needed to align the blending factor slider
            ImGui::Button(ICON_FA_FILE_EXPORT " export" ICON_FA_CARET_DOWN);
            if (ImGui::BeginPopupContextItem("##exportcontextmenu", ImGuiPopupFlags_MouseButtonLeft))
            {
                if (ImGui::MenuItem("Mesh to OBJ"))
                {
                    ActionTrySaveMeshToObj(GetResultMesh(*m_State));
                }
                if (ImGui::MenuItem("Mesh to STL"))
                {
                    ActionTrySaveMeshToStl(GetResultMesh(*m_State));
                }
                if (ImGui::MenuItem("Non-Participating Landmarks to CSV"))
                {
                    ActionTrySaveWarpedNonParticipatingLandmarksToCSV(GetResultNonParticipatingLandmarks(*m_State));
                }
                ImGui::EndPopup();
            }
        }

        // draws a button that auto-fits the camera to the 3D scene
        void drawAutoFitCameraButton()
        {
            if (ImGui::Button(ICON_FA_EXPAND_ARROWS_ALT))
            {
                osc::AutoFocus(
                    m_Camera,
                    GetResultMesh(*m_State).getBounds(),
                    AspectRatio(m_LastTextureHittestResult.rect)
                );
                m_State->linkedCameraBase = m_Camera;
            }
            osc::DrawTooltipIfItemHovered(
                "Autoscale Scene",
                "Zooms camera to try and fit everything in the scene into the viewer"
            );
        }

        // draws a slider that lets the user edit how large the landmarks are
        void drawLandmarkRadiusSlider()
        {
            // note: log scale is important: some users have meshes that
            // are in different scales (e.g. millimeters)
            ImGuiSliderFlags const flags = ImGuiSliderFlags_Logarithmic;

            osc::CStringView const label = "landmark radius";
            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - ImGui::CalcTextSize(label.c_str()).x - ImGui::GetStyle().ItemInnerSpacing.x - c_OverlayPadding.x);
            ImGui::SliderFloat(label.c_str(), &m_LandmarkRadius, 0.0001f, 100.0f, "%.4f", flags);
        }

        void drawBlendingFactorSlider()
        {
            ImGui::SetCursorPosX(m_CursorXAtExportButton);  // align with "export" button in row above

            osc::CStringView const label = "blending factor  ";  // deliberate trailing spaces (for alignment with "landmark radius")
            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - ImGui::CalcTextSize(label.c_str()).x - ImGui::GetStyle().ItemInnerSpacing.x - m_OverlayPadding.x);

            float factor = getScratch(*m_State).blendingFactor;
            if (ImGui::SliderFloat(label.c_str(), &factor, 0.0f, 1.0f))
            {
                ActionSetBlendFactorWithoutSaving(*m_State->editedDocument, factor);
            }
            if (ImGui::IsItemDeactivatedAfterEdit())
            {
                ActionSetBlendFactorAndSave(*m_State->editedDocument, factor);
            }
        }

        // returns 3D decorations for the given result panel
        std::vector<osc::SceneDecoration> generateDecorations() const
        {
            std::vector<osc::SceneDecoration> decorations;
            std::function<void(osc::SceneDecoration&&)> const decorationConsumer =
                [&decorations](osc::SceneDecoration&& dec) { decorations.push_back(std::move(dec)); };

            AppendCommonDecorations(
                *m_State,
                GetResultMesh(*m_State),
                m_WireframeMode,
                decorationConsumer
            );

            if (m_ShowDestinationMesh)
            {
                osc::SceneDecoration& dec = decorations.emplace_back(getScratch(*m_State).destinationMesh);
                dec.color = {1.0f, 0.0f, 0.0f, 0.5f};
            }

            // draw non-participating landmarks
            for (Vec3 const& nonParticipatingLandmarkPos : GetResultNonParticipatingLandmarks(*m_State))
            {
                AppendNonParticipatingLandmark(
                    m_State->landmarkSphere,
                    m_LandmarkRadius,
                    nonParticipatingLandmarkPos,
                    decorationConsumer
                );
            }

            return decorations;
        }

        // renders a panel to a texture via its renderer and returns a reference to the rendered texture
        osc::RenderTexture& renderScene(Vec2 dims)
        {
            std::vector<osc::SceneDecoration> const decorations = generateDecorations();
            osc::SceneRendererParams const params = CalcStandardDarkSceneRenderParams(
                m_Camera,
                osc::App::get().getCurrentAntiAliasingLevel(),
                dims
            );
            return m_CachedRenderer.render(decorations, params);
        }

        std::shared_ptr<TPSUISharedState> m_State;
        osc::PolarPerspectiveCamera m_Camera = CreateCameraFocusedOn(GetResultMesh(*m_State).getBounds());
        osc::CachedSceneRenderer m_CachedRenderer
        {
            osc::App::config(),
            *osc::App::singleton<osc::SceneCache>(),
            *osc::App::singleton<osc::ShaderCache>(),
        };
        osc::ImGuiItemHittestResult m_LastTextureHittestResult;
        bool m_WireframeMode = true;
        bool m_ShowDestinationMesh = false;
        Vec2 m_OverlayPadding = {10.0f, 10.0f};
        float m_LandmarkRadius = 0.05f;
        float m_CursorXAtExportButton = 0.0f;
    };

    // pushes all available panels the TPS3D tab can render into the out param
    void PushBackAvailablePanels(std::shared_ptr<TPSUISharedState> const& state, osc::PanelManager& out)
    {
        out.registerToggleablePanel(
            "Source Mesh",
            [state](std::string_view panelName) { return std::make_shared<TPS3DInputPanel>(panelName, state, TPSDocumentInputIdentifier::Source); }
        );

        out.registerToggleablePanel(
            "Destination Mesh",
            [state](std::string_view panelName) { return std::make_shared<TPS3DInputPanel>(panelName, state, TPSDocumentInputIdentifier::Destination); }
        );

        out.registerToggleablePanel(
            "Result",
            [state](std::string_view panelName) { return std::make_shared<TPS3DResultPanel>(panelName, state); }
        );

        out.registerToggleablePanel(
            "History",
            [state](std::string_view panelName) { return std::make_shared<osc::UndoRedoPanel>(panelName, state->editedDocument); },
            osc::ToggleablePanelFlags::Default - osc::ToggleablePanelFlags::IsEnabledByDefault
        );

        out.registerToggleablePanel(
            "Log",
            [](std::string_view panelName) { return std::make_shared<osc::LogViewerPanel>(panelName); },
            osc::ToggleablePanelFlags::Default - osc::ToggleablePanelFlags::IsEnabledByDefault
        );

        out.registerToggleablePanel(
            "Performance",
            [](std::string_view panelName) { return std::make_shared<osc::PerfPanel>(panelName); },
            osc::ToggleablePanelFlags::Default - osc::ToggleablePanelFlags::IsEnabledByDefault
        );
    }
}

// top-level tab implementation
class osc::MeshWarpingTab::Impl final {
public:

    explicit Impl(ParentPtr<TabHost> const& parent_) : m_Parent{parent_}
    {
        PushBackAvailablePanels(m_SharedState, *m_PanelManager);
    }

    UID getID() const
    {
        return m_TabID;
    }

    CStringView getName() const
    {
        return ICON_FA_BEZIER_CURVE " Mesh Warping";
    }

    void onMount()
    {
        App::upd().makeMainEventLoopWaiting();
        m_PanelManager->onMount();
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
        // re-perform hover test each frame
        m_SharedState->currentHover.reset();

        // garbage collect panel data
        m_PanelManager->onTick();
    }

    void onDrawMainMenu()
    {
        m_MainMenu.onDraw();
    }

    void onDraw()
    {
        ImGui::DockSpaceOverViewport(ImGui::GetMainViewport(), ImGuiDockNodeFlags_PassthruCentralNode);

        m_TopToolbar.onDraw();
        m_PanelManager->onDraw();
        m_StatusBar.onDraw();

        // draw active popups over the UI
        m_SharedState->popupManager.onDraw();
    }

private:
    bool onKeydownEvent(SDL_KeyboardEvent const& e)
    {
        bool const ctrlOrSuperDown = osc::IsCtrlOrSuperDown();

        if (ctrlOrSuperDown && e.keysym.mod & KMOD_SHIFT && e.keysym.sym == SDLK_z)
        {
            // Ctrl+Shift+Z: redo
            ActionRedo(*m_SharedState->editedDocument);
            return true;
        }
        else if (ctrlOrSuperDown && e.keysym.sym == SDLK_z)
        {
            // Ctrl+Z: undo
            ActionUndo(*m_SharedState->editedDocument);
            return true;
        }
        else
        {
            return false;
        }
    }

    UID m_TabID;
    ParentPtr<TabHost> m_Parent;

    // top-level state that all panels can potentially access
    std::shared_ptr<TPSUISharedState> m_SharedState = std::make_shared<TPSUISharedState>(m_TabID, m_Parent);

    // available/active panels that the user can toggle via the `window` menu
    std::shared_ptr<PanelManager> m_PanelManager = std::make_shared<PanelManager>();

    // not-user-toggleable widgets
    TPS3DMainMenu m_MainMenu{m_SharedState, m_PanelManager};
    TPS3DToolbar m_TopToolbar{"##TPS3DToolbar", m_SharedState};
    TPS3DStatusBar m_StatusBar{"##TPS3DStatusBar", m_SharedState};
};


// public API (PIMPL)

osc::CStringView osc::MeshWarpingTab::id()
{
    return "OpenSim/Warping";
}

osc::MeshWarpingTab::MeshWarpingTab(ParentPtr<TabHost> const& parent_) :
    m_Impl{std::make_unique<Impl>(parent_)}
{
}

osc::MeshWarpingTab::MeshWarpingTab(MeshWarpingTab&&) noexcept = default;
osc::MeshWarpingTab& osc::MeshWarpingTab::operator=(MeshWarpingTab&&) noexcept = default;
osc::MeshWarpingTab::~MeshWarpingTab() noexcept = default;

osc::UID osc::MeshWarpingTab::implGetID() const
{
    return m_Impl->getID();
}

osc::CStringView osc::MeshWarpingTab::implGetName() const
{
    return m_Impl->getName();
}

void osc::MeshWarpingTab::implOnMount()
{
    m_Impl->onMount();
}

void osc::MeshWarpingTab::implOnUnmount()
{
    m_Impl->onUnmount();
}

bool osc::MeshWarpingTab::implOnEvent(SDL_Event const& e)
{
    return m_Impl->onEvent(e);
}

void osc::MeshWarpingTab::implOnTick()
{
    m_Impl->onTick();
}

void osc::MeshWarpingTab::implOnDrawMainMenu()
{
    m_Impl->onDrawMainMenu();
}

void osc::MeshWarpingTab::implOnDraw()
{
    m_Impl->onDraw();
}
