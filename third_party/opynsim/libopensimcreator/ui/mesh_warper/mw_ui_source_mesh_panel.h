#pragma once

#include <libopensimcreator/documents/mesh_warper/mw_document_input_identifier.h>
#include <libopensimcreator/documents/mesh_warper/mw_document_landmark_pair.h>
#include <libopensimcreator/documents/mesh_warper/mw_undoable_actions.h>
#include <libopensimcreator/platform/msmicons.h>
#include <libopensimcreator/ui/mesh_warper/mw_ui_landmark_context_menu.h>
#include <libopensimcreator/ui/mesh_warper/mw_ui_decoration_generators.h>
#include <libopensimcreator/ui/mesh_warper/mw_ui_hovertest_result.h>
#include <libopensimcreator/ui/mesh_warper/mw_ui_mesh_context_menu.h>
#include <libopensimcreator/ui/mesh_warper/mw_ui_panel.h>
#include <libopensimcreator/ui/mesh_warper/mw_ui_shared_state.h>

#include <liboscar/graphics/geometries/solid_geometries.h>
#include <liboscar/graphics/scene/cached_scene_renderer.h>
#include <liboscar/graphics/scene/scene_cache.h>
#include <liboscar/graphics/scene/scene_decoration.h>
#include <liboscar/graphics/scene/scene_helpers.h>
#include <liboscar/graphics/scene/scene_renderer_params.h>
#include <liboscar/graphics/camera.h>
#include <liboscar/graphics/color.h>
#include <liboscar/graphics/mesh.h>
#include <liboscar/graphics/orbit_camera_controller.h>
#include <liboscar/maths/collision_tests.h>
#include <liboscar/maths/math_helpers.h>
#include <liboscar/maths/ray.h>
#include <liboscar/maths/ray_collision.h>
#include <liboscar/maths/rect.h>
#include <liboscar/maths/rect_functions.h>
#include <liboscar/maths/sphere.h>
#include <liboscar/maths/transform.h>
#include <liboscar/maths/vector.h>
#include <liboscar/platform/app.h>
#include <liboscar/platform/app_settings.h>
#include <liboscar/ui/oscimgui.h>
#include <liboscar/utilities/c_string_view.h>
#include <liboscar/utilities/typelist.h>

#include <functional>
#include <memory>
#include <optional>
#include <string_view>
#include <utility>
#include <vector>

namespace osc
{
    // an "input" panel (i.e. source or destination mesh, before warping)
    class MwUiSourceMeshPanel final : public MwUiPanel {
    public:
        explicit MwUiSourceMeshPanel(
            Widget* parent,
            std::string_view panel_name,
            std::shared_ptr<MwUiSharedState> state,
            MiDocumentInputIdentifier document_identifier) :

            MwUiPanel{parent, panel_name},
            state_{std::move(state)},
            document_identifier_{document_identifier}
        {
            camera_.set_vertical_field_of_view(Degrees{35.0f});
            camera_controller_.focus_on(
                state_->get_scratch_mesh(document_identifier_).bounds().value_or(AABB{}), camera_);
            camera_controller_.update_camera(camera_);
        }

