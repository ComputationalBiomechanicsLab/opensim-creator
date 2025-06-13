#pragma once

#include <libopensimcreator/Documents/Landmarks/LandmarkCSVFlags.h>
#include <libopensimcreator/Documents/MeshWarper/TPSDocumentInputIdentifier.h>
#include <libopensimcreator/Documents/MeshWarper/TPSDocumentLandmarkPair.h>
#include <libopensimcreator/Documents/MeshWarper/UndoableTPSDocumentActions.h>
#include <libopensimcreator/Platform/IconCodepoints.h>
#include <libopensimcreator/UI/MeshWarper/MeshWarpingTabContextMenu.h>
#include <libopensimcreator/UI/MeshWarper/MeshWarpingTabDecorationGenerators.h>
#include <libopensimcreator/UI/MeshWarper/MeshWarpingTabHover.h>
#include <libopensimcreator/UI/MeshWarper/MeshWarpingTabPanel.h>
#include <libopensimcreator/UI/MeshWarper/MeshWarpingTabSharedState.h>

#include <liboscar/Graphics/Color.h>
#include <liboscar/Graphics/Geometries/SolidGeometries.h>
#include <liboscar/Graphics/Mesh.h>
#include <liboscar/Graphics/RenderTexture.h>
#include <liboscar/Graphics/Scene/CachedSceneRenderer.h>
#include <liboscar/Graphics/Scene/SceneCache.h>
#include <liboscar/Graphics/Scene/SceneDecoration.h>
#include <liboscar/Graphics/Scene/SceneHelpers.h>
#include <liboscar/Graphics/Scene/SceneRendererParams.h>
#include <liboscar/Maths/BVH.h>
#include <liboscar/Maths/CollisionTests.h>
#include <liboscar/Maths/Line.h>
#include <liboscar/Maths/MathHelpers.h>
#include <liboscar/Maths/PolarPerspectiveCamera.h>
#include <liboscar/Maths/RayCollision.h>
#include <liboscar/Maths/Rect.h>
#include <liboscar/Maths/RectFunctions.h>
#include <liboscar/Maths/Transform.h>
#include <liboscar/Maths/Vec2.h>
#include <liboscar/Maths/Vec3.h>
#include <liboscar/Maths/Vec4.h>
#include <liboscar/Platform/App.h>
#include <liboscar/Platform/AppSettings.h>
#include <liboscar/UI/oscimgui.h>
#include <liboscar/Utils/CStringView.h>
#include <liboscar/Utils/Typelist.h>

#include <functional>
#include <memory>
#include <optional>
#include <string_view>
#include <utility>
#include <vector>

using osc::lm::LandmarkCSVFlags;

namespace osc
{
    // an "input" panel (i.e. source or destination mesh, before warping)
    class MeshWarpingTabInputMeshPanel final : public MeshWarpingTabPanel {
    public:
        explicit MeshWarpingTabInputMeshPanel(
            Widget* parent,
            std::string_view panelName_,
            std::shared_ptr<MeshWarpingTabSharedState> state_,
            TPSDocumentInputIdentifier documentIdentifier_) :

