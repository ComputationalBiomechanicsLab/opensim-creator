#pragma once

#include <libopensimcreator/documents/mesh_importer/mi_body.h>
#include <libopensimcreator/documents/mesh_importer/mi_crossref_direction.h>
#include <libopensimcreator/documents/mesh_importer/mi_document.h>
#include <libopensimcreator/documents/mesh_importer/mi_document_helpers.h>
#include <libopensimcreator/documents/mesh_importer/mi_ground.h>
#include <libopensimcreator/documents/mesh_importer/mi_joint.h>
#include <libopensimcreator/documents/mesh_importer/mi_mesh.h>
#include <libopensimcreator/documents/mesh_importer/mi_object.h>
#include <libopensimcreator/documents/mesh_importer/mi_x_open_sim.h>
#include <libopensimcreator/documents/mesh_importer/mi_station.h>
#include <libopensimcreator/documents/mesh_importer/mi_undoable_document.h>
#include <libopensimcreator/platform/msmicons.h>
#include <libopensimcreator/ui/mesh_importer/drawable_thing.h>
#include <libopensimcreator/ui/mesh_importer/mesh_importer_hover.h>
#include <libopensimcreator/ui/mesh_importer/mesh_loader.h>

#include <libopynsim/documents/file_filters.h>
#include <libopynsim/graphics/simbody_mesh_loader.h>
#include <liboscar/graphics/geometries/cone_geometry.h>
#include <liboscar/graphics/geometries/cylinder_geometry.h>
#include <liboscar/graphics/geometries/sphere_geometry.h>
#include <liboscar/graphics/materials/mesh_basic_material.h>
#include <liboscar/graphics/scene/scene_cache.h>
#include <liboscar/graphics/scene/scene_decoration.h>
#include <liboscar/graphics/scene/scene_decoration_flags.h>
#include <liboscar/graphics/scene/scene_helpers.h>
#include <liboscar/graphics/scene/scene_renderer.h>
#include <liboscar/graphics/scene/scene_renderer_params.h>
#include <liboscar/graphics/color.h>
#include <liboscar/graphics/camera.h>
#include <liboscar/graphics/material.h>
#include <liboscar/graphics/orbit_camera_controller.h>
#include <liboscar/maths/angle.h>
#include <liboscar/maths/collision_tests.h>
#include <liboscar/maths/quaternion_functions.h>
#include <liboscar/maths/ray.h>
#include <liboscar/maths/ray_collision.h>
#include <liboscar/maths/rect.h>
#include <liboscar/maths/rect_functions.h>
#include <liboscar/maths/sphere.h>
#include <liboscar/maths/transform.h>
#include <liboscar/maths/vector.h>
#include <liboscar/platform/app.h>
#include <liboscar/platform/app_settings.h>
#include <liboscar/platform/log.h>
#include <liboscar/ui/oscimgui.h>
#include <liboscar/ui/panels/perf_panel.h>
#include <liboscar/ui/tabs/tab_save_result.h>
#include <liboscar/ui/widgets/log_viewer.h>
#include <liboscar/utilities/c_string_view.h>
#include <liboscar/utilities/enum_helpers.h>
#include <liboscar/utilities/std_variant_helpers.h>
#include <liboscar/utilities/uid.h>
#include <OpenSim/Simulation/Model/Model.h>

#include <array>
#include <cstddef>
#include <exception>
#include <filesystem>
#include <future>
#include <memory>
#include <optional>
#include <span>
#include <sstream>
#include <string_view>
#include <string>
#include <unordered_set>
#include <utility>
#include <variant>
#include <vector>

namespace OpenSim
{
    class Model;
}

namespace osc
{
    class Event;

