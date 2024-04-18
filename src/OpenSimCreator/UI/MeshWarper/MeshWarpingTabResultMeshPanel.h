#pragma once

#include <OpenSimCreator/Documents/Landmarks/LandmarkCSVFlags.h>
#include <OpenSimCreator/Documents/MeshWarper/UndoableTPSDocumentActions.h>
#include <OpenSimCreator/UI/MeshWarper/MeshWarpingTabDecorationGenerators.h>
#include <OpenSimCreator/UI/MeshWarper/MeshWarpingTabPanel.h>
#include <OpenSimCreator/UI/MeshWarper/MeshWarpingTabSharedState.h>

#include <IconsFontAwesome5.h>
#include <oscar/Graphics/RenderTexture.h>
#include <oscar/Graphics/Scene/CachedSceneRenderer.h>
#include <oscar/Graphics/Scene/SceneCache.h>
#include <oscar/Graphics/Scene/SceneDecoration.h>
#include <oscar/Graphics/Scene/SceneRendererParams.h>
#include <oscar/Maths/MathHelpers.h>
#include <oscar/Maths/PolarPerspectiveCamera.h>
#include <oscar/Maths/Vec2.h>
#include <oscar/Maths/Vec3.h>
#include <oscar/Platform/App.h>
#include <oscar/UI/ImGuiHelpers.h>
#include <oscar/UI/oscimgui.h>
#include <oscar/Utils/CStringView.h>

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
            Vec2 const dims = ui::GetContentRegionAvail();

            updateCamera();

            // render it via ImGui and hittest it
            RenderTexture& renderTexture = renderScene(dims);
            ui::Image(renderTexture);
            m_LastTextureHittestResult = ui::HittestLastItem();

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
                if (ui::UpdatePolarCameraFromMouseInputs(m_Camera, dimensions_of(m_LastTextureHittestResult.rect)))
                {
                    m_State->linkedCameraBase = m_Camera;  // reflects latest modification
                }
            }
        }

        // draw ImGui overlays over a result panel
        void drawOverlays(Rect const& renderRect)
        {
            // ImGui: set cursor to draw over the top-right of the render texture (with padding)
            ui::SetCursorScreenPos(renderRect.p1 + m_OverlayPadding);

            drawInformationIcon();
            ui::SameLine();
            drawExportButton();
            ui::SameLine();
            drawAutoFitCameraButton();
            ui::SameLine();
            ui::Checkbox("show destination", &m_ShowDestinationMesh);
            ui::SameLine();
            drawLandmarkRadiusSlider();
            drawBlendingFactorSlider();
        }

        // draws a information icon that shows basic mesh info when hovered
        void drawInformationIcon()
        {
            ui::ButtonNoBg(ICON_FA_INFO_CIRCLE);
            if (ui::IsItemHovered())
            {
                ui::BeginTooltip();

                ui::TextDisabled("Result Information:");
                drawInformationTable();

                ui::EndTooltip();
            }
        }

        // draws a table containing useful input information (handy for debugging)
        void drawInformationTable()
        {
            if (ui::BeginTable("##inputinfo", 2))
            {
                ui::TableSetupColumn("Name");
                ui::TableSetupColumn("Value");

                ui::TableNextRow();
                ui::TableSetColumnIndex(0);
                ui::Text("# vertices");
                ui::TableSetColumnIndex(1);
                ui::Text("%zu", m_State->getResultMesh().num_vertices());

                ui::TableNextRow();
                ui::TableSetColumnIndex(0);
                ui::Text("# triangles");
                ui::TableSetColumnIndex(1);
                ui::Text("%zu", m_State->getResultMesh().num_indices()/3);

                ui::EndTable();
            }
        }

        // draws an export button that enables the user to export things from this input
        void drawExportButton()
        {
            m_CursorXAtExportButton = ui::GetCursorPos().x;  // needed to align the blending factor slider
            ui::Button(ICON_FA_FILE_EXPORT " export" ICON_FA_CARET_DOWN);
            if (ui::BeginPopupContextItem("##exportcontextmenu", ImGuiPopupFlags_MouseButtonLeft))
            {
                if (ui::MenuItem("Mesh to OBJ"))
                {
                    ActionTrySaveMeshToObjFile(m_State->getResultMesh(), ObjWriterFlags::Default);
                }
                if (ui::MenuItem("Mesh to OBJ (no normals)"))
                {
                    ActionTrySaveMeshToObjFile(m_State->getResultMesh(), ObjWriterFlags::NoWriteNormals);
                }
                if (ui::MenuItem("Mesh to STL"))
                {
                    ActionTrySaveMeshToStlFile(m_State->getResultMesh());
                }
                if (ui::MenuItem("Warped Non-Participating Landmarks to CSV"))
                {
                    ActionSaveWarpedNonParticipatingLandmarksToCSV(m_State->getScratch(), m_State->meshResultCache);
                }
                if (ui::MenuItem("Warped Non-Participating Landmark Positions to CSV"))
                {
                    ActionSaveWarpedNonParticipatingLandmarksToCSV(m_State->getScratch(), m_State->meshResultCache, LandmarkCSVFlags::NoHeader | LandmarkCSVFlags::NoNames);
                }
                if (ui::MenuItem("Landmark Pairs to CSV"))
                {
                    ActionSavePairedLandmarksToCSV(m_State->getScratch());
                }
                if (ui::MenuItem("Landmark Pairs to CSV (no names)"))
                {
                    ActionSavePairedLandmarksToCSV(m_State->getScratch(), LandmarkCSVFlags::NoNames);
                }
                ui::EndPopup();
            }
        }

        // draws a button that auto-fits the camera to the 3D scene
        void drawAutoFitCameraButton()
        {
            if (ui::Button(ICON_FA_EXPAND_ARROWS_ALT))
            {
                AutoFocus(
                    m_Camera,
                    m_State->getResultMesh().bounds(),
                    aspect_ratio(m_LastTextureHittestResult.rect)
                );
                m_State->linkedCameraBase = m_Camera;
            }
            ui::DrawTooltipIfItemHovered(
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
            ui::SetNextItemWidth(ui::GetContentRegionAvail().x - ui::CalcTextSize(label).x - ui::GetStyleItemInnerSpacing().x - m_State->overlayPadding.x);
            ui::SliderFloat(label, &m_LandmarkRadius, 0.0001f, 100.0f, "%.4f", flags);
        }

        void drawBlendingFactorSlider()
        {
            ui::SetCursorPosX(m_CursorXAtExportButton);  // align with "export" button in row above

            CStringView const label = "blending factor  ";  // deliberate trailing spaces (for alignment with "landmark radius")
            ui::SetNextItemWidth(ui::GetContentRegionAvail().x - ui::CalcTextSize(label).x - ui::GetStyleItemInnerSpacing().x - m_OverlayPadding.x);

            float factor = m_State->getScratch().blendingFactor;
            if (ui::SliderFloat(label, &factor, 0.0f, 1.0f))
            {
                ActionSetBlendFactorWithoutCommitting(m_State->updUndoable(), factor);
            }
            if (ui::IsItemDeactivatedAfterEdit())
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
                    .color = Color::red().with_alpha(0.5f),
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
            SceneRendererParams const params = calc_standard_dark_scene_render_params(
                m_Camera,
                App::get().anti_aliasing_level(),
                dims
            );
            return m_CachedRenderer.render(decorations, params);
        }

        std::shared_ptr<MeshWarpingTabSharedState> m_State;
        PolarPerspectiveCamera m_Camera = CreateCameraFocusedOn(m_State->getResultMesh().bounds());
        CachedSceneRenderer m_CachedRenderer{
            *App::singleton<SceneCache>(App::resource_loader()),
        };
        ui::HittestResult m_LastTextureHittestResult;
        bool m_WireframeMode = true;
        bool m_ShowDestinationMesh = false;
        Vec2 m_OverlayPadding = {10.0f, 10.0f};
        float m_LandmarkRadius = 0.05f;
        float m_CursorXAtExportButton = 0.0f;
    };
}
