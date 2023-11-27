#pragma once

#include <OpenSimCreator/Documents/MeshWarp/TPSDocumentInputIdentifier.hpp>
#include <OpenSimCreator/Documents/MeshWarp/TPSDocumentLandmarkPair.hpp>
#include <OpenSimCreator/Documents/MeshWarp/UndoableTPSDocumentActions.hpp>
#include <OpenSimCreator/UI/Tabs/MeshWarper/MeshWarpingTabDecorationGenerators.hpp>
#include <OpenSimCreator/UI/Tabs/MeshWarper/MeshWarpingTabHover.hpp>
#include <OpenSimCreator/UI/Tabs/MeshWarper/MeshWarpingTabPanel.hpp>
#include <OpenSimCreator/UI/Tabs/MeshWarper/MeshWarpingTabSharedState.hpp>

#include <IconsFontAwesome5.h>
#include <imgui.h>
#include <OpenSimCreator/UI/Tabs/MeshWarper/MeshWarpingTabPanel.hpp>
#include <OpenSimCreator/UI/Tabs/MeshWarper/MeshWarpingTabSharedState.hpp>
#include <oscar/Bindings/ImGuiHelpers.hpp>
#include <oscar/Graphics/Color.hpp>
#include <oscar/Graphics/Mesh.hpp>
#include <oscar/Graphics/RenderTexture.hpp>
#include <oscar/Graphics/ShaderCache.hpp>
#include <oscar/Maths/BVH.hpp>
#include <oscar/Maths/CollisionTests.hpp>
#include <oscar/Maths/Line.hpp>
#include <oscar/Maths/MathHelpers.hpp>
#include <oscar/Maths/PolarPerspectiveCamera.hpp>
#include <oscar/Maths/RayCollision.hpp>
#include <oscar/Maths/Rect.hpp>
#include <oscar/Maths/Transform.hpp>
#include <oscar/Maths/Vec2.hpp>
#include <oscar/Maths/Vec3.hpp>
#include <oscar/Maths/Vec4.hpp>
#include <oscar/Platform/App.hpp>
#include <oscar/Scene/CachedSceneRenderer.hpp>
#include <oscar/Scene/SceneCache.hpp>
#include <oscar/Scene/SceneDecoration.hpp>
#include <oscar/Scene/SceneHelpers.hpp>
#include <oscar/Scene/SceneRendererParams.hpp>
#include <oscar/Utils/Assertions.hpp>
#include <oscar/Utils/CStringView.hpp>

#include <functional>
#include <memory>
#include <optional>
#include <string_view>
#include <utility>
#include <vector>

namespace osc
{
    // an "input" panel (i.e. source or destination mesh, before warping)
    class MeshWarpingTabInputMeshPanel final : public MeshWarpingTabPanel {
    public:
        MeshWarpingTabInputMeshPanel(
            std::string_view panelName_,
            std::shared_ptr<MeshWarpingTabSharedState> state_,
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
            Rect const contentRect = osc::ContentRegionAvailScreenRect();
            Vec2 const contentRectDims = osc::Dimensions(contentRect);
            Vec2 const mousePos = ImGui::GetMousePos();
            Line const cameraRay = m_Camera.unprojectTopLeftPosToWorldRay(mousePos - contentRect.p1, osc::Dimensions(contentRect));

            // mesh hittest: compute whether the user is hovering over the mesh (affects rendering)
            Mesh const& inputMesh = m_State->getScratchMesh(m_DocumentIdentifier);
            BVH const& inputMeshBVH = m_State->getScratchMeshBVH(m_DocumentIdentifier);
            std::optional<RayCollision> const meshCollision = m_LastTextureHittestResult.isHovered ?
                osc::GetClosestWorldspaceRayCollision(inputMesh, inputMeshBVH, Transform{}, cameraRay) :
                std::nullopt;

            // landmark hittest: compute whether the user is hovering over a landmark
            std::optional<MeshWarpingTabHover> landmarkCollision = m_LastTextureHittestResult.isHovered ?
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
            RenderTexture& renderTexture = renderScene(contentRectDims, meshCollision, landmarkCollision);
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
        std::optional<MeshWarpingTabHover> getMouseLandmarkCollisions(Line const& cameraRay) const
        {
            std::optional<MeshWarpingTabHover> rv;
            for (TPSDocumentLandmarkPair const& p : m_State->getScratch().landmarkPairs)
            {
                std::optional<Vec3> const maybePos = GetLocation(p, m_DocumentIdentifier);

                if (!maybePos)
                {
                    // doesn't have a source/destination landmark
                    continue;
                }
                // else: hittest the landmark as a sphere

                std::optional<RayCollision> const coll = osc::GetRayCollisionSphere(cameraRay, Sphere{*maybePos, m_LandmarkRadius});
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
            ImGuiItemHittestResult const& htResult,
            std::optional<RayCollision> const& meshCollision,
            std::optional<MeshWarpingTabHover> const& landmarkCollision)
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
        void drawOverlays(Rect const& renderRect)
        {
            ImGui::SetCursorScreenPos(renderRect.p1 + m_State->overlayPadding);

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
                ImGui::Text("%zu", CountNumLandmarksForInput(m_State->getScratch(), m_DocumentIdentifier));

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("# verts");
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("%zu", m_State->getScratchMesh(m_DocumentIdentifier).getVerts().size());

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("# triangles");
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("%zu", m_State->getScratchMesh(m_DocumentIdentifier).getIndices().size()/3);

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
                    ActionTrySaveMeshToObj(m_State->getScratchMesh(m_DocumentIdentifier));
                }
                if (ImGui::MenuItem("Mesh to STL"))
                {
                    ActionTrySaveMeshToStl(m_State->getScratchMesh(m_DocumentIdentifier));
                }
                if (ImGui::MenuItem("Landmarks to CSV"))
                {
                    ActionSaveLandmarksToCSV(m_State->getScratch(), m_DocumentIdentifier);
                }
                ImGui::EndPopup();
            }
        }