            MeshWarpingTabPanel{parent, panelName_},
            m_State{std::move(state_)},
            m_DocumentIdentifier{documentIdentifier_}
        {
        }
    private:
        // draws all of the panel's content
        void impl_draw_content() final
        {
            // compute top-level UI variables (render rect, mouse pos, etc.)
            const Rect contentRect = ui::content_region_available_ui_rect();
            const Vec2 contentRectDims = dimensions_of(contentRect);
            const Vec2 mousePos = ui::get_mouse_ui_pos();

            // un-project mouse's (2D) location into the 3D scene as a ray
            const Line cameraRay = m_Camera.unproject_topleft_pos_to_world_ray(mousePos - contentRect.p1, contentRectDims);

            // mesh hittest: compute whether the user is hovering over the mesh (affects rendering)
            const Mesh& inputMesh = m_State->getScratchMesh(m_DocumentIdentifier);
            const BVH& inputMeshBVH = m_State->getScratchMeshBVH(m_DocumentIdentifier);
            const std::optional<RayCollision> meshCollision = m_LastTextureHittestResult.is_hovered ?
                get_closest_world_space_ray_triangle_collision(inputMesh, inputMeshBVH, Transform{}, cameraRay) :
                std::nullopt;

            // landmark hittest: compute whether the user is hovering over a landmark
            std::optional<MeshWarpingTabHover> landmarkCollision = m_LastTextureHittestResult.is_hovered ?
                getMouseLandmarkCollisions(cameraRay) :
                std::nullopt;

            // state update: tell central state if something's being hovered in this panel
            if (landmarkCollision)
            {
                m_State->setHover(landmarkCollision);
            }
            else if (meshCollision)
            {
                m_State->setHover(m_DocumentIdentifier, meshCollision->position);
            }

            // update camera: NOTE: make sure it's updated *before* rendering; otherwise, it'll be one frame late
            updateCamera();

            // render 3D: draw the scene into the content rect and 2D-hittest it
            RenderTexture& renderTexture = renderScene(contentRectDims, meshCollision, landmarkCollision);
            ui::draw_image(renderTexture);
            m_LastTextureHittestResult = ui::hittest_last_drawn_item();

            // handle any events due to hovering over, clicking, etc.
            handleInputAndHoverEvents(m_LastTextureHittestResult, meshCollision, landmarkCollision);

            // render 2D: draw any 2D overlays over the 3D render
            draw2DOverlayUI(m_LastTextureHittestResult.item_ui_rect);
        }

        // update the 3D camera from user inputs/external data
        void updateCamera()
        {
            // if the cameras are linked together, ensure this camera is updated from the linked camera
            m_State->updateOneCameraFromLinkedBase(m_Camera);

            // if the user interacts with the render, update the camera as necessary
            if (m_LastTextureHittestResult.is_hovered and
                ui::update_polar_camera_from_mouse_inputs(m_Camera, dimensions_of(m_LastTextureHittestResult.item_ui_rect))) {

                m_State->setLinkedBaseCamera(m_Camera);
            }
        }

        // returns the closest collision, if any, between the provided camera ray and a landmark
        std::optional<MeshWarpingTabHover> getMouseLandmarkCollisions(const Line& cameraRay) const
        {
            std::optional<MeshWarpingTabHover> rv;
            hittestLandmarks(cameraRay, rv);
            hittestNonParticipatingLandmarks(cameraRay, rv);
            return rv;
        }

        // 3D hittests landmarks and updates `closest` if a closer collision is found
        void hittestLandmarks(
            const Line& cameraRay,
            std::optional<MeshWarpingTabHover>& closest) const
        {
            for (const TPSDocumentLandmarkPair& landmark : m_State->getScratch().landmarkPairs)
            {
                hittestLandmark(cameraRay, closest, landmark);
            }
        }

        // 3D hittests one landmark and updates `closest` if a closer collision is found
        void hittestLandmark(
            const Line& cameraRay,
            std::optional<MeshWarpingTabHover>& closest,
            const TPSDocumentLandmarkPair& landmark) const
        {
            const std::optional<Vec3> maybePos = GetLocation(landmark, m_DocumentIdentifier);
            if (!maybePos) {
                return;  // landmark doesn't have a source/destination
            }

            // hittest the landmark as an analytic sphere
            const Sphere landmarkSphere = {.origin = *maybePos, .radius = m_LandmarkRadius};
            if (const auto collision = find_collision(cameraRay, landmarkSphere))
            {
                if (!closest || length(closest->getWorldSpaceLocation() - cameraRay.origin) > collision->distance)
                {
                    TPSDocumentElementID fullID{landmark.uid, TPSDocumentElementType::Landmark, m_DocumentIdentifier};
                    closest.emplace(std::move(fullID), *maybePos);
                }
            }
        }

        // 3D hittests non-participating landmarks and updates `closest` if a closer collision is found
        void hittestNonParticipatingLandmarks(
            const Line& cameraRay,
            std::optional<MeshWarpingTabHover>& closest) const
        {
            for (const auto& nonPariticpatingLandmark : m_State->getScratch().nonParticipatingLandmarks)
            {
                hittestNonParticipatingLandmark(cameraRay, closest, nonPariticpatingLandmark);
            }
        }