    // shared data support
    //
    // data that's shared between multiple UI states.
    class MeshImporterSharedState final
        : public std::enable_shared_from_this<MeshImporterSharedState> {
    public:
        explicit MeshImporterSharedState(Widget* parent) :
            MeshImporterSharedState{parent, std::vector<std::filesystem::path>{}}
        {}

        explicit MeshImporterSharedState(
            Widget* parent, std::vector<std::filesystem::path> mesh_files) :

            log_viewer_{parent},
            perf_panel_{parent}
        {
            floor_material_.set_transparent(true);
            push_mesh_load_requests(std::move(mesh_files));
        }

        //
        // OpenSim OUTPUT MODEL STUFF
        //

        bool has_output_model() const { return maybe_output_model_ != nullptr; }

        std::unique_ptr<OpenSim::Model>& upd_output_model() { return maybe_output_model_; }

        void try_create_output_model()
        {
            try {
                maybe_output_model_ = create_open_sim_model_from_mesh_importer_document(
                    get_model_graph(), model_creation_flags_, issues_buffer_);
            } catch (const std::exception& ex) {
                log_error(
                    "error occurred while trying to create an OpenSim model from the mesh editor scene: {}",
                    ex.what());
            }
        }

        //
        // MODEL GRAPH STUFF
        //

        void open_osim_file_as_model_graph()
        {
            if (not shared_from_this()) {
                log_critical(
                    "cannot open import dialog because the mesh importer's state isn't reference-counted");
                return;
            }

            App::upd().prompt_user_to_select_file_async(
                [state = shared_from_this()](const FileDialogResponse&& response) {
                    if (not state) {
                        return;  // Something went horribly wrong (should've been checked earlier).
                    }
                    if (response.size() != 1) {
                        return;  // Error, cancellation, or the user somehow selected >1 file.
                    }

                    // Wrap model import in a `try..catch` because the user may provide malformed
                    // data and we don't want the exception to propagate all the way up to the
                    // event loop (#1050).
                    try {
                        state->model_graph_snapshots_ =
                            MiUndoableDocument{create_model_from_osim_file(response.front())};
                        state->maybe_model_graph_export_location_ = response.front();
                        state->maybe_model_graph_exported_uid_ =
                            state->model_graph_snapshots_.head_id();
                    } catch (const std::exception& ex) {
                        log_error(
                            "Error importing {} as a model: {}",
                            response.front().string(),
                            ex.what());
                        return;  // Error importing the model
                    }
                },
                opyn::get_model_file_filters());
        }

        std::future<TabSaveResult> export_as_model_graph_as_osim_file()
        {
            auto promise = std::make_shared<std::promise<TabSaveResult>>();
            App::upd().prompt_user_to_save_file_with_extension_async(
                [promise, ptr = shared_from_this()](std::optional<std::filesystem::path> p) {
                    if (not p) {
                        promise->set_value(TabSaveResult::Cancelled);
                        return;  // user cancelled out of the prompt
                    }
                    try {
                        ptr->export_model_graph_to(*p);
                        promise->set_value(TabSaveResult::Done);
                    } catch (const std::exception&) {
                        promise->set_value(TabSaveResult::Cancelled);
                    }
                },
                "osim");
            return promise->get_future();
        }

        std::future<TabSaveResult> export_model_graph_as_osim_file()
        {
            if (maybe_model_graph_export_location_.empty()) {
                return export_as_model_graph_as_osim_file();
            } else {
                std::promise<TabSaveResult> promise;
                promise.set_value(
                    export_model_graph_to(maybe_model_graph_export_location_)
                        ? TabSaveResult::Done
                        : TabSaveResult::Cancelled);
                return promise.get_future();
            }
        }

        bool is_model_graph_up_to_date_with_disk() const
        {
            return maybe_model_graph_exported_uid_ == model_graph_snapshots_.head_id();
        }

        bool is_close_requested() const { return close_requested_; }

        void request_close() { close_requested_ = true; }

        void reset_request_close() { close_requested_ = false; }

        bool is_new_mesh_impoter_tab_requested() const { return new_tab_requested_; }

        void request_new_mesh_importer_tab() { new_tab_requested_ = true; }

        void reset_request_new_mesh_importer() { new_tab_requested_ = false; }

        std::string get_recommended_title() const
        {
            std::stringstream ss;
            ss << MSMICONS_CUBE << ' ' << get_document_name();
            return std::move(ss).str();
        }

        const MiDocument& get_model_graph() const { return model_graph_snapshots_.scratch(); }

        MiDocument& upd_model_graph() { return model_graph_snapshots_.upd_scratch(); }

        MiUndoableDocument& upd_committable_model_graph() { return model_graph_snapshots_; }

        void commit_current_model_graph(std::string_view commit_msg)
        {
            model_graph_snapshots_.commit_scratch(commit_msg);
        }

        bool can_undo_current_model_graph() const { return model_graph_snapshots_.can_undo(); }

        void undo_current_model_graph() { model_graph_snapshots_.undo(); }

        bool can_redo_current_model_graph() const { return model_graph_snapshots_.can_redo(); }

        void redo_current_model_graph() { model_graph_snapshots_.redo(); }

        const std::unordered_set<UID>& get_current_selection() const
        {
            return get_model_graph().get_selected();
        }

        void select_all() { upd_model_graph().select_all(); }

        void de_select_all() { upd_model_graph().de_select_all(); }

        bool has_selection() const { return get_model_graph().has_selection(); }

        bool is_selected(UID id) const { return get_model_graph().is_selected(id); }

        //
        // MESH LOADING STUFF
        //

        void push_mesh_load_requests(
            std::vector<std::filesystem::path> paths, UID attachment_point = MiIDs::ground())
        {
            mesh_loader_.send(MeshLoadRequest{attachment_point, std::move(paths)});
        }

        void prompt_user_for_mesh_files_and_push_them_onto_mesh_loader(
            UID attachment_point = MiIDs::ground())
        {
            if (not shared_from_this()) {
                log_critical(
                    "cannot open mesh import dialog because the mesh importer's state isn't reference-counted");
                return;
            }
            App::upd().prompt_user_to_select_file_async(
                [st = shared_from_this(), attachment_point](const FileDialogResponse&& response) {
                    if (not st) {
                        return;  // Something went wrong
                    }
                    std::vector<std::filesystem::path> paths(response.begin(), response.end());
                    st->push_mesh_load_requests(paths, attachment_point);
                },
                opyn::get_supported_sim_tk_mesh_formats_as_filters(),
                std::nullopt,
                true);
        }

        void reload_meshes()
        {
            for (auto& mesh : model_graph_snapshots_.upd_scratch().iter<MiMesh>()) {
                try {
                    mesh.reload_mesh_data_from_disk();
                } catch (const std::exception& ex) {
                    log_info("{}: error reloading: {}", mesh.get_label(), ex.what());
                }
            }
        }

        //
        // UI OVERLAY STUFF
        //

        Vector2 world_pos_to_ui_pos(const Vector3& world_pos) const
        {
            return get_camera().world_to_ui(world_pos, scene_ui_rect());
        }

        void draw_connection_line(
            const Color& color, const Vector3& parent, const Vector3& child) const
        {
            // the line
            ui::get_panel_draw_list().add_line(
                world_pos_to_ui_pos(parent),
                world_pos_to_ui_pos(child),
                color,
                c_connection_line_width);

            // the triangle
            draw_connection_line_triangle_at_midpoint(color, parent, child);
        }

        void draw_connection_lines(
            const Color& color, const std::unordered_set<UID>& excluded_ids) const
        {
            const MiDocument& mg = get_model_graph();

            for (const MiObject& el : mg.iter()) {
                UID id = el.get_id();

                if (excluded_ids.contains(id)) {
                    continue;
                }

                if (!should_show_connection_lines(el)) {
                    continue;
                }

                if (el.get_num_cross_references() > 0) {
                    draw_connection_lines(el, color, excluded_ids);
                } else if (not is_a_child_attachment_in_any_joint(mg, el)) {
                    draw_connection_line_to_ground(el, color);
                }
            }
        }

        void draw_connection_lines(const Color& color) const { draw_connection_lines(color, {}); }

        void draw_connection_lines(const MeshImporterHover& current_hover) const
        {
            const MiDocument& mg = get_model_graph();

            for (const MiObject& el : mg.iter()) {
                UID id = el.get_id();

                if (id != current_hover.id && !el.is_cross_referencing(current_hover.id)) {
                    continue;
                }

                if (not should_show_connection_lines(el)) {
                    continue;
                }

                if (el.get_num_cross_references() > 0) {
                    draw_connection_lines(el, colors_.connection_lines);
                } else if (not is_a_child_attachment_in_any_joint(mg, el)) {
                    draw_connection_line_to_ground(el, colors_.connection_lines);
                }
            }
        }

        //
        // RENDERING STUFF
        //

        void set_content_region_avail_as_scene_rect()
        {
            set_scene_ui_rect(ui::get_content_region_available_ui_rect());
        }

        void draw_scene(std::span<const DrawableThing> drawables)
        {
            const App& app = App::get();

            // setup rendering params
            SceneRendererParams p;
            p.dimensions = scene_dims();
            p.device_pixel_ratio = App::settings().get_value<float>("graphics/render_scale", 1.0f) *
                                   app.main_window_device_pixel_ratio();
            p.anti_aliasing_level = app.anti_aliasing_level();
            p.draw_rims = true;
            p.draw_floor = false;
            p.near_clipping_plane = camera_.near_clipping_plane();
            p.far_clipping_plane = camera_.far_clipping_plane();
            p.view_matrix = camera_.view_matrix();
            p.projection_matrix = camera_.projection_matrix(aspect_ratio_of(p.dimensions));
            p.viewer_position = camera_.position();
            p.light_direction = recommended_light_direction(camera_);
            p.light_color = Color::white();
            p.ambient_strength *= 1.5f;
            p.background_color = get_color_scene_background();

            std::vector<SceneDecoration> decs;
            decs.reserve(drawables.size());
            for (const DrawableThing& dt : drawables) {
                decs.push_back(
                    SceneDecoration{
                        .mesh = dt.mesh,
                        .transform = dt.transform,
                        .shading = dt.shading,
                        .flags = dt.flags,
                    });
            }

            // render
            scene_renderer_.render(decs, p);

            // draw texture in ui
            ui::draw_image(scene_renderer_.upd_render_texture());

            // handle hittesting, etc.
            set_is_render_hovered(ui::is_item_hovered(ui::HoveredFlag::AllowWhenBlockedByPopup));
        }

        bool is_render_hovered() const { return is_render_hovered_; }

        const Rect& scene_ui_rect() const { return scene_ui_rect_; }

        Vector2 scene_dims() const { return scene_ui_rect_.dimensions(); }

        const Camera& get_camera() const { return camera_; }

        Camera& upd_camera() { return camera_; }

        const OrbitCameraController& get_camera_controller() { return camera_controller_; }

        OrbitCameraController& upd_camera_controller() { return camera_controller_; }

        void reset_camera()
        {
            camera_controller_ = create_default_camera_controller();
            camera_controller_.update_camera(camera_);
        }

        void focus_camera_on(const Vector3& focus_point)
        {
            camera_controller_.focus_point = focus_point;
            camera_controller_.update_camera(camera_);
        }

        std::span<const Color> colors() const
        {
            static_assert(offsetof(Colors, ground) == 0);
            static_assert(sizeof(Colors) % sizeof(Color) == 0);
            return {&colors_.ground, sizeof(colors_) / sizeof(Color)};
        }

        void set_color(size_t i, const Color& new_color_value)
        {
            upd_colors()[i] = new_color_value;
        }

        std::span<const char* const> get_color_labels() const { return c_color_names; }

        const Color& get_color_connection_line() const { return colors_.connection_lines; }

        std::span<const bool> get_visibility_flags() const
        {
            static_assert(offsetof(VisibilityFlags, ground) == 0);
            static_assert(sizeof(VisibilityFlags) % sizeof(bool) == 0);
            return {&visibility_flags_.ground, sizeof(visibility_flags_) / sizeof(bool)};
        }

        void set_visibility_flag(size_t i, bool new_visibility_value)
        {
            upd_visibility_flags()[i] = new_visibility_value;
        }

        std::span<const char* const> get_visibility_flag_labels() const
        {
            return c_visibility_flag_names;
        }

        bool is_showing_floor() const { return visibility_flags_.floor; }

        DrawableThing generate_floor_drawable() const
        {
            auto props = MeshBasicMaterial::PropertyBlock{};
            props.set_color(colors_.grid_lines);

            const auto& sf = scene_scale_factor_;
            return DrawableThing{
                .id = MiIDs::empty(),
                .group_id = MiIDs::empty(),
                .mesh = App::singleton<SceneCache>(App::resource_loader())->grid_mesh(),
                .transform =
                    {
                        .scale = 0.5f * Vector3{sf * 100.0f, sf * 100.0f, 1.0f},
                        .rotation = angle_axis(Degrees{90}, Vector3{-1.0f, 0.0f, 0.0f}),
                    },
                .shading = std::pair<Material, MaterialPropertyBlock>{floor_material_, props},
                .flags = SceneDecorationFlag::AnnotationElement,
            };
        }

        //
        // HOVERTEST/INTERACTIVITY
        //

        std::span<const bool> get_ineractivity_flags() const
        {
            static_assert(offsetof(InteractivityFlags, ground) == 0);
            static_assert(sizeof(InteractivityFlags) % sizeof(bool) == 0);
            return {&interactivity_flags_.ground, sizeof(interactivity_flags_) / sizeof(bool)};
        }

        void set_interactivity_flag(size_t i, bool new_interactivity_value)
        {
            upd_interactivity_flags()[i] = new_interactivity_value;
        }

        std::span<const char* const> get_interactivity_flag_labels() const
        {
            return c_interactivity_flag_names;
        }

        float get_scene_scale_factor() const { return scene_scale_factor_; }

        void set_scene_scale_factor(float new_scale_factor)
        {
            scene_scale_factor_ = new_scale_factor;
        }

        MeshImporterHover do_hovertest(const std::vector<DrawableThing>& drawables) const
        {
            auto cache = App::singleton<SceneCache>(App::resource_loader());

            const Rect scene_rect = scene_ui_rect();
            const Vector2 mouse_ui_position = ui::get_mouse_ui_position();

            if (not is_intersecting(scene_rect, mouse_ui_position)) {
                // mouse isn't over the scene render
                return MeshImporterHover{};
            }

            const Ray ray = get_camera().ui_to_world(mouse_ui_position, scene_rect);
            const bool hittest_meshes = is_meshes_interactable();
            const bool hittest_bodies = is_bodies_interactable();
            const bool hittest_joint_centers = is_joint_centers_interactable();
            const bool hittest_ground = is_ground_interactable();
            const bool hittest_stations = is_stations_interactable();

            UID closest_id = MiIDs::empty();
            float closest_dist = std::numeric_limits<float>::max();
            for (const DrawableThing& drawable : drawables) {
                if (drawable.id == MiIDs::empty()) {
                    continue;  // no hittest data
                }

                if (drawable.group_id == MiIDs::body_group() && !hittest_bodies) {
                    continue;
                }

                if (drawable.group_id == MiIDs::mesh_group() && !hittest_meshes) {
                    continue;
                }

                if (drawable.group_id == MiIDs::joint_group() && !hittest_joint_centers) {
                    continue;
                }

                if (drawable.group_id == MiIDs::ground_group() && !hittest_ground) {
                    continue;
                }

                if (drawable.group_id == MiIDs::station_group() && !hittest_stations) {
                    continue;
                }

                const std::optional<RayCollision> rc =
                    get_closest_world_space_ray_triangle_collision(
                        drawable.mesh, cache->get_bvh(drawable.mesh), drawable.transform, ray);

                if (rc && rc->distance < closest_dist) {
                    closest_id = drawable.id;
                    closest_dist = rc->distance;
                }
            }

            const Vector3 hit_pos = closest_id != MiIDs::empty()
                                        ? ray.origin + closest_dist * ray.direction
                                        : Vector3{};

            return MeshImporterHover{closest_id, hit_pos};
        }

        //
        // MODEL CREATION FLAGS
        //

        ModelCreationFlags get_model_creation_flags() const { return model_creation_flags_; }

        void set_model_creation_flags(ModelCreationFlags new_flags)
        {
            model_creation_flags_ = new_flags;
        }

        //
        // SCENE ELEMENT STUFF (specific methods for specific scene element types)
        //

        DrawableThing generate_mesh_drawable(const MiMesh& el) const
        {
            return DrawableThing{
                .id = el.get_id(),
                .group_id = MiIDs::mesh_group(),
                .mesh = el.get_mesh_data(),
                .transform = el.get_x_form(),
                .shading =
                    el.get_parent_id() == MiIDs::ground() or el.get_parent_id() == MiIDs::empty()
                        ? redify_color(get_color_mesh())
                        : get_color_mesh(),
            };
        }

        void append_drawables(const MiObject& e, std::vector<DrawableThing>& append_out) const
        {
            std::visit(
                Overload{
                    [this, &append_out](const MiGround&) {
                        if (!is_showing_ground()) {
                            return;
                        }

                        append_out.push_back(generate_ground_sphere(get_color_ground()));
                    },
                    [this, &append_out](const MiMesh& el) {
                        if (!is_showing_meshes()) {
                            return;
                        }

                        append_out.push_back(generate_mesh_drawable(el));
                    },
                    [this, &append_out](const MiBody& el) {
                        if (!is_showing_bodies()) {
                            return;
                        }

                        append_body_el_as_cube_thing(el, append_out);
                    },
                    [this, &append_out](const MiJoint& el) {
                        if (!is_showing_joint_centers()) {
                            return;
                        }

                        append_as_frame(
                            el.get_id(),
                            MiIDs::joint_group(),
                            el.get_x_form(),
                            append_out,
                            1.0f,
                            SceneDecorationFlag::Default,
                            get_joint_axis_lengths(el));
                    },
                    [this, &append_out](const MiStation& el) {
                        if (!is_showing_stations()) {
                            return;
                        }

                        append_out.push_back(generate_station_sphere(el, get_color_station()));
                    },
                },
                e.to_variant());
        }

        //
        // WINDOWS
        //
        enum class PanelIndex {
            History = 0,
            Navigator,
            Log,
            Performance,
            NUM_OPTIONS,
        };

        size_t num_toggleable_panels() const { return num_options_v<PanelIndex>; }

        CStringView get_nth_panel_name(size_t n) const { return c_opened_panel_names[n]; }

        bool is_nth_panel_enabled(size_t n) const { return panel_states_[n]; }

        void set_nth_panel_enabled(size_t n, bool v) { panel_states_[n] = v; }

        bool is_panel_enabled(PanelIndex idx) const { return panel_states_[to_index(idx)]; }

        void set_panel_enabled(PanelIndex idx, bool v) { panel_states_[to_index(idx)] = v; }

        LogViewer& upd_log_viewer() { return log_viewer_; }

        PerfPanel& upd_perf_panel() { return perf_panel_; }

        //
        // TOP-LEVEL STUFF
        //

        bool on_event(Event&);

        void tick(float)
        {
            // push any user-drag-dropped files as one batch
            if (not dropped_files_.empty()) {
                std::vector<std::filesystem::path> buf;
                std::swap(buf, dropped_files_);
                push_mesh_load_requests(std::move(buf));
            }

            // pop any background-loaded meshes
            pop_mesh_loader();

            model_graph_snapshots_.upd_scratch().garbage_collect();
        }

    private:
        bool export_model_graph_to(const std::filesystem::path& export_path)
        {
            std::vector<std::string> issues;
            std::unique_ptr<OpenSim::Model> m;

            try {
                m = create_open_sim_model_from_mesh_importer_document(
                    get_model_graph(), model_creation_flags_, issues);
            } catch (const std::exception& ex) {
                log_error(
                    "error occurred while trying to create an OpenSim model from the mesh editor scene: {}",
                    ex.what());
            }

            if (m) {
                m->print(export_path.string());
                maybe_model_graph_export_location_ = export_path;
                maybe_model_graph_exported_uid_ = model_graph_snapshots_.head_id();
                return true;
            } else {
                for (const std::string& issue : issues) {
                    log_error(issue);
                }
                return false;
            }
        }

        std::string get_document_name() const
        {
            if (maybe_model_graph_export_location_.empty()) {
                return "untitled.osim";
            } else {
                return maybe_model_graph_export_location_.filename().string();
            }
        }

        // called when the mesh loader responds with a fully-loaded mesh
        void pop_mesh_loader_handle_ok_response(MeshLoadOKResponse& ok)
        {
            if (ok.meshes.empty()) {
                return;
            }

            // add each loaded mesh into the model graph
            MiDocument& mg = upd_model_graph();
            mg.de_select_all();

            for (const LoadedMesh& lm : ok.meshes) {
                const MiObject* el = mg.try_upd_by_id(ok.preferred_attachment_point);

                if (el) {
                    auto& mesh = mg.emplace<MiMesh>(
                        UID{}, ok.preferred_attachment_point, lm.mesh_data, lm.path);
                    mesh.set_xform(el->get_x_form(mg));
                    mg.select(mesh);
                    mg.select(*el);
                }
            }

            // commit
            {
                std::stringstream commit_msg_ss;
                if (ok.meshes.empty()) {
                    commit_msg_ss << "loaded 0 meshes";
                } else if (ok.meshes.size() == 1) {
                    commit_msg_ss << "loaded " << ok.meshes.front().path.filename();
                } else {
                    commit_msg_ss << "loaded " << ok.meshes.size() << " meshes";
                }

                commit_current_model_graph(std::move(commit_msg_ss).str());
            }
        }

        // called when the mesh loader responds with a mesh loading error
        void pop_mesh_loader_handle_error_response(MeshLoadErrorResponse& err)
        {
            log_error("{}: error loading mesh file: {}", err.path.string(), err.error);
        }

        void pop_mesh_loader()
        {
            for (auto maybe_response = mesh_loader_.poll(); maybe_response.has_value();
                 maybe_response = mesh_loader_.poll()) {

                MeshLoadResponse& mesh_loader_resp = *maybe_response;

                if (std::holds_alternative<MeshLoadOKResponse>(mesh_loader_resp)) {
                    pop_mesh_loader_handle_ok_response(
                        std::get<MeshLoadOKResponse>(mesh_loader_resp));
                } else {
                    pop_mesh_loader_handle_error_response(
                        std::get<MeshLoadErrorResponse>(mesh_loader_resp));
                }
            }
        }

        void draw_connection_line_triangle_at_midpoint(
            const Color& color, const Vector3& parent, const Vector3& child) const
        {
            constexpr float triangle_width = 6.0f * c_connection_line_width;
            constexpr float triangle_width_squared = triangle_width * triangle_width;

            const Vector2 parent_scr = world_pos_to_ui_pos(parent);
            const Vector2 child_scr = world_pos_to_ui_pos(child);
            const Vector2 child2_parent_scr = parent_scr - child_scr;

            if (dot(child2_parent_scr, child2_parent_scr) < triangle_width_squared) {
                return;
            }

            const Vector3 mp = midpoint(parent, child);
            const Vector2 midpoint_scr = world_pos_to_ui_pos(mp);
            const Vector2 direction_scr = normalize(child2_parent_scr);
            const Vector2 direction_normal_scr = {-direction_scr.y(), direction_scr.x()};

            const Vector2 p1 = midpoint_scr + (triangle_width / 2.0f) * direction_normal_scr;
            const Vector2 p2 = midpoint_scr - (triangle_width / 2.0f) * direction_normal_scr;
            const Vector2 p3 = midpoint_scr + triangle_width * direction_scr;

            ui::get_panel_draw_list().add_triangle_filled(p1, p2, p3, color);
        }

        void draw_connection_lines(
            const MiObject& el,
            const Color& color,
            const std::unordered_set<UID>& excluded_ids) const
        {
            const MiDocument& mg = get_model_graph();
            for (int i = 0, len = el.get_num_cross_references(); i < len; ++i) {
                UID ref_id = el.get_cross_reference_connectee_id(i);

                if (excluded_ids.contains(ref_id)) {
                    continue;
                }

                const MiObject* other = mg.try_get_by_id(ref_id);

                if (!other) {
                    continue;
                }

                Vector3 child = el.get_pos(mg);
                Vector3 parent = other->get_pos(mg);

                if (el.get_cross_reference_direction(i) == MiCrossrefDirection::ToChild) {
                    std::swap(parent, child);
                }

                draw_connection_line(color, parent, child);
            }
        }

        void draw_connection_lines(const MiObject& el, const Color& color) const
        {
            draw_connection_lines(el, color, std::unordered_set<UID>{});
        }

        void draw_connection_line_to_ground(const MiObject& el, const Color& color) const
        {
            if (el.get_id() == MiIDs::ground()) {
                return;
            }

            draw_connection_line(color, Vector3{}, el.get_pos(get_model_graph()));
        }

        bool should_show_connection_lines(const MiObject& el) const
        {
            return std::visit(
                Overload{
                    [](const MiGround&) { return false; },
                    [this](const MiMesh&) { return this->is_showing_mesh_connection_lines(); },
                    [this](const MiBody&) { return this->is_showing_body_connection_lines(); },
                    [this](const MiJoint&) { return this->is_showing_joint_connection_lines(); },
                    [this](const MiStation&) { return this->is_showing_mesh_connection_lines(); },
                },
                el.to_variant());
        }

        void set_is_render_hovered(bool new_is_hovered) { is_render_hovered_ = new_is_hovered; }

        void set_scene_ui_rect(const Rect& new_rect) { scene_ui_rect_ = new_rect; }

        std::span<Color> upd_colors()
        {
            static_assert(offsetof(Colors, ground) == 0);
            static_assert(sizeof(Colors) % sizeof(Color) == 0);
            return {&colors_.ground, sizeof(colors_) / sizeof(Color)};
        }

        const Color& get_color_scene_background() const { return colors_.scene_background; }

        const Color& get_color_ground() const { return colors_.ground; }

        const Color& get_color_mesh() const { return colors_.meshes; }

        const Color& get_color_station() const { return colors_.stations; }

        std::span<bool> upd_visibility_flags()
        {
            static_assert(offsetof(VisibilityFlags, ground) == 0);
            static_assert(sizeof(VisibilityFlags) % sizeof(bool) == 0);
            return {&visibility_flags_.ground, sizeof(visibility_flags_) / sizeof(bool)};
        }

        bool is_showing_meshes() const { return visibility_flags_.meshes; }

        void set_is_showing_meshes(bool new_is_showing)
        {
            visibility_flags_.meshes = new_is_showing;
        }

        bool is_showing_bodies() const { return visibility_flags_.bodies; }

        void set_is_showing_bodies(bool new_is_showing)
        {
            visibility_flags_.bodies = new_is_showing;
        }

        bool is_showing_joint_centers() const { return visibility_flags_.joints; }

        void set_is_showing_joint_centers(bool new_is_showing)
        {
            visibility_flags_.joints = new_is_showing;
        }

        bool is_showing_ground() const { return visibility_flags_.ground; }

        void set_is_showing_ground(bool new_is_showing)
        {
            visibility_flags_.ground = new_is_showing;
        }

        void set_is_showing_floor(bool new_is_showing) { visibility_flags_.floor = new_is_showing; }

        bool is_showing_stations() const { return visibility_flags_.stations; }

        void set_is_showing_stations(bool v) { visibility_flags_.stations = v; }

        bool is_showing_joint_connection_lines() const
        {
            return visibility_flags_.joint_connection_lines;
        }

        void set_is_showing_joint_connection_lines(bool new_is_showing)
        {
            visibility_flags_.joint_connection_lines = new_is_showing;
        }

        bool is_showing_mesh_connection_lines() const
        {
            return visibility_flags_.mesh_connection_lines;
        }

        void set_is_showing_mesh_connection_lines(bool new_is_showing)
        {
            visibility_flags_.mesh_connection_lines = new_is_showing;
        }

        bool is_showing_body_connection_lines() const
        {
            return visibility_flags_.body_to_ground_connection_lines;
        }

        void set_is_showing_body_connection_lines(bool new_is_showing)
        {
            visibility_flags_.body_to_ground_connection_lines = new_is_showing;
        }

        bool is_showing_station_connection_lines() const
        {
            return visibility_flags_.station_connection_lines;
        }

        void set_is_showing_station_connection_lines(bool new_is_showing)
        {
            visibility_flags_.station_connection_lines = new_is_showing;
        }

        float get_sphere_radius() const { return 0.02f * scene_scale_factor_; }

        Sphere sphere_at_translation(const Vector3& translation) const
        {
            return Sphere{translation, get_sphere_radius()};
        }

        void append_as_frame(
            UID logical_id,
            UID group_id,
            const Transform& xform,
            std::vector<DrawableThing>& append_out,
            float alpha = 1.0f,
            SceneDecorationFlags flags = SceneDecorationFlag::Default,
            Vector3 leg_len = {1.0f, 1.0f, 1.0f},
            Color core_color = Color::white()) const
        {
            const float core_radius = get_sphere_radius();
            const float leg_thickness = 0.5f * core_radius;

            // this is how much the cylinder has to be "pulled in" to the core to hide the edges
            const float cylinder_pullback =
                core_radius * sin((Degrees{180} * leg_thickness) / core_radius);

            // emit origin sphere
            append_out.push_back(
                DrawableThing{
                    .id = logical_id,
                    .group_id = group_id,
                    .mesh = sphere_mesh_,
                    .transform =
                        {
                            .scale = Vector3{core_radius},
                            .rotation = xform.rotation,
                            .translation = xform.translation,
                        },
                    .shading = core_color.with_alpha(core_color.a * alpha),
                    .flags = flags,
                });

            // emit "legs"
            for (int i = 0; i < 3; ++i) {
                // cylinder meshes are -1.0f to 1.0f in Y, so create a transform that maps the
                // mesh onto the legs, which are:
                //
                // - 4.0f * leglen[leg] * radius long
                // - 0.5f * radius thick

                const Vector3 mesh_direction = {0.0f, 1.0f, 0.0f};
                Vector3 cylinder_direction = {};
                cylinder_direction[i] = 1.0f;

                const float actual_leg_len = 4.0f * leg_len[i] * core_radius;

                const Quaternion rot =
                    normalize(xform.rotation * rotation(mesh_direction, cylinder_direction));
                const Vector3 translation =
                    xform.translation +
                    (rot * (((get_sphere_radius() + (0.5f * actual_leg_len)) - cylinder_pullback) *
                            mesh_direction));
                append_out.push_back(
                    DrawableThing{
                        .id = logical_id,
                        .group_id = group_id,
                        .mesh = cylinder_mesh_,
                        .transform =
                            {
                                // note: cylinder is 2 units high
                                .scale = {leg_thickness, 0.5f * actual_leg_len, leg_thickness},
                                .rotation = rot,
                                .translation = translation,
                            },
                        .shading = Color{0.0f, alpha}.with_element(i, 1.0f),
                        .flags = flags,
                    });
            }
        }

        void append_as_cube_thing(
            UID logical_id,
            UID group_id,
            const Transform& xform,
            std::vector<DrawableThing>& append_out) const
        {
            const float half_width = 1.5f * get_sphere_radius();

            // core
            {
                Transform scaled{xform};
                scaled.scale *= half_width;

                append_out.push_back(
                    DrawableThing{
                        .id = logical_id,
                        .group_id = group_id,
                        .mesh = App::singleton<SceneCache>(App::resource_loader())->brick_mesh(),
                        .transform = scaled,
                        .shading = Color::white(),
                    });
            }

            // legs
            for (int i = 0; i < 3; ++i) {
                // cone mesh has a source height of 2, stretches from -1 to +1 in Y
                const float cone_height = 0.75f * half_width;

                const Vector3 mesh_direction = {0.0f, 1.0f, 0.0f};
                const Vector3 cone_direction = Vector3{}.with_element(i, 1.0f);

                const Quaternion rot = xform.rotation * rotation(mesh_direction, cone_direction);
                const Vector3 translation =
                    xform.translation +
                    (rot * ((half_width + (0.5f * cone_height)) * mesh_direction));
                append_out.push_back(
                    DrawableThing{
                        .id = logical_id,
                        .group_id = group_id,
                        .mesh = App::singleton<SceneCache>(App::resource_loader())->cone_mesh(),
                        .transform =
                            {
                                .scale = 0.5f * Vector3{half_width, cone_height, half_width},
                                .rotation = rot,
                                .translation = translation,
                            },
                        .shading = Color::black().with_element(i, 1.0f),
                    });
            }
        }

        std::span<bool> upd_interactivity_flags()
        {
            static_assert(offsetof(InteractivityFlags, ground) == 0);
            static_assert(sizeof(InteractivityFlags) % sizeof(bool) == 0);
            return {&interactivity_flags_.ground, sizeof(interactivity_flags_) / sizeof(bool)};
        }

        bool is_meshes_interactable() const { return interactivity_flags_.meshes; }

        void set_is_meshes_interactable(bool new_is_interactable)
        {
            interactivity_flags_.meshes = new_is_interactable;
        }

        bool is_bodies_interactable() const { return interactivity_flags_.bodies; }

        void set_is_bodies_interactable(bool new_is_interactable)
        {
            interactivity_flags_.bodies = new_is_interactable;
        }

        bool is_joint_centers_interactable() const { return interactivity_flags_.joints; }

        void set_is_joint_centers_interactable(bool new_is_interactable)
        {
            interactivity_flags_.joints = new_is_interactable;
        }

        bool is_ground_interactable() const { return interactivity_flags_.ground; }

        void set_is_ground_interactable(bool new_is_interactable)
        {
            interactivity_flags_.ground = new_is_interactable;
        }

        bool is_stations_interactable() const { return interactivity_flags_.stations; }

        void set_is_stations_interactable(bool v) { interactivity_flags_.stations = v; }

        void append_body_el_as_cube_thing(
            const MiBody& body_el, std::vector<DrawableThing>& append_out) const
        {
            append_as_cube_thing(
                body_el.get_id(), MiIDs::body_group(), body_el.get_x_form(), append_out);
        }

        DrawableThing generate_body_el_sphere(const MiBody& body_el, const Color& color) const
        {
            return {
                .id = body_el.get_id(),
                .group_id = MiIDs::body_group(),
                .mesh = sphere_mesh_,
                .transform = sphere_mesh_to_scene_sphere_transform(
                    sphere_at_translation(body_el.get_x_form().translation)),
                .shading = color,
            };
        }

        DrawableThing generate_ground_sphere(const Color& color) const
        {
            return {
                .id = MiIDs::ground(),
                .group_id = MiIDs::ground_group(),
                .mesh = sphere_mesh_,
                .transform = sphere_mesh_to_scene_sphere_transform(
                    sphere_at_translation({0.0f, 0.0f, 0.0f})),
                .shading = color,
            };
        }

        DrawableThing generate_station_sphere(const MiStation& el, const Color& color) const
        {
            return {
                .id = el.get_id(),
                .group_id = MiIDs::station_group(),
                .mesh = sphere_mesh_,
                .transform = sphere_mesh_to_scene_sphere_transform(
                    sphere_at_translation(el.get_pos(get_model_graph()))),
                .shading = color,
            };
        }

        Color redify_color(const Color& src_color) const
        {
            constexpr float factor = 0.8f;
            return {src_color.r, factor * src_color.g, factor * src_color.b, factor * src_color.a};
        }

        // returns a transform that maps a sphere mesh (defined to be @ 0,0,0 with radius 1)
        // to some sphere in the scene (e.g. a body/ground)
        Transform sphere_mesh_to_scene_sphere_transform(const Sphere& scene_sphere) const
        {
            return {
                .scale = Vector3{scene_sphere.radius},
                .translation = scene_sphere.origin,
            };
        }

        // returns a camera controller that is in the initial position the camera
        // should be in for the mesh importer ui
        OrbitCameraController create_default_camera_controller() const
        {
            OrbitCameraController rv;
            rv.phi = Degrees{45};
            rv.theta = Degrees{45};
            rv.radius = 2.5f;
            return rv;
        }

        // in-memory model graph (snapshots) that the user is manipulating
        MiUndoableDocument model_graph_snapshots_;

        // (maybe) the filesystem location where the model graph should be saved
        std::filesystem::path maybe_model_graph_export_location_;

        // (maybe) the UID of the model graph when it was last successfully saved to disk (used for
        // dirty checking)
        UID maybe_model_graph_exported_uid_ = model_graph_snapshots_.head_id();

        // a batch of files that the user drag-dropped into the UI in the last frame
        std::vector<std::filesystem::path> dropped_files_;

        // loads meshes in a background thread
        MeshLoader mesh_loader_;

        // sphere mesh used by various scene elements
        Mesh sphere_mesh_ = SphereGeometry{{.num_width_segments = 12, .num_height_segments = 12}};

        // cylinder mesh used by various scene elements
        Mesh cylinder_mesh_ = CylinderGeometry{{.height = 2.0f, .num_radial_segments = 32}};

        // cone mesh used to render scene elements
        Mesh cone_mesh_ = ConeGeometry{{.radius = 1.0f, .height = 2.0f, .num_radial_segments = 16}};

        // material used to draw the floor grid
        MeshBasicMaterial floor_material_;

        // main 3D scene camera controller
        OrbitCameraController camera_controller_ = create_default_camera_controller();

        // main 3D scene camera
        Camera camera_ = [this] {
            Camera rv;
            rv.set_vertical_field_of_view(Degrees{35.0f});
            camera_controller_.update_camera(rv);
            return rv;
        }();

        // screen space rect where the 3D scene is currently being drawn to
        Rect scene_ui_rect_;

        // renderer that draws the scene
        SceneRenderer scene_renderer_{
            *App::singleton<SceneCache>(App::resource_loader()),
        };

        // COLORS
        //
        // these are runtime-editable color values for things in the scene
        struct Colors {
            Color ground{196.0f / 255.0f, 196.0f / 255.0f, 196.0f / 255.0f, 1.0f};
            Color meshes{1.0f, 1.0f, 1.0f, 1.0f};
            Color stations{196.0f / 255.0f, 0.0f, 0.0f, 1.0f};
            Color connection_lines{0.6f, 0.6f, 0.6f, 1.0f};
            Color scene_background{48.0f / 255.0f, 48.0f / 255.0f, 48.0f / 255.0f, 1.0f};
            Color grid_lines{0.7f, 0.7f, 0.7f, 0.15f};
        } colors_;
        static constexpr auto c_color_names = std::to_array<const char*>({
            "ground",
            "meshes",
            "stations",
            "connection lines",
            "scene background",
            "grid lines",
        });
        static_assert(c_color_names.size() == sizeof(decltype(colors_)) / sizeof(Color));

        // VISIBILITY
        //
        // these are runtime-editable visibility flags for things in the scene
        struct VisibilityFlags {
            bool ground = true;
            bool meshes = true;
            bool bodies = true;
            bool joints = true;
            bool stations = true;
            bool joint_connection_lines = true;
            bool mesh_connection_lines = true;
            bool body_to_ground_connection_lines = true;
            bool station_connection_lines = true;
            bool floor = true;
        } visibility_flags_;
        static constexpr auto c_visibility_flag_names = std::to_array<const char*>({
            "ground",
            "meshes",
            "bodies",
            "joints",
            "stations",
            "joint connection lines",
            "mesh connection lines",
            "body-to-ground connection lines",
            "station connection lines",
            "grid lines",
        });
        static_assert(
            c_visibility_flag_names.size() == sizeof(decltype(visibility_flags_)) / sizeof(bool));

        // LOCKING
        //
        // these are runtime-editable flags that dictate what gets hit-tested
        struct InteractivityFlags {
            bool ground = true;
            bool meshes = true;
            bool bodies = true;
            bool joints = true;
            bool stations = true;
        } interactivity_flags_;
        static constexpr auto c_interactivity_flag_names = std::to_array<const char*>({
            "ground",
            "meshes",
            "bodies",
            "joints",
            "stations",
        });
        static_assert(
            c_interactivity_flag_names.size() ==
            sizeof(decltype(interactivity_flags_)) / sizeof(bool));

        // WINDOWS
        //
        // these are runtime-editable flags that dictate which panels are open
        static constexpr size_t c_num_panel_states = 4;
        std::array<bool, c_num_panel_states> panel_states_{false, true, false, false};
        static constexpr auto c_opened_panel_names = std::to_array<const char*>({
            "History",
            "Navigator",
            "Log",
            "Performance",
        });
        static_assert(c_opened_panel_names.size() == c_num_panel_states);
        static_assert(num_options_v<PanelIndex> == c_num_panel_states);
        LogViewer log_viewer_;
        PerfPanel perf_panel_;

        // scale factor for all non-mesh, non-overlay scene elements (e.g.
        // the floor, bodies)
        //
        // this is necessary because some meshes can be extremely small/large and
        // scene elements need to be scaled accordingly (e.g. without this, a body
        // sphere end up being much larger than a mesh instance). Imagine if the
        // mesh was the leg of a fly
        float scene_scale_factor_ = 1.0f;

        // buffer containing issues found in the modelgraph
        std::vector<std::string> issues_buffer_;

        // model created by this wizard
        //
        // `nullptr` until the model is successfully created
        std::unique_ptr<OpenSim::Model> maybe_output_model_ = nullptr;

        // set to true after drawing the ui::Image
        bool is_render_hovered_ = false;

        // true if the implementation wants the host to close the mesh importer UI
        bool close_requested_ = false;

        // true if the implementation wants the host to open a new mesh importer
        bool new_tab_requested_ = false;

        // changes how a model is created
        ModelCreationFlags model_creation_flags_ = ModelCreationFlags::None;

        static constexpr float c_connection_line_width = 1.0f;
    };
}