        // draws a button that auto-fits the camera to the 3D scene
        void drawAutoFitCameraButton()
        {
            if (ImGui::Button(ICON_FA_EXPAND_ARROWS_ALT))
            {
                osc::AutoFocus(m_Camera, m_State->getScratchMesh(m_DocumentIdentifier).getBounds(), osc::AspectRatio(m_LastTextureHittestResult.rect));
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

            CStringView const label = "landmark radius";
            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - ImGui::CalcTextSize(label.c_str()).x - ImGui::GetStyle().ItemInnerSpacing.x - m_State->overlayPadding.x);
            ImGui::SliderFloat(label.c_str(), &m_LandmarkRadius, 0.0001f, 100.0f, "%.4f", flags);
        }

        // renders this panel's 3D scene to a texture
        RenderTexture& renderScene(
            Vec2 dims,
            std::optional<RayCollision> const& maybeMeshCollision,
            std::optional<MeshWarpingTabHover> const& maybeLandmarkCollision)
        {
            SceneRendererParams const params = CalcStandardDarkSceneRenderParams(
                m_Camera,
                App::get().getCurrentAntiAliasingLevel(),
                dims
            );
            std::vector<SceneDecoration> const decorations = generateDecorations(maybeMeshCollision, maybeLandmarkCollision);
            return m_CachedRenderer.render(decorations, params);
        }

        // returns a fresh list of 3D decorations for this panel's 3D render
        std::vector<SceneDecoration> generateDecorations(
            std::optional<RayCollision> const& maybeMeshCollision,
            std::optional<MeshWarpingTabHover> const& maybeLandmarkCollision) const
        {
            // generate in-scene 3D decorations
            std::vector<SceneDecoration> decorations;
            decorations.reserve(6 + CountNumLandmarksForInput(m_State->getScratch(), m_DocumentIdentifier));  // likely guess

            std::function<void(SceneDecoration&&)> const decorationConsumer =
                [&decorations](SceneDecoration&& dec) { decorations.push_back(std::move(dec)); };

            AppendCommonDecorations(
                *m_State,
                m_State->getScratchMesh(m_DocumentIdentifier),
                m_WireframeMode,
                decorationConsumer
            );

            // append each landmark as a sphere
            for (TPSDocumentLandmarkPair const& p : m_State->getScratch().landmarkPairs)
            {
                std::optional<Vec3> const maybeLocation = GetLocation(p, m_DocumentIdentifier);

                if (!maybeLocation)
                {
                    continue;  // no source/destination location for the landmark
                }

                TPSDocumentElementID fullID{m_DocumentIdentifier, TPSDocumentInputElementType::Landmark, p.id};

                Transform transform{};
                transform.scale *= m_LandmarkRadius;
                transform.position = *maybeLocation;

                Color const& color = IsFullyPaired(p) ? m_State->pairedLandmarkColor : m_State->unpairedLandmarkColor;

                SceneDecoration& decoration = decorations.emplace_back(m_State->landmarkSphere, transform, color);

                if (m_State->userSelection.contains(fullID))
                {
                    Vec4 tmpColor = decoration.color;
                    tmpColor += Vec4{0.25f, 0.25f, 0.25f, 0.0f};
                    tmpColor = osc::Clamp(tmpColor, 0.0f, 1.0f);

                    decoration.color = Color{tmpColor};
                    decoration.flags = SceneDecorationFlags::IsSelected;
                }
                else if (m_State->currentHover && m_State->currentHover->maybeSceneElementID == fullID)
                {
                    Vec4 tmpColor = decoration.color;
                    tmpColor += Vec4{0.15f, 0.15f, 0.15f, 0.0f};
                    tmpColor = osc::Clamp(tmpColor, 0.0f, 1.0f);

                    decoration.color = Color{tmpColor};
                    decoration.flags = SceneDecorationFlags::IsHovered;
                }
            }

            // append non-participating landmarks as non-user-selctable purple spheres
            if (m_DocumentIdentifier == TPSDocumentInputIdentifier::Source)
            {
                for (auto const& nonParticipatingLandmark : m_State->getScratch().nonParticipatingLandmarks)
                {
                    AppendNonParticipatingLandmark(
                        m_State->landmarkSphere,
                        m_LandmarkRadius,
                        nonParticipatingLandmark.location,
                        m_State->nonParticipatingLandmarkColor,
                        decorationConsumer
                    );
                }
            }

            // if applicable, show mouse-to-mesh collision as faded landmark as a placement hint for user
            if (maybeMeshCollision && !maybeLandmarkCollision)
            {
                Transform transform{};
                transform.scale *= m_LandmarkRadius;
                transform.position = maybeMeshCollision->position;

                Color color = m_State->unpairedLandmarkColor;
                color.a *= 0.25f;

                decorations.emplace_back(m_State->landmarkSphere, transform, color);
            }

            return decorations;
        }

        std::shared_ptr<MeshWarpingTabSharedState> m_State;
        TPSDocumentInputIdentifier m_DocumentIdentifier;
        PolarPerspectiveCamera m_Camera = CreateCameraFocusedOn(m_State->getScratchMesh(m_DocumentIdentifier).getBounds());
        CachedSceneRenderer m_CachedRenderer
        {
            App::config(),
            *App::singleton<SceneCache>(),
            *App::singleton<ShaderCache>(),
        };
        ImGuiItemHittestResult m_LastTextureHittestResult;
        bool m_WireframeMode = true;
        float m_LandmarkRadius = 0.05f;
    };
}