    private:
        // draws all of the panel's content
        void impl_draw_content() override
        {
            // compute top-level UI variables (render rect, mouse position, etc.)
            const Rect content_rect = ui::get_content_region_available_ui_rect();

            // un-project mouse's (2D) location into the 3D scene as a ray
            const Ray camera_ray = camera_.ui_to_world(ui::get_mouse_ui_position(), content_rect);

            // mesh hittest: compute whether the user is hovering over the mesh (affects rendering)
            const Mesh& input_mesh = state_->get_scratch_mesh(document_identifier_);
            const BVH& input_mesh_bvh = state_->get_scratch_mesh_bvh(document_identifier_);
            std::optional<RayCollision> mesh_collision =
                last_texture_hittest_result_.is_hovered
                    ? get_closest_world_space_ray_triangle_collision(
                          input_mesh, input_mesh_bvh, Transform{}, camera_ray)
                    : std::nullopt;

            // landmark hittest: compute whether the user is hovering over a landmark
            std::optional<MwUiHovertestResult> landmark_collision =
                last_texture_hittest_result_.is_hovered ? get_mouse_landmark_collisions(camera_ray)
                                                        : std::nullopt;

            if (manipulator_gizmo_.is_over() or manipulator_gizmo_.is_using()) {
                landmark_collision.reset();
                mesh_collision.reset();
            }

            // state update: tell central state if something's being hovered in this panel
            if (landmark_collision) {
                state_->set_hover(landmark_collision);
            } else if (mesh_collision) {
                state_->set_hover(document_identifier_, mesh_collision->position);
            }

            // update camera: NOTE: make sure it's updated *before* rendering; otherwise, it'll be
            // one frame late
            if (not manipulator_gizmo_.is_using()) {
                update_camera();
            }

            // render 3D: draw the scene into the content rect and 2D-hittest it
            RenderTexture& render_texture =
                render_scene(content_rect.dimensions(), mesh_collision, landmark_collision);
            ui::draw_image(render_texture);
            last_texture_hittest_result_ = ui::hittest_last_drawn_item();

            // handle any events due to hovering over, clicking, etc.
            handle_input_and_hover_events(
                last_texture_hittest_result_, mesh_collision, landmark_collision);

            // render 2D: draw any 2D overlays over the 3D render
            draw_2d_overlay_ui(last_texture_hittest_result_.item_ui_rect);
        }

        // update the 3D camera from user inputs/external data
        void update_camera()
        {
            // if the cameras are linked together, ensure this camera is updated from the linked
            // camera
            if (state_->update_local_camera_controller_from_linked_base(camera_controller_)) {
                camera_controller_.update_camera(camera_);
            }

            // if the user interacts with the render, update the camera as necessary
            if (last_texture_hittest_result_.is_hovered and
                ui::update_orbit_controller_from_mouse_inputs(
                    camera_controller_,
                    camera_,
                    last_texture_hittest_result_.item_ui_rect.dimensions())) {

                camera_controller_.update_camera(camera_);
                state_->set_linked_camera_controller(camera_controller_);
            }
        }

        // returns the closest collision, if any, between the provided camera ray and a landmark
        std::optional<MwUiHovertestResult> get_mouse_landmark_collisions(
            const Ray& camera_ray) const
        {
            std::optional<MwUiHovertestResult> rv;
            hittest_landmarks(camera_ray, rv);
            hittest_non_participating_landmarks(camera_ray, rv);
            return rv;
        }

        // 3D hittests landmarks and updates `closest` if a closer collision is found
        void hittest_landmarks(
            const Ray& camera_ray, std::optional<MwUiHovertestResult>& closest) const
        {
            for (const MwDocumentLandmarkPair& landmark : state_->get_scratch().landmark_pairs) {
                hittest_landmark(camera_ray, closest, landmark);
            }
        }

        // 3D hittests one landmark and updates `closest` if a closer collision is found
        void hittest_landmark(
            const Ray& camera_ray,
            std::optional<MwUiHovertestResult>& closest,
            const MwDocumentLandmarkPair& landmark) const
        {
            const std::optional<Vector3> maybe_pos = get_location(landmark, document_identifier_);
            if (!maybe_pos) {
                return;  // landmark doesn't have a source/destination
            }

            // hittest the landmark as an analytic sphere
            const Sphere landmark_sphere = {.origin = *maybe_pos, .radius = landmark_radius_};
            if (const auto collision = find_collision(camera_ray, landmark_sphere)) {
                if (not closest or length(closest->get_world_space_location() - camera_ray.origin) >
                                       collision->distance) {

                    MwDocumentElementID full_id{
                        landmark.uid, MwDocumentElementType::Landmark, document_identifier_};
                    closest.emplace(full_id, *maybe_pos);
                }
            }
        }