        // 3D hittests one non-participating landmark and updates `closest` if a closer collision is found
        void hittestNonParticipatingLandmark(
            const Line& cameraRay,
            std::optional<MeshWarpingTabHover>& closest,
            const TPSDocumentNonParticipatingLandmark& nonPariticpatingLandmark) const
        {
            // hittest non-participating landmark as an analytic sphere

            const Sphere decorationSphere = {
                .origin = nonPariticpatingLandmark.location,
                .radius = GetNonParticipatingLandmarkScaleFactor()*m_LandmarkRadius
            };

            if (const auto collision = find_collision(cameraRay, decorationSphere))
            {
                if (!closest || length(closest->getWorldSpaceLocation() - cameraRay.origin) > collision->distance)
                {
                    TPSDocumentElementID fullID{nonPariticpatingLandmark.uid, TPSDocumentElementType::NonParticipatingLandmark, m_DocumentIdentifier};
                    closest.emplace(std::move(fullID), nonPariticpatingLandmark.location);
                }
            }
        }

        // renders this panel's 3D scene to a texture
        RenderTexture& renderScene(
            Vec2 dims,
            const std::optional<RayCollision>& maybeMeshCollision,
            const std::optional<MeshWarpingTabHover>& maybeLandmarkCollision)
        {
            SceneRendererParams params = calc_standard_dark_scene_render_params(
                m_Camera,
                App::get().anti_aliasing_level(),
                dims,
                App::settings().get_value<float>("graphics/render_scale", 1.0f) * App::get().main_window_device_pixel_ratio()
            );
            m_State->getCustomRenderingOptions().applyTo(params);
            const std::vector<SceneDecoration> decorations = generateDecorations(maybeMeshCollision, maybeLandmarkCollision);
            return m_CachedRenderer.render(decorations, params);
        }

        // returns a fresh list of 3D decorations for this panel's 3D render
        std::vector<SceneDecoration> generateDecorations(
            const std::optional<RayCollision>& maybeMeshCollision,
            const std::optional<MeshWarpingTabHover>& maybeLandmarkCollision) const
        {
            std::vector<SceneDecoration> decorations;
            decorations.reserve(
                6 +
                CountNumLandmarksForInput(m_State->getScratch(), m_DocumentIdentifier) +
                m_State->getScratch().nonParticipatingLandmarks.size()
            );

            const std::function<void(SceneDecoration&&)> decorationConsumer =
                [&decorations](SceneDecoration&& dec) { decorations.push_back(std::move(dec)); };

            // generate common decorations (mesh, wireframe, grid, etc.)
            AppendCommonDecorations(
                *m_State,
                m_State->getScratchMesh(m_DocumentIdentifier),
                m_State->isWireframeModeEnabled(),
                decorationConsumer
            );

            // generate decorations for all of the landmarks
            generateDecorationsForLandmarks(decorationConsumer);

            // if applicable, generate decorations for the non-participating landmarks
            generateDecorationsForNonParticipatingLandmarks(decorationConsumer);

            // if applicable, show a mouse-to-mesh collision as faded landmark as a placement hint for user
            if (maybeMeshCollision && !maybeLandmarkCollision)
            {
                generateDecorationsForMouseOverMeshHover(maybeMeshCollision->position, decorationConsumer);
            }

            return decorations;
        }

        void generateDecorationsForLandmarks(
            const std::function<void(SceneDecoration&&)>& decorationConsumer) const
        {
            for (const TPSDocumentLandmarkPair& landmarkPair : m_State->getScratch().landmarkPairs)
            {
                generateDecorationsForLandmark(landmarkPair, decorationConsumer);
            }
        }

