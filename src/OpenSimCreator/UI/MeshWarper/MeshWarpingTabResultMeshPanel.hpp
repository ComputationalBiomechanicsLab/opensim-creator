#pragma once

#include <OpenSimCreator/Documents/Landmarks/LandmarkCSVFlags.hpp>
#include <OpenSimCreator/Documents/MeshWarper/UndoableTPSDocumentActions.hpp>
#include <OpenSimCreator/UI/MeshWarper/MeshWarpingTabPanel.hpp>
#include <OpenSimCreator/UI/MeshWarper/MeshWarpingTabDecorationGenerators.hpp>
#include <OpenSimCreator/UI/MeshWarper/MeshWarpingTabSharedState.hpp>

#include <IconsFontAwesome5.h>
#include <imgui.h>
#include <oscar/Bindings/ImGuiHelpers.hpp>
#include <oscar/Graphics/RenderTexture.hpp>
#include <oscar/Graphics/ShaderCache.hpp>
#include <oscar/Maths/MathHelpers.hpp>
#include <oscar/Maths/PolarPerspectiveCamera.hpp>
#include <oscar/Maths/Vec2.hpp>
#include <oscar/Maths/Vec3.hpp>
#include <oscar/Platform/App.hpp>
#include <oscar/Scene/CachedSceneRenderer.hpp>
#include <oscar/Scene/SceneCache.hpp>
#include <oscar/Scene/SceneDecoration.hpp>
#include <oscar/Scene/SceneRendererParams.hpp>
#include <oscar/Utils/CStringView.hpp>

#include <functional>
#include <memory>
#include <string_view>
#include <utility>
#include <vector>

using osc::lm::LandmarkCSVFlags;

namespace osc
{
    // a "result" panel (i.e. after applying a warp to the source)
    class MeshWarpingTabResultMeshPanel final : public MeshWarpingTabPanel {
    public:
        MeshWarpingTabResultMeshPanel(
            std::string_view panelName_,
            std::shared_ptr<MeshWarpingTabSharedState> state_) :

            MeshWarpingTabPanel{panelName_},
            m_State{std::move(state_)}
        {
        }
    private:
        void implDrawContent() final
        {
            // fill the entire available region with the render
            Vec2 const dims = ImGui::GetContentRegionAvail();

            updateCamera();

            // render it via ImGui and hittest it
            RenderTexture& renderTexture = renderScene(dims);
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
                if (UpdatePolarCameraFromImGuiMouseInputs(m_Camera, Dimensions(m_LastTextureHittestResult.rect)))
                {
                    m_State->linkedCameraBase = m_Camera;  // reflects latest modification
                }
            }
        }