        // 3D hittests non-participating landmarks and updates `closest` if a closer collision is
        // found
        void hittest_non_participating_landmarks(
            const Ray& camera_ray, std::optional<MwUiHovertestResult>& closest) const
        {
            for (const auto& non_pariticpating_landmark :
                 state_->get_scratch().non_participating_landmarks) {

                hittest_non_participating_landmark(camera_ray, closest, non_pariticpating_landmark);
            }
        }

        // 3D hittests one non-participating landmark and updates `closest` if a closer collision is
        // found
        void hittest_non_participating_landmark(
            const Ray& camera_ray,
            std::optional<MwUiHovertestResult>& closest,
            const MwDocumentNonParticipatingLandmark& non_pariticpating_landmark) const
        {
            // hittest non-participating landmark as an analytic sphere

            const Sphere decoration_sphere = {
                .origin = non_pariticpating_landmark.location,
                .radius = get_non_participating_landmark_scale_factor() * landmark_radius_,
            };

            if (const auto collision = find_collision(camera_ray, decoration_sphere)) {
                if (not closest or length(closest->get_world_space_location() - camera_ray.origin) >
                                       collision->distance) {
                    MwDocumentElementID full_id{
                        non_pariticpating_landmark.uid,
                        MwDocumentElementType::NonParticipatingLandmark,
                        document_identifier_};
                    closest.emplace(full_id, non_pariticpating_landmark.location);
                }
            }
        }

        // renders this panel's 3D scene to a texture
        RenderTexture& render_scene(
            Vector2 dims,
            const std::optional<RayCollision>& maybe_mesh_collision,
            const std::optional<MwUiHovertestResult>& maybe_landmark_collision)
        {
            SceneRendererParams params = calc_standard_dark_scene_render_params(
                camera_,
                App::get().anti_aliasing_level(),
                dims,
                App::settings().get_value<float>("graphics/render_scale", 1.0f) *
                    App::get().main_window_device_pixel_ratio());
            state_->get_custom_rendering_options().apply_to(params);
            const std::vector<SceneDecoration> decorations =
                generate_decorations(maybe_mesh_collision, maybe_landmark_collision);
            return cached_renderer_.render(decorations, params);
        }

        // returns a fresh list of 3D decorations for this panel's 3D render
        std::vector<SceneDecoration> generate_decorations(
            const std::optional<RayCollision>& maybe_mesh_collision,
            const std::optional<MwUiHovertestResult>& maybe_landmark_collision) const
        {
            std::vector<SceneDecoration> decorations;
            decorations.reserve(
                6 + count_num_landmarks_for_input(state_->get_scratch(), document_identifier_) +
                state_->get_scratch().non_participating_landmarks.size());

            const std::function<void(SceneDecoration&&)> decoration_consumer =
                [&decorations](SceneDecoration&& dec) {
                    decorations.push_back(std::move(dec));
                };

            // generate common decorations (mesh, wireframe, grid, etc.)
            append_common_decorations(
                *state_,
                state_->get_scratch_mesh(document_identifier_),
                state_->is_wireframe_mode_enabled(),
                decoration_consumer);

            // generate decorations for all of the landmarks
            generate_decorations_for_landmarks(decoration_consumer);

            // if applicable, generate decorations for the non-participating landmarks
            generate_decorations_for_non_participating_landmarks(decoration_consumer);

            // if applicable, show a mouse-to-mesh collision as faded landmark as a placement hint
            // for user
            if (maybe_mesh_collision and not maybe_landmark_collision) {
                generate_decorations_for_mouse_over_mesh_hover(
                    maybe_mesh_collision->position, decoration_consumer);
            }

            return decorations;
        }

        void generate_decorations_for_landmarks(
            const std::function<void(SceneDecoration&&)>& decoration_consumer) const
        {
            for (const MwDocumentLandmarkPair& landmark_pair :
                 state_->get_scratch().landmark_pairs) {

                generate_decorations_for_landmark(landmark_pair, decoration_consumer);
            }
        }