        void generateDecorationsForLandmark(
            const TPSDocumentLandmarkPair& landmarkPair,
            const std::function<void(SceneDecoration&&)>& decorationConsumer) const
        {
            const std::optional<Vec3> location = GetLocation(landmarkPair, m_DocumentIdentifier);
            if (not location) {
                return;  // no source/destination location for the landmark
            }

            const Color color = IsFullyPaired(landmarkPair) ? m_State->getPairedLandmarkColor() : m_State->getUnpairedLandmarkColor();
            SceneDecoration decoration{
                .mesh = m_State->getLandmarkSphereMesh(),
                .transform = {.scale = Vec3{m_LandmarkRadius}, .position = *location},
                .shading = color,
            };

            const TPSDocumentElementID landmarkID{landmarkPair.uid, TPSDocumentElementType::Landmark, m_DocumentIdentifier};
            if (m_State->isSelected(landmarkID)) {
                decoration.flags |= SceneDecorationFlag::RimHighlight0;
            }
            if (m_State->isHovered(landmarkID)) {
                decoration.shading = to_srgb_colorspace(saturate(multiply_luminance(to_linear_colorspace(color), 1.2f)));
                decoration.flags |= SceneDecorationFlag::RimHighlight1;
            }

            decorationConsumer(std::move(decoration));
        }

        void generateDecorationsForNonParticipatingLandmarks(
            const std::function<void(SceneDecoration&&)>& decorationConsumer) const
        {
            if (m_DocumentIdentifier != TPSDocumentInputIdentifier::Source) {
                return;  // only show them on the source (to-be-warped) mesh
            }

            for (const auto& nonParticipatingLandmark : m_State->getScratch().nonParticipatingLandmarks) {
                generateDecorationsForNonParticipatingLandmark(nonParticipatingLandmark, decorationConsumer);
            }
        }

        void generateDecorationsForNonParticipatingLandmark(
            const TPSDocumentNonParticipatingLandmark& npl,
            const std::function<void(SceneDecoration&&)>& decorationConsumer) const
        {
            const Color color = m_State->getNonParticipatingLandmarkColor();
            SceneDecoration decoration{
                .mesh = m_State->getLandmarkSphereMesh(),
                .transform = {
                    .scale = Vec3{GetNonParticipatingLandmarkScaleFactor()*m_LandmarkRadius},
                    .position = npl.location,
                },
                .shading = color,
            };
            const TPSDocumentElementID id{npl.uid, TPSDocumentElementType::NonParticipatingLandmark, m_DocumentIdentifier};
            if (m_State->isSelected(id)) {
                decoration.flags |= SceneDecorationFlag::RimHighlight0;
            }
            if (m_State->isHovered(id)) {
                decoration.shading = to_srgb_colorspace(saturate(multiply_luminance(to_linear_colorspace(color), 1.2f)));
                decoration.flags |= SceneDecorationFlag::RimHighlight1;
            }

            decorationConsumer(std::move(decoration));
        }

        void generateDecorationsForMouseOverMeshHover(
            const Vec3& meshCollisionPosition,
            const std::function<void(SceneDecoration&&)>& decorationConsumer) const
        {
            const bool nonParticipating = isUserPlacingNonParticipatingLandmark();

            const Color color = nonParticipating ?
                m_State->getNonParticipatingLandmarkColor() :
                m_State->getUnpairedLandmarkColor();

            const float radius = nonParticipating ?
                GetNonParticipatingLandmarkScaleFactor()*m_LandmarkRadius :
                m_LandmarkRadius;

            decorationConsumer(SceneDecoration{
                .mesh = m_State->getLandmarkSphereMesh(),
                .transform = {.scale = Vec3{radius}, .position = meshCollisionPosition},
                .shading = color.with_alpha(0.8f),  // faded
            });
        }

