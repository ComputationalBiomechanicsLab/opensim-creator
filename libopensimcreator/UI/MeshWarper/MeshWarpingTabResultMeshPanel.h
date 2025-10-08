#pragma once

#include <libopensimcreator/Documents/Landmarks/LandmarkCSVFlags.h>
#include <libopensimcreator/Documents/MeshWarper/UndoableTPSDocumentActions.h>
#include <libopensimcreator/Platform/IconCodepoints.h>
#include <libopensimcreator/UI/MeshWarper/MeshWarpingTabDecorationGenerators.h>
#include <libopensimcreator/UI/MeshWarper/MeshWarpingTabPanel.h>
#include <libopensimcreator/UI/MeshWarper/MeshWarpingTabSharedState.h>

#include <liboscar/Graphics/RenderTexture.h>
#include <liboscar/Graphics/Scene/CachedSceneRenderer.h>
#include <liboscar/Graphics/Scene/SceneCache.h>
#include <liboscar/Graphics/Scene/SceneDecoration.h>
#include <liboscar/Graphics/Scene/SceneRendererParams.h>
#include <liboscar/Maths/PolarPerspectiveCamera.h>
#include <liboscar/Maths/Vector2.h>
#include <liboscar/Maths/Vector3.h>
#include <liboscar/Platform/App.h>
#include <liboscar/UI/oscimgui.h>
#include <liboscar/Utils/CStringView.h>

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
        explicit MeshWarpingTabResultMeshPanel(
            Widget* parent,
            std::string_view panelName_,
            std::shared_ptr<MeshWarpingTabSharedState> state_) :

            MeshWarpingTabPanel{parent, panelName_},
            m_State{std::move(state_)}
        {}
    private:
        void impl_draw_content() final
        {
            // fill the entire available region with the render
            const Vector2 dims = ui::get_content_region_available();

            updateCamera();

            // render it via ImGui and hittest it
            RenderTexture& renderTexture = renderScene(dims);
            ui::draw_image(renderTexture);
            m_LastTextureHittestResult = ui::hittest_last_drawn_item();

            drawOverlays(m_LastTextureHittestResult.item_ui_rect);
        }

        void updateCamera()
        {
            // if cameras are linked together, ensure all cameras match the "base" camera
            m_State->updateOneCameraFromLinkedBase(m_Camera);

            // update camera if user drags it around etc.
            if (m_LastTextureHittestResult.is_hovered)
            {
                if (ui::update_polar_camera_from_mouse_inputs(m_Camera, m_LastTextureHittestResult.item_ui_rect.dimensions()))
                {
                    m_State->setLinkedBaseCamera(m_Camera);  // reflects latest modification
                }
            }
        }

        // draw ImGui overlays over a result panel
        void drawOverlays(const Rect& renderRect)
        {
            // ImGui: set cursor to draw over the top-right of the render texture (with padding)
            ui::set_cursor_ui_position(renderRect.ypd_top_left() + m_OverlayPadding);

            drawInformationIcon();
            ui::same_line();
            drawExportButton();
            ui::same_line();
            drawAutoFitCameraButton();
            ui::same_line();
            drawLandmarkRadiusSlider();
            drawBlendingFactorSlider();
            drawPrescaleInputs();

            ui::set_cursor_panel_x(m_CursorXAtExportButton);  // align with "export" button in row above
            ui::draw_checkbox("overlay destination mesh", &m_ShowDestinationMesh);
            ui::same_line();
            {
                bool recalculatingNormals = m_State->getScratch().recalculateNormals;
                if (ui::draw_checkbox("recalculate mesh's normals", &recalculatingNormals)) {
                    ActionSetRecalculatingNormals(m_State->updUndoable(), recalculatingNormals);
                }
            }
        }

        // draws a information icon that shows basic mesh info when hovered
        void drawInformationIcon()
        {
            ui::draw_button_nobg(OSC_ICON_INFO_CIRCLE);
            if (ui::is_item_hovered())
            {
                ui::begin_tooltip();

                ui::draw_text_disabled("Result Information:");
                drawInformationTable();

                ui::end_tooltip();
            }
        }

        // draws a table containing useful input information (handy for debugging)
        void drawInformationTable()
        {
            if (ui::begin_table("##inputinfo", 2))
            {
                ui::table_setup_column("Name");
                ui::table_setup_column("Value");

                ui::table_next_row();
                ui::table_set_column_index(0);
                ui::draw_text("# vertices");
                ui::table_set_column_index(1);
                ui::draw_text("%zu", m_State->getResultMesh().num_vertices());

                ui::table_next_row();
                ui::table_set_column_index(0);
                ui::draw_text("# triangles");
                ui::table_set_column_index(1);
                ui::draw_text("%zu", m_State->getResultMesh().num_indices()/3);

                ui::end_table();
            }
        }

        // draws an export button that enables the user to export things from this input
        void drawExportButton()
        {
            m_CursorXAtExportButton = ui::get_cursor_panel_x();  // needed to align the blending factor slider
            ui::draw_button(OSC_ICON_FILE_EXPORT " export" OSC_ICON_CARET_DOWN);
            if (ui::begin_popup_context_menu("##exportcontextmenu", ui::PopupFlag::MouseButtonLeft))
            {
                if (ui::draw_menu_item("Mesh to OBJ"))
                {
                    ActionPromptUserToSaveMeshToObjFile(m_State->getResultMesh(), OBJWriterFlag::Default);
                }
                if (ui::draw_menu_item("Mesh to OBJ (no normals)"))
                {
                    ActionPromptUserToSaveMeshToObjFile(m_State->getResultMesh(), OBJWriterFlag::NoWriteNormals);
                }
                if (ui::draw_menu_item("Mesh to STL"))
                {
                    ActionPromptUserToMeshToStlFile(m_State->getResultMesh());
                }
                if (ui::draw_menu_item("Warped Non-Participating Landmarks to CSV"))
                {
                    ActionPromptUserToSaveWarpedNonParticipatingLandmarksToCSV(m_State->getScratch(), m_State->updResultCache());
                }
                if (ui::draw_menu_item("Warped Non-Participating Landmark Positions to CSV"))
                {
                    ActionPromptUserToSaveWarpedNonParticipatingLandmarksToCSV(m_State->getScratch(), m_State->updResultCache(), LandmarkCSVFlags::NoHeader | LandmarkCSVFlags::NoNames);
                }
                if (ui::draw_menu_item("Landmark Pairs to CSV"))
                {
                    ActionPromptUserToSavePairedLandmarksToCSV(m_State->getScratch());
                }
                if (ui::draw_menu_item("Landmark Pairs to CSV (no names)"))
                {
                    ActionPromptUserToSavePairedLandmarksToCSV(m_State->getScratch(), LandmarkCSVFlags::NoNames);
                }
                ui::end_popup();
            }
        }

        // draws a button that auto-fits the camera to the 3D scene
        void drawAutoFitCameraButton()
        {
            if (ui::draw_button(OSC_ICON_EXPAND_ARROWS_ALT))
            {
                auto_focus(
                    m_Camera,
                    m_State->getResultMesh().bounds(),
                    aspect_ratio_of(m_LastTextureHittestResult.item_ui_rect)
                );
                m_State->setLinkedBaseCamera(m_Camera);
            }
            ui::draw_tooltip_if_item_hovered(
                "Autoscale Scene",
                "Zooms camera to try and fit everything in the scene into the viewer"
            );
        }

        // draws a slider that lets the user edit how large the landmarks are
        void drawLandmarkRadiusSlider()
        {
            // note: log scale is important: some users have meshes that
            // are in different scales (e.g. millimeters)
            const ui::SliderFlags flags = ui::SliderFlag::Logarithmic;

            const CStringView label = "landmark radius";
            ui::set_next_item_width(ui::get_content_region_available().x - ui::calc_text_size(label).x - ui::get_style_item_inner_spacing().x - m_State->getOverlayPadding().x);
            ui::draw_float_slider(label, &m_LandmarkRadius, 0.0001f, 100.0f, "%.4f", flags);
        }

        void drawBlendingFactorSlider()
        {
            ui::set_cursor_panel_x(m_CursorXAtExportButton);  // align with "export" button in row above

            const CStringView label = "blending factor  ";  // deliberate trailing spaces (for alignment with "landmark radius")
            ui::set_next_item_width(ui::get_content_region_available().x - ui::calc_text_size(label).x - ui::get_style_item_inner_spacing().x - m_OverlayPadding.x);

            float factor = m_State->getScratch().blendingFactor;
            if (ui::draw_float_slider(label, &factor, 0.0f, 1.0f))
            {
                ActionSetBlendFactorWithoutCommitting(m_State->updUndoable(), factor);
            }
            if (ui::is_item_deactivated_after_edit())
            {
                ActionSetBlendFactor(m_State->updUndoable(), factor);
            }
        }

        void drawPrescaleInputs()
        {
            float sourcePrescale = m_State->getScratch().sourceLandmarksPrescale;
            ui::set_cursor_panel_x(m_CursorXAtExportButton);
            ui::draw_float_input("source landmarks prescale", &sourcePrescale);
            if (ui::is_item_deactivated_after_edit()) {
                ActionSetSourceLandmarksPrescale(m_State->updUndoable(), sourcePrescale);
            }
            float destinationPrescale = m_State->getScratch().destinationLandmarksPrescale;
            ui::set_cursor_panel_x(m_CursorXAtExportButton);
            ui::draw_float_input("destination prescale", &destinationPrescale);
            if (ui::is_item_deactivated_after_edit()) {
                ActionSetDestinationLandmarksPrescale(m_State->updUndoable(), destinationPrescale);
            }

            ui::set_cursor_panel_x(m_CursorXAtExportButton);
            bool affineScale = m_State->getScratch().applyAffineScale;
            ui::draw_checkbox("scale", &affineScale);
            if (ui::is_item_deactivated_after_edit()) {
                m_State->updUndoable().upd_scratch().applyAffineScale = affineScale;  // TODO: undo/redo
            }
            ui::same_line();
            bool affineRotation = m_State->getScratch().applyAffineRotation;
            ui::draw_checkbox("rotate", &affineRotation);
            if (ui::is_item_deactivated_after_edit()) {
                m_State->updUndoable().upd_scratch().applyAffineRotation = affineRotation;  // TODO: undo/redo
            }
            ui::same_line();
            bool affineTranslation = m_State->getScratch().applyAffineTranslation;
            ui::draw_checkbox("translate", &affineTranslation);
            if (ui::is_item_deactivated_after_edit()) {
                m_State->updUndoable().upd_scratch().applyAffineTranslation = affineTranslation;  // TODO: undo/redo
            }
            ui::same_line();
            bool nonAffineWarp = m_State->getScratch().applyNonAffineWarp;
            ui::draw_checkbox("warp", &nonAffineWarp);
            if (ui::is_item_deactivated_after_edit()) {
                m_State->updUndoable().upd_scratch().applyNonAffineWarp = nonAffineWarp;  // TODO: undo/redo
            }
        }

        // returns 3D decorations for the given result panel
        std::vector<SceneDecoration> generateDecorations() const
        {
            std::vector<SceneDecoration> decorations;
            const std::function<void(SceneDecoration&&)> decorationConsumer =
                [&decorations](SceneDecoration&& dec) { decorations.push_back(std::move(dec)); };

            AppendCommonDecorations(
                *m_State,
                m_State->getResultMesh(),
                m_State->isWireframeModeEnabled(),
                decorationConsumer
            );

            if (m_ShowDestinationMesh)
            {
                decorationConsumer(SceneDecoration{
                    .mesh = m_State->getScratch().destinationMesh,
                    .shading = Color::red().with_alpha(0.5f),
                });
            }

            // draw non-participating landmarks
            for (const Vector3& nonParticipatingLandmarkPos : m_State->getResultNonParticipatingLandmarkLocations())
            {
                decorationConsumer(SceneDecoration{
                    .mesh = m_State->getLandmarkSphereMesh(),
                    .transform = {
                        .scale = Vector3{GetNonParticipatingLandmarkScaleFactor()*m_LandmarkRadius},
                        .translation = nonParticipatingLandmarkPos,
                    },
                    .shading = m_State->getNonParticipatingLandmarkColor(),
                });
            }

            return decorations;
        }

        // renders a panel to a texture via its renderer and returns a reference to the rendered texture
        RenderTexture& renderScene(Vector2 dims)
        {
            const std::vector<SceneDecoration> decorations = generateDecorations();
            SceneRendererParams params = calc_standard_dark_scene_render_params(
                m_Camera,
                App::get().anti_aliasing_level(),
                dims,
                App::settings().get_value<float>("graphics/render_scale", 1.0f) * App::get().main_window_device_pixel_ratio()
            );
            m_State->getCustomRenderingOptions().applyTo(params);
            return m_CachedRenderer.render(decorations, params);
        }

        std::shared_ptr<MeshWarpingTabSharedState> m_State;
        PolarPerspectiveCamera m_Camera = create_camera_focused_on(m_State->getResultMesh().bounds());
        CachedSceneRenderer m_CachedRenderer{
            *App::singleton<SceneCache>(App::resource_loader()),
        };
        ui::HittestResult m_LastTextureHittestResult;
        bool m_ShowDestinationMesh = false;
        Vector2 m_OverlayPadding = {10.0f, 10.0f};
        float m_LandmarkRadius = 0.05f;
        float m_CursorXAtExportButton = 0.0f;
    };
}