        void generate_decorations_for_landmark(
            const MwDocumentLandmarkPair& landmark_pair,
            const std::function<void(SceneDecoration&&)>& decoration_consumer) const
        {
            const std::optional<Vector3> location =
                get_location(landmark_pair, document_identifier_);
            if (not location) {
                return;  // no source/destination location for the landmark
            }

            const Color color = is_fully_paired(landmark_pair)
                                    ? state_->get_paired_landmark_color()
                                    : state_->get_unpaired_landmark_color();
            SceneDecoration decoration{
                .mesh = state_->get_landmark_sphere_mesh(),
                .transform = {.scale = Vector3{landmark_radius_}, .translation = *location},
                .shading = color,
            };

            const MwDocumentElementID landmark_id{
                landmark_pair.uid, MwDocumentElementType::Landmark, document_identifier_};
            if (state_->is_selected(landmark_id)) {
                decoration.flags |= SceneDecorationFlag::RimHighlight0;
            }
            if (state_->is_hovered(landmark_id)) {
                decoration.shading = to_srgb_colorspace(
                    saturate(multiply_luminance(to_linear_colorspace(color), 1.2f)));
                decoration.flags |= SceneDecorationFlag::RimHighlight1;
            }

            decoration_consumer(std::move(decoration));
        }

        void generate_decorations_for_non_participating_landmarks(
            const std::function<void(SceneDecoration&&)>& decoration_consumer) const
        {
            if (document_identifier_ != MiDocumentInputIdentifier::Source) {
                return;  // only show them on the source (to-be-warped) mesh
            }

            for (const auto& non_participating_landmark :
                 state_->get_scratch().non_participating_landmarks) {

                generate_decorations_for_non_participating_landmark(
                    non_participating_landmark, decoration_consumer);
            }
        }

        void generate_decorations_for_non_participating_landmark(
            const MwDocumentNonParticipatingLandmark& npl,
            const std::function<void(SceneDecoration&&)>& decoration_consumer) const
        {
            const Color color = state_->get_non_participating_landmark_color();
            const auto sf = get_non_participating_landmark_scale_factor();
            SceneDecoration decoration{
                .mesh = state_->get_landmark_sphere_mesh(),
                .transform =
                    {
                        .scale = Vector3{sf * landmark_radius_},
                        .translation = npl.location,
                    },
                .shading = color,
            };
            const MwDocumentElementID id{
                npl.uid, MwDocumentElementType::NonParticipatingLandmark, document_identifier_};
            if (state_->is_selected(id)) {
                decoration.flags |= SceneDecorationFlag::RimHighlight0;
            }
            if (state_->is_hovered(id)) {
                decoration.shading = to_srgb_colorspace(
                    saturate(multiply_luminance(to_linear_colorspace(color), 1.2f)));
                decoration.flags |= SceneDecorationFlag::RimHighlight1;
            }

            decoration_consumer(std::move(decoration));
        }

        void generate_decorations_for_mouse_over_mesh_hover(
            const Vector3& mesh_collision_position,
            const std::function<void(SceneDecoration&&)>& decoration_consumer) const
        {
            const bool non_participating = is_user_placing_non_participating_landmark();

            const Color color = non_participating ? state_->get_non_participating_landmark_color()
                                                  : state_->get_unpaired_landmark_color();

            const float radius =
                non_participating ? get_non_participating_landmark_scale_factor() * landmark_radius_
                                  : landmark_radius_;

            decoration_consumer(
                SceneDecoration{
                    .mesh = state_->get_landmark_sphere_mesh(),
                    .transform = {.scale = Vector3{radius}, .translation = mesh_collision_position},
                    .shading = color.with_alpha(0.8f),  // faded
                });
        }