        // handle any input-related side-effects
        void handleInputAndHoverEvents(
            const ui::HittestResult& htResult,
            const std::optional<RayCollision>& meshCollision,
            const std::optional<MeshWarpingTabHover>& landmarkCollision)
        {
            // event: if the user left-clicks and something is hovered, select it; otherwise, add a landmark
            if (htResult.is_left_click_released_without_dragging)
            {
                if (landmarkCollision && landmarkCollision->isHoveringASceneElement())
                {
                    if (!ui::is_shift_down())
                    {
                        m_State->clearSelection();
                    }
                    m_State->select(*landmarkCollision->getSceneElementID());
                }
                else if (meshCollision)
                {
                    const auto pos = meshCollision->position;
                    if (isUserPlacingNonParticipatingLandmark())
                    {
                        ActionAddNonParticipatingLandmark(m_State->updUndoable(), pos);
                    }
                    else
                    {
                        ActionAddLandmark(m_State->updUndoable(), m_DocumentIdentifier, pos);
                    }
                }
            }

            // event: if the user right-clicks on a landmark then open the context menu for that landmark
            if (htResult.is_right_click_released_without_dragging)
            {
                if (landmarkCollision && landmarkCollision->isHoveringASceneElement())
                {
                    m_State->emplacePopup<MeshWarpingTabContextMenu>(
                        this,
                        "##MeshInputContextMenu",
                        m_State,
                        *landmarkCollision->getSceneElementID()
                    );
                }
            }

            // event: if the user is hovering the render while something is selected and the user
            // presses delete then the landmarks should be deleted
            if (htResult.is_hovered && ui::any_of_keys_pressed({Key::Delete, Key::Backspace}))
            {
                ActionDeleteSceneElementsByID(
                    m_State->updUndoable(),
                    m_State->getUnderlyingSelectionSet()
                );
                m_State->clearSelection();
            }
        }

        // 2D UI stuff (buttons, sliders, tables, etc.):

        // draws 2D ImGui overlays over the scene render
        void draw2DOverlayUI(const Rect& renderRect)
        {
            ui::set_cursor_ui_pos(renderRect.p1 + m_State->getOverlayPadding());

            drawInformationIcon();
            ui::same_line();
            drawImportButton();
            ui::same_line();
            drawExportButton();
            ui::same_line();
            drawAutoFitCameraButton();
            ui::same_line();
            drawLandmarkRadiusSlider();
        }

        // draws a information icon that shows basic mesh info when hovered
        void drawInformationIcon()
        {
            ui::draw_button_nobg(OSC_ICON_INFO_CIRCLE);
            if (ui::is_item_hovered())
            {
                ui::begin_tooltip();

                ui::draw_text_disabled("Input Information:");
                drawInputInformationTable();

                ui::end_tooltip();
            }
        }

        // draws a table containing useful input information (handy for debugging)
        void drawInputInformationTable()
        {
            if (ui::begin_table("##inputinfo", 2))
            {
                ui::table_setup_column("Name");
                ui::table_setup_column("Value");

                ui::table_next_row();
                ui::table_set_column_index(0);
                ui::draw_text("# landmarks");
                ui::table_set_column_index(1);
                ui::draw_text("%zu", CountNumLandmarksForInput(m_State->getScratch(), m_DocumentIdentifier));

                ui::table_next_row();
                ui::table_set_column_index(0);
                ui::draw_text("# vertices");
                ui::table_set_column_index(1);
                ui::draw_text("%zu", m_State->getScratchMesh(m_DocumentIdentifier).num_vertices());

                ui::table_next_row();
                ui::table_set_column_index(0);
                ui::draw_text("# triangles");
                ui::table_set_column_index(1);
                ui::draw_text("%zu", m_State->getScratchMesh(m_DocumentIdentifier).num_indices()/3);

                ui::end_table();
            }
        }

        // draws an import button that enables the user to import things for this input
        void drawImportButton()
        {
            ui::draw_button(OSC_ICON_FILE_IMPORT " import" OSC_ICON_CARET_DOWN);
            if (ui::begin_popup_context_menu("##importcontextmenu", ui::PopupFlag::MouseButtonLeft))
            {
                if (ui::draw_menu_item("Mesh File"))
                {
                    ActionPromptUserToLoadMeshFile(m_State->getUndoableSharedPtr(), m_DocumentIdentifier);
                }
                if (ui::begin_menu("Generated Mesh")) {
                    drawGeneratedMeshOptions(SolidGeometries{});
                    ui::end_menu();
                }
                if (ui::draw_menu_item("Landmarks from CSV"))
                {
                    ActionPromptUserToLoadLandmarksFromCSV(m_State->getUndoableSharedPtr(), m_DocumentIdentifier);
                }
                if (m_DocumentIdentifier == TPSDocumentInputIdentifier::Source &&
                    ui::draw_menu_item("Non-Participating Landmarks from CSV"))
                {
                    ActionPromptUserToLoadNonParticipatingLandmarksFromCSV(m_State->getUndoableSharedPtr());
                }
                ui::end_popup();
            }
        }