        // draw ImGui overlays over a result panel
        void drawOverlays(Rect const& renderRect)
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
                ImGui::Text("%zu", m_State->getResultMesh().getVerts().size());

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("# triangles");
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("%zu", m_State->getResultMesh().getIndices().size()/3);

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
                    ActionTrySaveMeshToObjFile(m_State->getResultMesh());
                }
                if (ImGui::MenuItem("Mesh to STL"))
                {
                    ActionTrySaveMeshToStlFile(m_State->getResultMesh());
                }
                if (ImGui::MenuItem("Warped Non-Participating Landmarks to CSV"))
                {
                    ActionSaveWarpedNonParticipatingLandmarksToCSV(m_State->getScratch(), m_State->meshResultCache);
                }
                if (ImGui::MenuItem("Warped Non-Participating Landmark Positions to CSV"))
                {
                    ActionSaveWarpedNonParticipatingLandmarksToCSV(m_State->getScratch(), m_State->meshResultCache, LandmarkCSVFlags::NoHeader | LandmarkCSVFlags::NoNames);
                }
                if (ImGui::MenuItem("Landmark Pairs to CSV"))
                {
                    ActionSavePairedLandmarksToCSV(m_State->getScratch());
                }
                if (ImGui::MenuItem("Landmark Pairs to CSV (no names)"))
                {
                    ActionSavePairedLandmarksToCSV(m_State->getScratch(), LandmarkCSVFlags::NoNames);
                }
                ImGui::EndPopup();
            }
        }

        // draws a button that auto-fits the camera to the 3D scene
        void drawAutoFitCameraButton()
        {
            if (ImGui::Button(ICON_FA_EXPAND_ARROWS_ALT))
            {
                AutoFocus(
                    m_Camera,
                    m_State->getResultMesh().getBounds(),
                    AspectRatio(m_LastTextureHittestResult.rect)
                );
                m_State->linkedCameraBase = m_Camera;
            }
            DrawTooltipIfItemHovered(
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

            CStringView const label = "landmark radius";
            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - ImGui::CalcTextSize(label.c_str()).x - ImGui::GetStyle().ItemInnerSpacing.x - m_State->overlayPadding.x);
            ImGui::SliderFloat(label.c_str(), &m_LandmarkRadius, 0.0001f, 100.0f, "%.4f", flags);
        }

        void drawBlendingFactorSlider()
        {
            ImGui::SetCursorPosX(m_CursorXAtExportButton);  // align with "export" button in row above

            CStringView const label = "blending factor  ";  // deliberate trailing spaces (for alignment with "landmark radius")
            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - ImGui::CalcTextSize(label.c_str()).x - ImGui::GetStyle().ItemInnerSpacing.x - m_OverlayPadding.x);

            float factor = m_State->getScratch().blendingFactor;
            if (ImGui::SliderFloat(label.c_str(), &factor, 0.0f, 1.0f))
            {
                ActionSetBlendFactorWithoutCommitting(m_State->updUndoable(), factor);
            }
            if (ImGui::IsItemDeactivatedAfterEdit())
            {
                ActionSetBlendFactor(m_State->updUndoable(), factor);
            }
        }

        // returns 3D decorations for the given result panel
        std::vector<SceneDecoration> generateDecorations() const
        {
            std::vector<SceneDecoration> decorations;
            std::function<void(SceneDecoration&&)> const decorationConsumer =
                [&decorations](SceneDecoration&& dec) { decorations.push_back(std::move(dec)); };

            AppendCommonDecorations(
                *m_State,
                m_State->getResultMesh(),
                m_WireframeMode,
                decorationConsumer
            );

            if (m_ShowDestinationMesh)
            {
                decorationConsumer({
                    .mesh = m_State->getScratch().destinationMesh,
                    .color = Color::red().withAlpha(0.5f),
                });
            }

            // draw non-participating landmarks
            for (Vec3 const& nonParticipatingLandmarkPos : m_State->getResultNonParticipatingLandmarkLocations())
            {
                decorationConsumer({
                    .mesh = m_State->landmarkSphere,
                    .transform = {
                        .scale = Vec3{GetNonParticipatingLandmarkScaleFactor()*m_LandmarkRadius},
                        .position = nonParticipatingLandmarkPos,
                    },
                    .color = m_State->nonParticipatingLandmarkColor,
                });
            }

            return decorations;
        }

        // renders a panel to a texture via its renderer and returns a reference to the rendered texture
        RenderTexture& renderScene(Vec2 dims)
        {
            std::vector<SceneDecoration> const decorations = generateDecorations();
            SceneRendererParams const params = CalcStandardDarkSceneRenderParams(
                m_Camera,
                App::get().getCurrentAntiAliasingLevel(),
                dims
            );
            return m_CachedRenderer.render(decorations, params);
        }

        std::shared_ptr<MeshWarpingTabSharedState> m_State;
        PolarPerspectiveCamera m_Camera = CreateCameraFocusedOn(m_State->getResultMesh().getBounds());
        CachedSceneRenderer m_CachedRenderer
        {
            App::config(),
            *App::singleton<SceneCache>(),
            *App::singleton<ShaderCache>(),
        };
        ImGuiItemHittestResult m_LastTextureHittestResult;
        bool m_WireframeMode = true;
        bool m_ShowDestinationMesh = false;
        Vec2 m_OverlayPadding = {10.0f, 10.0f};
        float m_LandmarkRadius = 0.05f;
        float m_CursorXAtExportButton = 0.0f;
    };
}