        // handle any input-related side-effects
        void handle_input_and_hover_events(
            const ui::HittestResult& ht_result,
            const std::optional<RayCollision>& mesh_collision,
            const std::optional<MwUiHovertestResult>& landmark_collision)
        {
            // event: if the user left-clicks and something is hovered, select it; otherwise, add a
            // landmark
            if (ht_result.is_left_click_released_without_dragging) {
                if (landmark_collision and landmark_collision->is_hovering_a_scene_element()) {
                    if (not ui::is_shift_down()) {
                        state_->clear_selection();
                    }
                    state_->select(landmark_collision->get_scene_element_id().value());
                } else if (mesh_collision) {
                    const auto position = mesh_collision->position;
                    if (is_user_placing_non_participating_landmark()) {
                        action_add_non_participating_landmark(state_->upd_undoable(), position);
                    } else {
                        action_add_landmark(state_->upd_undoable(), document_identifier_, position);
                    }
                }
            }

            // event: if the user right-clicks on a landmark then open the context menu for that
            // landmark
            if (ht_result.is_right_click_released_without_dragging) {
                if (landmark_collision && landmark_collision->is_hovering_a_scene_element()) {
                    state_->emplace_popup<MwUiLandmarkContextMenu>(
                        this,
                        "##MeshInputLandmarkContextMenu",
                        state_,
                        landmark_collision->get_scene_element_id().value());
                } else if (mesh_collision) {
                    state_->emplace_popup<MwUiMeshContextMenu>(
                        this,
                        "##MeshInputMeshContextMenu",
                        state_,
                        document_identifier_,
                        *mesh_collision);
                }
            }

            // event: if the user is hovering the render while something is selected and the user
            // presses delete then the landmarks should be deleted
            if (ht_result.is_hovered && ui::any_of_keys_pressed({Key::Delete, Key::Backspace})) {
                action_delete_scene_elements_by_id(
                    state_->upd_undoable(), state_->get_underlying_selection_set());
                state_->clear_selection();
            }

            // event: if the user is hovering the mesh and they press F then they would like to
            // focus the camera on that point
            if (mesh_collision and ui::any_of_keys_pressed({Key::F})) {
                camera_controller_.focus_point = mesh_collision->position;
                camera_controller_.update_camera(camera_);
                state_->set_linked_camera_controller(camera_controller_);
            }
        }

        // 2D UI stuff (buttons, sliders, tables, etc.):

        // draws 2D ImGui overlays over the scene render
        void draw_2d_overlay_ui(const Rect& render_rect)
        {
            ui::push_id(std::to_underlying(document_identifier_));

            // Draw the manipulation gizmo before anything else
            draw_manipulation_gizmo(render_rect);

            ui::set_cursor_ui_position(render_rect.ypd_top_left() + state_->get_overlay_padding());

            draw_information_icon();
            ui::same_line();
            draw_import_button();
            ui::same_line();
            draw_export_button();
            ui::same_line();
            draw_auto_fit_camera_button();
            ui::same_line();
            draw_landmark_radius_slider();

            ui::pop_id();
        }

        void draw_manipulation_gizmo(const Rect& render_rect)
        {
            const auto locations = state_->get_selection_landmark_locations(document_identifier_);
            if (locations.empty()) {
                return;  // Only draw the gizmo if something's selected
            }

            // Calculate mean centroid of the selection
            Vector3d mean = locations.front();
            size_t i = 1;
            for (auto it = locations.begin() + 1; it != locations.end(); ++it) {
                mean += *it;
                ++i;
            }
            mean /= static_cast<double>(i);

            // Draw a gizmo that manipulates the mean centroid
            manipulator_gizmo_.set_operation(ui::GizmoOperation::Translate);
            Matrix4x4 model_matrix = translate(identity<Matrix4x4>(), Vector3{mean});
            const auto user_transform = manipulator_gizmo_.draw(
                model_matrix,
                camera_.view_matrix(),
                camera_.projection_matrix(aspect_ratio_of(render_rect)),
                render_rect);
            if (user_transform) {
                action_translate_landmarks_dont_save(
                    state_->upd_undoable(),
                    state_->get_selected(document_identifier_),
                    user_transform->translation);
            }
            if (manipulator_gizmo_.was_using() and not manipulator_gizmo_.is_using()) {
                action_save_landmark_translation(
                    state_->upd_undoable(), state_->get_selected(document_identifier_));
            }
        }