        template<typename... Geometries>
        void drawGeneratedMeshOptions(Typelist<Geometries...>)
        {
            (drawGeneratedMeshOption<Geometries>(), ...);
        }

        template<typename Geometry>
        void drawGeneratedMeshOption()
        {
            if (ui::draw_menu_item(Geometry::name())) {
                ActionLoadMesh(m_State->updUndoable(), Geometry{}, m_DocumentIdentifier);
            }
        }

        // draws an export button that enables the user to export things from this input
        void drawExportButton()
        {
            ui::draw_button(OSC_ICON_FILE_EXPORT " export" OSC_ICON_CARET_DOWN);
            if (ui::begin_popup_context_menu("##exportcontextmenu", ui::PopupFlag::MouseButtonLeft))
            {
                if (ui::draw_menu_item("Mesh to OBJ"))
                {
                    ActionPromptUserToSaveMeshToObjFile(m_State->getScratchMesh(m_DocumentIdentifier), ObjWriterFlag::Default);
                }
                if (ui::draw_menu_item("Mesh to OBJ (no normals)"))
                {
                    ActionPromptUserToSaveMeshToObjFile(m_State->getScratchMesh(m_DocumentIdentifier), ObjWriterFlag::NoWriteNormals);
                }
                if (ui::draw_menu_item("Mesh to STL"))
                {
                    ActionPromptUserToMeshToStlFile(m_State->getScratchMesh(m_DocumentIdentifier));
                }
                if (ui::draw_menu_item("Landmarks to CSV"))
                {
                    ActionPromptUserToSaveLandmarksToCSV(m_State->getScratch(), m_DocumentIdentifier);
                }
                if (ui::draw_menu_item("Landmark Positions to CSV"))
                {
                    ActionPromptUserToSaveLandmarksToCSV(m_State->getScratch(), m_DocumentIdentifier, LandmarkCSVFlags::NoHeader | LandmarkCSVFlags::NoNames);
                }
                if (m_DocumentIdentifier == TPSDocumentInputIdentifier::Source)
                {
                    if (ui::draw_menu_item("Non-Participating Landmarks to CSV"))
                    {
                        ActionPromptUserToSaveNonParticipatingLandmarksToCSV(m_State->getScratch());
                    }
                    if (ui::draw_menu_item("Non-Participating Landmark Positions to CSV"))
                    {
                        ActionPromptUserToSaveNonParticipatingLandmarksToCSV(m_State->getScratch(), LandmarkCSVFlags::NoHeader | LandmarkCSVFlags::NoNames);
                    }
                }
                ui::end_popup();
            }
        }

        // draws a button that auto-fits the camera to the 3D scene
        void drawAutoFitCameraButton()
        {
            if (ui::draw_button(OSC_ICON_EXPAND_ARROWS_ALT))
            {
                auto_focus(m_Camera, m_State->getScratchMesh(m_DocumentIdentifier).bounds(), aspect_ratio_of(m_LastTextureHittestResult.item_ui_rect));
                m_State->setLinkedBaseCamera(m_Camera);
            }
            ui::draw_tooltip_if_item_hovered("Autoscale Scene", "Zooms camera to try and fit everything in the scene into the viewer");
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

        bool isUserPlacingNonParticipatingLandmark() const
        {
            static_assert(num_options<TPSDocumentInputIdentifier>() == 2);
            const bool isSourceMesh = m_DocumentIdentifier == TPSDocumentInputIdentifier::Source;
            const bool isCtrlPressed = ui::any_of_keys_down({Key::LeftCtrl, Key::RightCtrl});
            return isSourceMesh && isCtrlPressed;
        }

        std::shared_ptr<MeshWarpingTabSharedState> m_State;
        TPSDocumentInputIdentifier m_DocumentIdentifier;
        PolarPerspectiveCamera m_Camera = create_camera_focused_on(m_State->getScratchMesh(m_DocumentIdentifier).bounds());
        CachedSceneRenderer m_CachedRenderer{
            *App::singleton<SceneCache>(App::resource_loader()),
        };
        ui::HittestResult m_LastTextureHittestResult;
        float m_LandmarkRadius = 0.05f;
    };
}