        // draws a information icon that shows basic mesh info when hovered
        void draw_information_icon()
        {
            ui::draw_button_nobg(MSMICONS_INFO_CIRCLE);
            if (ui::is_item_hovered()) {
                ui::begin_tooltip();

                ui::draw_text_disabled("Input Information:");
                draw_input_information_table();

                ui::end_tooltip();
            }
        }

        // draws a table containing useful input information (handy for debugging)
        void draw_input_information_table()
        {
            if (ui::begin_table("##inputinfo", 2)) {
                ui::table_setup_column("Name");
                ui::table_setup_column("Value");

                ui::table_next_row();
                ui::table_set_column_index(0);
                ui::draw_text("# landmarks");
                ui::table_set_column_index(1);
                ui::draw_text(
                    "{}",
                    count_num_landmarks_for_input(state_->get_scratch(), document_identifier_));

                ui::table_next_row();
                ui::table_set_column_index(0);
                ui::draw_text("# vertices");
                ui::table_set_column_index(1);
                ui::draw_text("{}", state_->get_scratch_mesh(document_identifier_).num_vertices());

                ui::table_next_row();
                ui::table_set_column_index(0);
                ui::draw_text("# triangles");
                ui::table_set_column_index(1);
                ui::draw_text(
                    "{}", state_->get_scratch_mesh(document_identifier_).num_indices() / 3);

                ui::end_table();
            }
        }

        // draws an import button that enables the user to import things for this input
        void draw_import_button()
        {
            ui::draw_button(MSMICONS_FILE_IMPORT " import" MSMICONS_CARET_DOWN);
            if (ui::begin_popup_context_menu(
                    "##importcontextmenu", ui::PopupFlag::MouseButtonLeft)) {

                if (ui::draw_menu_item("Mesh File")) {
                    action_prompt_user_to_load_mesh_file(
                        state_->get_undoable_shared_ptr(), document_identifier_);
                }
                if (ui::begin_menu("Generated Mesh")) {
                    draw_generated_mesh_options(SolidGeometries{});
                    ui::end_menu();
                }
                if (ui::draw_menu_item("Landmarks from CSV")) {
                    action_prompt_user_to_load_landmarks_from_csv(
                        state_->get_undoable_shared_ptr(), document_identifier_);
                }
                if (document_identifier_ == MiDocumentInputIdentifier::Source &&
                    ui::draw_menu_item("Non-Participating Landmarks from CSV")) {
                    action_prompt_user_to_load_non_participating_landmarks_from_csv(
                        state_->get_undoable_shared_ptr());
                }
                ui::end_popup();
            }
        }

        template<typename... Geometries>
        void draw_generated_mesh_options(Typelist<Geometries...>)
        {
            (draw_generated_mesh_option<Geometries>(), ...);
        }

        template<typename Geometry>
        void draw_generated_mesh_option()
        {
            if (ui::draw_menu_item(Geometry::name())) {
                action_load_mesh(state_->upd_undoable(), Geometry{}, document_identifier_);
            }
        }

        // draws an export button that enables the user to export things from this input
        void draw_export_button()
        {
            ui::draw_button(MSMICONS_FILE_EXPORT " export" MSMICONS_CARET_DOWN);
            if (ui::begin_popup_context_menu(
                    "##exportcontextmenu", ui::PopupFlag::MouseButtonLeft)) {

                if (ui::draw_menu_item("Mesh to OBJ")) {
                    action_prompt_user_to_save_mesh_to_obj_file(
                        state_->get_scratch_mesh(document_identifier_), OBJWriterFlag::Default);
                }
                if (ui::draw_menu_item("Mesh to OBJ (no normals)")) {
                    action_prompt_user_to_save_mesh_to_obj_file(
                        state_->get_scratch_mesh(document_identifier_),
                        OBJWriterFlag::NoWriteNormals);
                }
                if (ui::draw_menu_item("Mesh to STL")) {
                    action_prompt_user_to_mesh_to_stl_file(
                        state_->get_scratch_mesh(document_identifier_));
                }
                if (ui::draw_menu_item("Landmarks to CSV")) {
                    action_prompt_user_to_save_landmarks_to_csv(
                        state_->get_scratch(), document_identifier_);
                }
                if (ui::draw_menu_item("Landmark Positions to CSV")) {
                    action_prompt_user_to_save_landmarks_to_csv(
                        state_->get_scratch(),
                        document_identifier_,
                        opyn::LandmarkCSVFlags::NoHeader | opyn::LandmarkCSVFlags::NoNames);
                }
                if (document_identifier_ == MiDocumentInputIdentifier::Source) {
                    if (ui::draw_menu_item("Non-Participating Landmarks to CSV")) {
                        action_prompt_user_to_save_non_participating_landmarks_to_csv(
                            state_->get_scratch());
                    }
                    if (ui::draw_menu_item("Non-Participating Landmark Positions to CSV")) {
                        action_prompt_user_to_save_non_participating_landmarks_to_csv(
                            state_->get_scratch(),
                            opyn::LandmarkCSVFlags::NoHeader | opyn::LandmarkCSVFlags::NoNames);
                    }
                }
                ui::end_popup();
            }
        }

        // draws a button that auto-fits the camera to the 3D scene
        void draw_auto_fit_camera_button()
        {
            if (ui::draw_button(MSMICONS_EXPAND_ARROWS_ALT)) {
                if (const auto bounds = state_->get_scratch_mesh(document_identifier_).bounds()) {
                    camera_controller_.focus_on(
                        *bounds,
                        camera_,
                        aspect_ratio_of(last_texture_hittest_result_.item_ui_rect));
                    camera_controller_.update_camera(camera_);
                    state_->set_linked_camera_controller(camera_controller_);
                }
            }
            ui::draw_tooltip_if_item_hovered(
                "Autoscale Scene",
                "Zooms camera to try and fit everything in the scene into the viewer");
        }

        // draws a slider that lets the user edit how large the landmarks are
        void draw_landmark_radius_slider()
        {
            // note: log scale is important: some users have meshes that
            // are in different scales (e.g. millimeters)
            const ui::SliderFlags flags = ui::SliderFlag::Logarithmic;

            const CStringView label = "landmark radius";
            ui::set_next_item_width(
                ui::get_content_region_available().x() - ui::calc_text_size(label).x() -
                ui::get_style_item_inner_spacing().x() - state_->get_overlay_padding().x());
            ui::draw_float_slider(label, &landmark_radius_, 0.0001f, 100.0f, "%.4f", flags);
        }

        bool is_user_placing_non_participating_landmark() const
        {
            static_assert(num_options_v<MiDocumentInputIdentifier> == 2);
            const bool is_source_mesh = document_identifier_ == MiDocumentInputIdentifier::Source;
            const bool is_ctrl_pressed = ui::any_of_keys_down({Key::LeftCtrl, Key::RightCtrl});
            return is_source_mesh && is_ctrl_pressed;
        }

        std::shared_ptr<MwUiSharedState> state_;
        MiDocumentInputIdentifier document_identifier_;
        Camera camera_;
        OrbitCameraController camera_controller_;
        CachedSceneRenderer cached_renderer_{
            *App::singleton<SceneCache>(App::resource_loader()),
        };
        ui::HittestResult last_texture_hittest_result_;
        ui::Gizmo manipulator_gizmo_;
        float landmark_radius_ = 0.05f;
    };
}
