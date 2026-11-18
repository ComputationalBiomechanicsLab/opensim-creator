#include "mesh_importer_tab.h"

#include <libopensimcreator/documents/mesh_importer/mi_body.h>
#include <libopensimcreator/documents/mesh_importer/mi_document.h>
#include <libopensimcreator/documents/mesh_importer/mi_ground.h>
#include <libopensimcreator/documents/mesh_importer/mi_joint.h>
#include <libopensimcreator/documents/mesh_importer/mi_mesh.h>
#include <libopensimcreator/documents/mesh_importer/mi_class.h>
#include <libopensimcreator/documents/mesh_importer/mi_ids.h>
#include <libopensimcreator/documents/mesh_importer/mi_object.h>
#include <libopensimcreator/documents/mesh_importer/mi_object_helpers.h>
#include <libopensimcreator/documents/mesh_importer/mi_strings.h>
#include <libopensimcreator/documents/mesh_importer/mi_station.h>
#include <libopensimcreator/documents/mesh_importer/mi_undoable_actions.h>
#include <libopensimcreator/documents/mesh_importer/mi_undoable_document.h>
#include <libopensimcreator/platform/msmicons.h>
#include <libopensimcreator/platform/open_sim_creator_colors.h>
#include <libopensimcreator/ui/mesh_importer/choose_el_layer.h>
#include <libopensimcreator/ui/mesh_importer/drawable_thing.h>
#include <libopensimcreator/ui/mesh_importer/mesh_importer_ui_layer_host.h>
#include <libopensimcreator/ui/mesh_importer/mesh_importer_hover.h>
#include <libopensimcreator/ui/mesh_importer/mesh_importer_shared_state.h>
#include <libopensimcreator/ui/mesh_importer/mesh_importer_ui_layer.h>
#include <libopensimcreator/ui/mesh_importer/select_2_mesh_points_layer.h>
#include <libopensimcreator/ui/model_editor/model_editor_tab.h>
#include <libopensimcreator/ui/shared/import_stations_from_csv_popup.h>
#include <libopensimcreator/ui/shared/main_menu.h>

#include <libopynsim/component_registry/component_registry.h>
#include <libopynsim/component_registry/static_component_registries.h>
#include <liboscar/formats/obj.h>
#include <liboscar/formats/stl.h>
#include <liboscar/graphics/color.h>
#include <liboscar/graphics/mesh.h>
#include <liboscar/maths/aabb.h>
#include <liboscar/maths/math_helpers.h>
#include <liboscar/maths/matrix4x4.h>
#include <liboscar/maths/matrix_functions.h>
#include <liboscar/maths/quaternion.h>
#include <liboscar/maths/rect.h>
#include <liboscar/maths/rect_functions.h>
#include <liboscar/maths/transform.h>
#include <liboscar/maths/transform_functions.h>
#include <liboscar/maths/vector.h>
#include <liboscar/maths/vector_functions.h>
#include <liboscar/platform/app.h>
#include <liboscar/ui/events/close_tab_event.h>
#include <liboscar/ui/events/open_tab_event.h>
#include <liboscar/ui/oscimgui.h>
#include <liboscar/ui/panels/perf_panel.h>
#include <liboscar/ui/panels/undo_redo_panel.h>
#include <liboscar/ui/popups/popup_manager.h>
#include <liboscar/ui/tabs/tab_private.h>
#include <liboscar/ui/widgets/camera_view_axes.h>
#include <liboscar/ui/widgets/log_viewer.h>
#include <liboscar/utilities/assertions.h>
#include <liboscar/utilities/c_string_view.h>
#include <liboscar/utilities/scope_exit.h>
#include <liboscar/utilities/uid.h>

#include <cstddef>
#include <filesystem>
#include <fstream>
#include <future>
#include <memory>
#include <optional>
#include <span>
#include <sstream>
#include <string_view>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

using namespace osc;
using namespace osc::literals;

// mesh importer tab implementation
class osc::MeshImporterTab::Impl final : public TabPrivate, public MeshImporterUILayerHost {
public:
    explicit Impl(MeshImporterTab& owner, Widget* parent) :

        TabPrivate{owner, parent, "MeshImporterTab"},
        shared_{std::make_shared<MeshImporterSharedState>(&owner)}
    {}

    explicit Impl(
        MeshImporterTab& owner, Widget* parent, std::vector<std::filesystem::path> mesh_paths) :

        TabPrivate{owner, parent, "MeshImporterTab"},
        shared_{std::make_shared<MeshImporterSharedState>(&owner, std::move(mesh_paths))}
    {}

    bool is_unsaved() const { return !shared_->is_model_graph_up_to_date_with_disk(); }

    std::future<TabSaveResult> try_save()
    {
        if (shared_->is_model_graph_up_to_date_with_disk()) {
            std::promise<TabSaveResult> promise;
            promise.set_value(TabSaveResult::Done);
            return promise.get_future();
        } else {
            return shared_->export_model_graph_as_osim_file();
        }
    }

    void on_mount()
    {
        App::upd().make_main_loop_waiting();
        popup_manager_.on_mount();
    }

    void on_unmount() { App::upd().make_main_loop_polling(); }

    bool on_event(Event& e)
    {
        if (shared_->on_event(e)) {
            return true;
        }

        if (maybe_3d_viewer_modal_) {
            // ensure it stays alive - even if it pops itself during the drawcall
            std::shared_ptr<MeshImporterUILayer> ptr = maybe_3d_viewer_modal_;

            if (ptr->on_event(e)) {
                return true;
            }
        }

        return false;
    }

    void on_tick()
    {
        const auto dt = static_cast<float>(App::get().frame_delta_since_last_frame().count());

        shared_->tick(dt);

        if (maybe_3d_viewer_modal_) {
            // ensure it stays alive - even if it pops itself during the drawcall
            std::shared_ptr<MeshImporterUILayer> ptr = maybe_3d_viewer_modal_;
            ptr->tick(dt);
        }

        // if some screen generated an OpenSim::Model, transition to the main editor
        if (shared_->has_output_model()) {
            auto tab = std::make_unique<ModelEditorTab>(
                parent(),
                std::move(shared_->upd_output_model()),
                shared_->get_scene_scale_factor());
            App::post_event<OpenTabEvent>(owner(), std::move(tab));
        }

        set_name(shared_->get_recommended_title());

        if (shared_->is_close_requested()) {
            App::post_event<CloseTabEvent>(*parent(), id());
            shared_->reset_request_close();
        }

        if (shared_->is_new_mesh_impoter_tab_requested()) {
            App::post_event<OpenTabEvent>(*parent(), std::make_unique<MeshImporterTab>(parent()));
            shared_->reset_request_new_mesh_importer();
        }
    }

    void draw_main_menu()
    {
        draw_main_menu_file_menu();
        draw_main_menu_edit_menu();
        draw_main_menu_window_menu();
        draw_main_menu_about_menu();
    }

    void on_draw()
    {
        // enable panel docking
        ui::enable_dockspace_over_main_window();

        // handle keyboards using ImGui's input poller
        if (not maybe_3d_viewer_modal_) {
            update_from_im_gui_keyboard_state();
        }

        if (!maybe_3d_viewer_modal_ && shared_->is_render_hovered() && !gizmo_.is_using()) {
            const bool controller_updated = ui::update_orbit_controller_from_mouse_inputs(
                shared_->upd_camera_controller(), shared_->get_camera(), shared_->scene_dims());
            if (controller_updated) {
                shared_->get_camera_controller().update_camera(shared_->upd_camera());
            }
        }

        // draw history panel (if enabled)
        if (shared_->is_panel_enabled(MeshImporterSharedState::PanelIndex::History)) {
            bool v = true;
            if (ui::begin_panel("history", &v)) {
                draw_history_panel_content();
            }
            ui::end_panel();

            shared_->set_panel_enabled(MeshImporterSharedState::PanelIndex::History, v);
        }

        // draw navigator panel (if enabled)
        if (shared_->is_panel_enabled(MeshImporterSharedState::PanelIndex::Navigator)) {
            bool v = true;
            if (ui::begin_panel("navigator", &v)) {
                draw_navigator_panel_content();
            }
            ui::end_panel();

            shared_->set_panel_enabled(MeshImporterSharedState::PanelIndex::Navigator, v);
        }

        // draw log panel (if enabled)
        if (shared_->is_panel_enabled(MeshImporterSharedState::PanelIndex::Log)) {
            bool v = true;
            if (ui::begin_panel("Log", &v, ui::PanelFlag::MenuBar)) {
                shared_->upd_log_viewer().on_draw();
            }
            ui::end_panel();

            shared_->set_panel_enabled(MeshImporterSharedState::PanelIndex::Log, v);
        }

        // draw performance panel (if enabled)
        if (shared_->is_panel_enabled(MeshImporterSharedState::PanelIndex::Performance)) {
            PerfPanel& pp = shared_->upd_perf_panel();

            pp.open();
            pp.on_draw();
            if (!pp.is_open()) {
                shared_->set_panel_enabled(MeshImporterSharedState::PanelIndex::Performance, false);
            }
        }

        // draw contextual 3D modal (if there is one), else: draw standard 3D viewer
        draw_main_viewer_panel_or_modal();

        // draw any active popups over the scene
        popup_manager_.on_draw();
    }

private:
    //
    // ACTIONS
    //

    // pop the current UI layer
    void impl_request_pop(MeshImporterUILayer&) override
    {
        maybe_3d_viewer_modal_.reset();
        App::upd().request_redraw();
    }

    // try to select *only* what is currently hovered
    void select_just_hover()
    {
        if (not maybe_hover_) {
            return;
        }

        shared_->upd_model_graph().select(maybe_hover_.id);
    }

    // try to select what is currently hovered *and* anything that is "grouped"
    // with the hovered item
    //
    // "grouped" here specifically means other meshes connected to the same body
    void select_anything_grouped_with_hover()
    {
        if (not maybe_hover_) {
            return;
        }

        select_anything_grouped_with(shared_->upd_model_graph(), maybe_hover_.id);
    }

    // add a body element to whatever's currently hovered at the hover (raycast) position
    void try_add_body_to_hovered_element()
    {
        if (not maybe_hover_) {
            return;
        }

        add_body(shared_->upd_committable_model_graph(), maybe_hover_.pos, {maybe_hover_.id});
    }

    void try_creating_joint_from_hovered_element()
    {
        if (not maybe_hover_) {
            return;  // nothing hovered
        }

        const MiDocument& mg = shared_->get_model_graph();

        const MiObject* hovered_mi_object = mg.try_get_by_id(maybe_hover_.id);

        if (not hovered_mi_object) {
            return;  // current hover isn't in the current model graph
        }

        UID maybe_id = get_station_attachment_parent(mg, *hovered_mi_object);

        if (maybe_id == MiIDs::ground() || maybe_id == MiIDs::empty()) {
            return;  // can't attach to it as-if it were a body
        }

        const auto* body_el = mg.try_get_by_id<MiBody>(maybe_id);
        if (not body_el) {
            return;  // suggested attachment parent isn't in the current model graph?
        }

        transition_to_choosing_joint_parent(*body_el);
    }

    // try transitioning the shown UI layer to one where the user is assigning a mesh
    void try_transition_to_assigning_hover_and_selection_next_frame()
    {
        const MiDocument& mg = shared_->get_model_graph();

        std::unordered_set<UID> meshes;
        meshes.insert(mg.get_selected().begin(), mg.get_selected().end());
        if (maybe_hover_) {
            meshes.insert(maybe_hover_.id);
        }

        std::erase_if(meshes, [&mg](UID mesh_id) { return !mg.contains<MiMesh>(mesh_id); });

        if (meshes.empty()) {
            return;  // nothing to assign
        }

        std::unordered_set<UID> attachments;
        for (UID mesh_id : meshes) {
            attachments.insert(mg.get_by_id<MiMesh>(mesh_id).get_parent_id());
        }

        transition_to_assigning_meshes_next_frame(meshes, attachments);
    }

    void try_adding_station_at_mouse_pos_to_hovered_element()
    {
        if (not maybe_hover_) {
            return;
        }

        add_station_at_location(
            shared_->upd_committable_model_graph(), maybe_hover_.id, maybe_hover_.pos);
    }

    //
    // TRANSITIONS
    //
    // methods for transitioning the main 3D UI to some other state
    //

    // transition the shown UI layer to one where the user is assigning a mesh
    void transition_to_assigning_meshes_next_frame(
        const std::unordered_set<UID>& meshes, const std::unordered_set<UID>& existing_attachments)
    {
        ChooseElLayerOptions opts;
        opts.can_choose_bodies = true;
        opts.can_choose_ground = true;
        opts.can_choose_joints = false;
        opts.can_choose_meshes = false;
        opts.maybe_els_attaching_to = meshes;
        opts.is_attaching_toward_el = false;
        opts.maybe_els_being_replaced_by_choice = existing_attachments;
        opts.header = "choose mesh attachment (ESC to cancel)";
        opts.on_user_choice = [shared = shared_, meshes](std::span<UID> choices) {
            if (choices.empty()) {
                return false;
            }

            return try_assign_mesh_attachments(
                shared->upd_committable_model_graph(), meshes, choices.front());
        };

        // request a state transition
        maybe_3d_viewer_modal_ = std::make_shared<ChooseElLayer>(*this, shared_, opts);
    }

    // transition the shown UI layer to one where the user is choosing a joint parent
    void transition_to_choosing_joint_parent(const MiBody& child)
    {
        ChooseElLayerOptions opts;
        opts.can_choose_bodies = true;
        opts.can_choose_ground = true;
        opts.can_choose_joints = false;
        opts.can_choose_meshes = false;
        opts.header = "choose joint parent (ESC to cancel)";
        opts.maybe_els_attaching_to = {child.get_id()};
        opts.is_attaching_toward_el = false;  // away from the body
        opts.on_user_choice = [shared = shared_,
                               child_id = child.get_id()](std::span<UID> choices) {
            if (choices.empty()) {
                return false;
            }

            return try_create_joint(
                shared->upd_committable_model_graph(), child_id, choices.front());
        };
        maybe_3d_viewer_modal_ = std::make_shared<ChooseElLayer>(*this, shared_, opts);
    }

    // transition the shown UI layer to one where the user is choosing which element in the scene to
    // point an element's axis towards
    void transition_to_choosing_which_element_to_point_axis_towards(MiObject& el, int axis)
    {
        ChooseElLayerOptions opts;
        opts.can_choose_bodies = true;
        opts.can_choose_ground = true;
        opts.can_choose_joints = true;
        opts.can_choose_meshes = false;
        opts.can_choose_stations = true;
        opts.maybe_els_attaching_to = {el.get_id()};
        opts.header = "choose what to point towards (ESC to cancel)";
        opts.on_user_choice = [shared = shared_, id = el.get_id(), axis](std::span<UID> choices) {
            if (choices.empty()) {
                return false;
            }

            return point_axis_towards(
                shared->upd_committable_model_graph(), id, axis, choices.front());
        };
        maybe_3d_viewer_modal_ = std::make_shared<ChooseElLayer>(*this, shared_, opts);
    }

    // transition the shown UI layer to one where the user is choosing two elements that the given
    // axis should be aligned along (i.e. the direction vector from the first element to the second
    // element becomes the direction vector of the given axis)
    void transition_to_choosing_two_elements_to_align_axis_along(MiObject& el, int axis)
    {
        ChooseElLayerOptions opts;
        opts.can_choose_bodies = true;
        opts.can_choose_ground = true;
        opts.can_choose_joints = true;
        opts.can_choose_meshes = false;
        opts.can_choose_stations = true;
        opts.maybe_els_attaching_to = {el.get_id()};
        opts.header = "choose two elements to align the axis along (ESC to cancel)";
        opts.num_elements_user_must_choose = 2;
        opts.on_user_choice = [shared = shared_, id = el.get_id(), axis](std::span<UID> choices) {
            if (choices.size() < 2) {
                return false;
            }

            return try_orient_object_axis_along_two_objects(
                shared->upd_committable_model_graph(), id, axis, choices[0], choices[1]);
        };
        maybe_3d_viewer_modal_ = std::make_shared<ChooseElLayer>(*this, shared_, opts);
    }

    void transition_to_choosing_which_element_to_translate_to(MiObject& el)
    {
        ChooseElLayerOptions opts;
        opts.can_choose_bodies = true;
        opts.can_choose_ground = true;
        opts.can_choose_joints = true;
        opts.can_choose_meshes = false;
        opts.can_choose_stations = true;
        opts.maybe_els_attaching_to = {el.get_id()};
        opts.header = "choose what to translate to (ESC to cancel)";
        opts.on_user_choice = [shared = shared_, id = el.get_id()](std::span<UID> choices) {
            if (choices.empty()) {
                return false;
            }

            return try_translate_object_to_another_object(
                shared->upd_committable_model_graph(), id, choices.front());
        };
        maybe_3d_viewer_modal_ = std::make_shared<ChooseElLayer>(*this, shared_, opts);
    }

    void transition_to_choosing_elements_to_translate_between(MiObject& el)
    {
        ChooseElLayerOptions opts;
        opts.can_choose_bodies = true;
        opts.can_choose_ground = true;
        opts.can_choose_joints = true;
        opts.can_choose_meshes = false;
        opts.can_choose_stations = true;
        opts.maybe_els_attaching_to = {el.get_id()};
        opts.header = "choose two elements to translate between (ESC to cancel)";
        opts.num_elements_user_must_choose = 2;
        opts.on_user_choice = [shared = shared_, id = el.get_id()](std::span<UID> choices) {
            if (choices.size() < 2) {
                return false;
            }

            return try_translate_between_two_objects(
                shared->upd_committable_model_graph(), id, choices[0], choices[1]);
        };
        maybe_3d_viewer_modal_ = std::make_shared<ChooseElLayer>(*this, shared_, opts);
    }

    void transition_to_copying_something_elses_orientation(MiObject& el)
    {
        ChooseElLayerOptions opts;
        opts.can_choose_bodies = true;
        opts.can_choose_ground = true;
        opts.can_choose_joints = true;
        opts.can_choose_meshes = true;
        opts.maybe_els_attaching_to = {el.get_id()};
        opts.header = "choose which orientation to copy (ESC to cancel)";
        opts.on_user_choice = [shared = shared_, id = el.get_id()](std::span<UID> choices) {
            if (choices.empty()) {
                return false;
            }

            return try_copy_orientation(shared->upd_committable_model_graph(), id, choices.front());
        };
        maybe_3d_viewer_modal_ = std::make_shared<ChooseElLayer>(*this, shared_, opts);
    }

    // transition the shown UI layer to one where the user is choosing two mesh points that
    // the element should be oriented along
    void transition_to_orienting_element_along_two_mesh_points(MiObject& el, int axis)
    {
        Select2MeshPointsOptions opts;
        opts.on_two_points_chosen =
            [shared = shared_, id = el.get_id(), axis](Vector3 a, Vector3 b) {
                return try_orient_object_axis_along_two_points(
                    shared->upd_committable_model_graph(), id, axis, a, b);
            };
        maybe_3d_viewer_modal_ = std::make_shared<Select2MeshPointsLayer>(*this, shared_, opts);
    }

    // transition the shown UI layer to one where the user is choosing two mesh points that
    // the element sould be translated to the midpoint of
    void transition_to_translating_element_along_two_mesh_points(MiObject& el)
    {
        Select2MeshPointsOptions opts;
        opts.on_two_points_chosen = [shared = shared_, id = el.get_id()](Vector3 a, Vector3 b) {
            return try_translate_object_between_two_points(
                shared->upd_committable_model_graph(), id, a, b);
        };
        maybe_3d_viewer_modal_ = std::make_shared<Select2MeshPointsLayer>(*this, shared_, opts);
    }

    void transition_to_translating_element_to_mesh_average_center(MiObject& el)
    {
        ChooseElLayerOptions opts;
        opts.can_choose_bodies = false;
        opts.can_choose_ground = false;
        opts.can_choose_joints = false;
        opts.can_choose_meshes = true;
        opts.header = "choose a mesh (ESC to cancel)";
        opts.on_user_choice = [shared = shared_, id = el.get_id()](std::span<UID> choices) {
            if (choices.empty()) {
                return false;
            }

            return try_translate_to_mesh_average_center(
                shared->upd_committable_model_graph(), id, choices.front());
        };
        maybe_3d_viewer_modal_ = std::make_shared<ChooseElLayer>(*this, shared_, opts);
    }

    void transition_to_translating_element_to_mesh_bounds_center(MiObject& el)
    {
        ChooseElLayerOptions opts;
        opts.can_choose_bodies = false;
        opts.can_choose_ground = false;
        opts.can_choose_joints = false;
        opts.can_choose_meshes = true;
        opts.header = "choose a mesh (ESC to cancel)";
        opts.on_user_choice = [shared = shared_, id = el.get_id()](std::span<UID> choices) {
            if (choices.empty()) {
                return false;
            }

            return try_translate_to_mesh_bounds_center(
                shared->upd_committable_model_graph(), id, choices.front());
        };
        maybe_3d_viewer_modal_ = std::make_shared<ChooseElLayer>(*this, shared_, opts);
    }

    void transition_to_translating_element_to_mesh_mass_center(MiObject& el)
    {
        ChooseElLayerOptions opts;
        opts.can_choose_bodies = false;
        opts.can_choose_ground = false;
        opts.can_choose_joints = false;
        opts.can_choose_meshes = true;
        opts.header = "choose a mesh (ESC to cancel)";
        opts.on_user_choice = [shared = shared_, id = el.get_id()](std::span<UID> choices) {
            if (choices.empty()) {
                return false;
            }

            return try_translate_to_mesh_mass_center(
                shared->upd_committable_model_graph(), id, choices.front());
        };
        maybe_3d_viewer_modal_ = std::make_shared<ChooseElLayer>(*this, shared_, opts);
    }

    // transition the shown UI layer to one where the user is choosing another element that
    // the element should be translated to the midpoint of
    void transition_to_translating_element_to_another_elements_center(MiObject& el)
    {
        ChooseElLayerOptions opts;
        opts.can_choose_bodies = true;
        opts.can_choose_ground = true;
        opts.can_choose_joints = true;
        opts.can_choose_meshes = true;
        opts.maybe_els_attaching_to = {el.get_id()};
        opts.header = "choose where to place it (ESC to cancel)";
        opts.on_user_choice = [shared = shared_, id = el.get_id()](std::span<UID> choices) {
            if (choices.empty()) {
                return false;
            }

            return try_translate_object_to_another_object(
                shared->upd_committable_model_graph(), id, choices.front());
        };
        maybe_3d_viewer_modal_ = std::make_shared<ChooseElLayer>(*this, shared_, opts);
    }

    void transition_to_reassigning_cross_ref(MiObject& el, int crossref_idx)
    {
        int n_refs = el.get_num_cross_references();

        if (crossref_idx < 0 || crossref_idx >= n_refs) {
            return;  // invalid index?
        }

        const MiObject* old = shared_->get_model_graph().try_get_by_id(
            el.get_cross_reference_connectee_id(crossref_idx));

        if (not old) {
            return;  // old el doesn't exist?
        }

        ChooseElLayerOptions opts;
        opts.can_choose_bodies = (dynamic_cast<const MiBody*>(old) != nullptr) or
                                 (dynamic_cast<const MiGround*>(old) != nullptr);
        opts.can_choose_ground = (dynamic_cast<const MiBody*>(old) != nullptr) or
                                 (dynamic_cast<const MiGround*>(old) != nullptr);
        opts.can_choose_joints = dynamic_cast<const MiJoint*>(old) != nullptr;
        opts.can_choose_meshes = dynamic_cast<const Mesh*>(old) != nullptr;
        opts.maybe_els_attaching_to = {el.get_id()};
        opts.header = "choose what to attach to";
        opts.on_user_choice =
            [shared = shared_, id = el.get_id(), crossref_idx](std::span<UID> choices) {
                if (choices.empty()) {
                    return false;
                }
                return try_reassign_crossref(
                    shared->upd_committable_model_graph(), id, crossref_idx, choices.front());
            };
        maybe_3d_viewer_modal_ = std::make_shared<ChooseElLayer>(*this, shared_, opts);
    }

    // ensure any stale references into the modelgrah are cleaned up
    void garbage_collect_stale_refs()
    {
        const MiDocument& mg = shared_->get_model_graph();

        if (maybe_hover_ and not mg.contains(maybe_hover_.id)) {
            maybe_hover_.reset();
        }

        if (maybe_opened_context_menu_ and not mg.contains(maybe_opened_context_menu_.id)) {
            maybe_opened_context_menu_.reset();
        }
    }

    // delete currently-selected scene elements
    void delete_selected()
    {
        ::delete_selected(shared_->upd_committable_model_graph());
        garbage_collect_stale_refs();
    }

    // delete a particular scene element
    void delete_el(UID el_id)
    {
        delete_object(shared_->upd_committable_model_graph(), el_id);
        garbage_collect_stale_refs();
    }

    // update this scene from the current keyboard state, as saved by ImGui
    bool update_from_im_gui_keyboard_state()
    {
        if (ui::wants_keyboard()) {
            return false;
        }

        bool shift_down = ui::is_shift_down();
        bool ctrl_or_super_down = ui::is_ctrl_or_super_down();

        if (ctrl_or_super_down && ui::is_key_pressed(Key::N)) {
            // Ctrl+N: new scene
            shared_->request_new_mesh_importer_tab();
            return true;
        } else if (ctrl_or_super_down && ui::is_key_pressed(Key::O)) {
            // Ctrl+O: open osim
            shared_->open_osim_file_as_model_graph();
            return true;
        } else if (ctrl_or_super_down && shift_down && ui::is_key_pressed(Key::S)) {
            // Ctrl+Shift+S: export as: export scene as osim to user-specified location
            shared_->export_as_model_graph_as_osim_file();
            return true;
        } else if (ctrl_or_super_down && ui::is_key_pressed(Key::S)) {
            // Ctrl+S: export: export scene as osim according to typical export heuristic
            shared_->export_model_graph_as_osim_file();
            return true;
        } else if (ctrl_or_super_down && ui::is_key_pressed(Key::Q)) {
            // Ctrl+Q: quit application
            App::upd().request_quit();
            return true;
        } else if (ctrl_or_super_down && ui::is_key_pressed(Key::A)) {
            // Ctrl+A: select all
            shared_->select_all();
            return true;
        } else if (ctrl_or_super_down && shift_down && ui::is_key_pressed(Key::Z)) {
            // Ctrl+Shift+Z: redo
            shared_->redo_current_model_graph();
            return true;
        } else if (ctrl_or_super_down && ui::is_key_pressed(Key::Z)) {
            // Ctrl+Z: undo
            shared_->undo_current_model_graph();
            return true;
        } else if (ui::any_of_keys_down({Key::Delete, Key::Backspace})) {
            // Delete/Backspace: delete any selected elements
            delete_selected();
            return true;
        } else if (ui::is_key_pressed(Key::B)) {
            // B: add body to hovered element
            try_add_body_to_hovered_element();
            return true;
        } else if (ui::is_key_pressed(Key::A)) {
            // A: assign a parent for the hovered element
            try_transition_to_assigning_hover_and_selection_next_frame();
            return true;
        } else if (ui::is_key_pressed(Key::J)) {
            // J: try to create a joint
            try_creating_joint_from_hovered_element();
            return true;
        } else if (ui::is_key_pressed(Key::T)) {
            // T: try to add a station to the current hover
            try_adding_station_at_mouse_pos_to_hovered_element();
            return true;
        } else if (gizmo_.handle_keyboard_inputs()) {
            return true;
        } else if (ui::update_orbit_controller_from_keyboard_inputs(
                       shared_->upd_camera_controller(),
                       shared_->get_camera(),
                       shared_->scene_ui_rect(),
                       calc_scene_aabb())) {
            shared_->get_camera_controller().update_camera(shared_->upd_camera());
            return true;
        } else {
            return false;
        }
    }

    void draw_nothing_context_menu_content_header()
    {
        ui::draw_text(MSMICONS_BOLT " Actions");
        ui::same_line();
        ui::draw_text_disabled("(nothing clicked)");
        ui::draw_separator();
    }

    void draw_mi_object_context_menu_content_header(const MiObject& e)
    {
        ui::draw_text("{} {}", e.get_class().get_icon_utf8(), e.get_label());
        ui::same_line();
        ui::draw_text_disabled(get_context_menu_sub_header_text(shared_->get_model_graph(), e));
        ui::same_line();
        ui::draw_help_marker(e.get_class().get_name(), e.get_class().get_description());
        ui::draw_separator();
    }

    void draw_mi_object_prop_editors(const MiObject& e)
    {
        MiDocument& mg = shared_->upd_model_graph();

        // label/name editor
        if (e.can_change_label()) {
            std::string buf{static_cast<std::string_view>(e.get_label())};
            if (ui::draw_string_input("Name", buf)) {
                mg.upd_by_id(e.get_id()).set_label(buf);
            }
            if (ui::is_item_deactivated_after_edit()) {
                std::stringstream ss;
                ss << "changed " << e.get_class().get_name() << " name";
                shared_->commit_current_model_graph(std::move(ss).str());
            }
            ui::same_line();
            ui::draw_help_marker(
                "Component Name",
                "This is the name that the component will have in the exported OpenSim model.");
        }

        // position editor
        if (e.can_change_position()) {
            Vector3 translation = e.get_pos(mg);
            if (ui::draw_vector3_input("Translation", translation, "%.6f")) {
                mg.upd_by_id(e.get_id()).set_pos(mg, translation);
            }
            if (ui::is_item_deactivated_after_edit()) {
                std::stringstream ss;
                ss << "changed " << e.get_label() << "'s translation";
                shared_->commit_current_model_graph(std::move(ss).str());
            }
            ui::same_line();
            ui::draw_help_marker("Translation", MiStrings::c_translation_description);
        }

        // rotation editor
        if (e.can_change_rotation()) {
            EulerAngles eulers = to_euler_angles(normalize(e.rotation(shared_->get_model_graph())));

            if (ui::draw_angle3_input("Rotation", eulers, "%.6f")) {
                const Quaternion rotation_radians = osc::to_world_space_rotation_quaternion(eulers);
                mg.upd_by_id(e.get_id()).set_rotation(mg, rotation_radians);
            }
            if (ui::is_item_deactivated_after_edit()) {
                std::stringstream ss;
                ss << "changed " << e.get_label() << "'s rotation";
                shared_->commit_current_model_graph(std::move(ss).str());
            }
            ui::same_line();
            ui::draw_help_marker(
                "Rotation",
                "These are the rotation Euler angles for the component in ground. Positive rotations are anti-clockwise along that axis.\n\nNote: the numbers may contain slight rounding error, due to backend constraints. Your values *should* be accurate to a few decimal places.");
        }

        // scale factor editor
        if (e.can_change_scale()) {
            Vector3 scale_factors = e.get_scale(mg);
            if (ui::draw_vector3_input("Scale", scale_factors, "%.6f")) {
                mg.upd_by_id(e.get_id()).set_scale(mg, scale_factors);
            }
            if (ui::is_item_deactivated_after_edit()) {
                std::stringstream ss;
                ss << "changed " << e.get_label() << "'s scale";
                shared_->commit_current_model_graph(std::move(ss).str());
            }
            ui::same_line();
            ui::draw_help_marker(
                "Scale",
                "These are the scale factors of the component in ground. These scale-factors are applied to the element before any other transform (it scales first, then rotates, then translates).");
        }
    }

    // draw content of "Add" menu for some scene element
    void draw_add_other_to_mi_object_actions(MiObject& el, const Vector3& click_pos)
    {
        ui::push_style_var(ui::StyleVar::ItemSpacing, {10.0f, 10.0f});
        const ScopeExit g1{[] {
            ui::pop_style_var();
        }};

        int imgui_id = 0;
        ui::push_id(imgui_id++);
        const ScopeExit g2{[] {
            ui::pop_id();
        }};

        if (can_attach_mesh_to(el)) {
            if (ui::draw_menu_item(MSMICONS_CUBE " Meshes")) {
                shared_->prompt_user_for_mesh_files_and_push_them_onto_mesh_loader(el.get_id());
            }
            ui::draw_tooltip_if_item_hovered("Add Meshes", MiStrings::c_mesh_description);
        }
        ui::pop_id();

        ui::push_id(imgui_id++);
        if (el.has_physical_size()) {
            if (ui::begin_menu(MSMICONS_CIRCLE " Body")) {
                if (ui::draw_menu_item(MSMICONS_COMPRESS_ARROWS_ALT " at center")) {
                    add_body(
                        shared_->upd_committable_model_graph(),
                        el.get_pos(shared_->get_model_graph()),
                        el.get_id());
                }
                ui::draw_tooltip_if_item_hovered("Add Body", MiStrings::c_body_description);

                if (ui::draw_menu_item(MSMICONS_MOUSE_POINTER " at click position")) {
                    add_body(shared_->upd_committable_model_graph(), click_pos, el.get_id());
                }
                ui::draw_tooltip_if_item_hovered("Add Body", MiStrings::c_body_description);

                if (ui::draw_menu_item(MSMICONS_DOT_CIRCLE " at ground")) {
                    add_body(shared_->upd_committable_model_graph());
                }
                ui::draw_tooltip_if_item_hovered("Add body", MiStrings::c_body_description);

                if (const auto* mesh = dynamic_cast<const MiMesh*>(&el)) {
                    if (ui::draw_menu_item(MSMICONS_BORDER_ALL " at bounds center")) {
                        const AABB bounds = mesh->calc_bounds().value_or(AABB{});
                        const Vector3 location = centroid_of(bounds);
                        add_body(shared_->upd_committable_model_graph(), location, mesh->get_id());
                    }
                    ui::draw_tooltip_if_item_hovered("Add Body", MiStrings::c_body_description);

                    if (ui::draw_menu_item(MSMICONS_DIVIDE " at mesh average center")) {
                        const Vector3 location = average_center(*mesh);
                        add_body(shared_->upd_committable_model_graph(), location, mesh->get_id());
                    }
                    ui::draw_tooltip_if_item_hovered("Add Body", MiStrings::c_body_description);

                    if (ui::draw_menu_item(MSMICONS_WEIGHT " at mesh mass center")) {
                        const Vector3 location = mass_center_of(*mesh);
                        add_body(shared_->upd_committable_model_graph(), location, mesh->get_id());
                    }
                    ui::draw_tooltip_if_item_hovered("Add body", MiStrings::c_body_description);
                }

                ui::end_menu();
            }
        } else {
            if (ui::draw_menu_item(MSMICONS_CIRCLE " Body")) {
                add_body(
                    shared_->upd_committable_model_graph(),
                    el.get_pos(shared_->get_model_graph()),
                    el.get_id());
            }
            ui::draw_tooltip_if_item_hovered("Add Body", MiStrings::c_body_description);
        }
        ui::pop_id();

        ui::push_id(imgui_id++);
        if (const auto* body = dynamic_cast<const MiBody*>(&el)) {
            if (ui::draw_menu_item(MSMICONS_LINK " Joint")) {
                transition_to_choosing_joint_parent(*body);
            }
            ui::draw_tooltip_if_item_hovered(
                "Creating Joints",
                "Create a joint from this body (the \"child\") to some other body in the model (the \"parent\").\n\nAll bodies in an OpenSim model must eventually connect to ground via joints. If no joint is added to the body then OpenSim Creator will automatically add a WeldJoint between the body and ground.");
        }
        ui::pop_id();

        ui::push_id(imgui_id++);
        if (can_attach_station_to(el)) {
            if (el.has_physical_size()) {
                if (ui::begin_menu(MSMICONS_MAP_PIN " Station")) {
                    if (ui::draw_menu_item(MSMICONS_COMPRESS_ARROWS_ALT " at center")) {
                        add_station_at_location(
                            shared_->upd_committable_model_graph(),
                            el,
                            el.get_pos(shared_->get_model_graph()));
                    }
                    ui::draw_tooltip_if_item_hovered(
                        "Add Station", MiStrings::c_station_description);

                    if (ui::draw_menu_item(MSMICONS_MOUSE_POINTER " at click position")) {
                        add_station_at_location(
                            shared_->upd_committable_model_graph(), el, click_pos);
                    }
                    ui::draw_tooltip_if_item_hovered(
                        "Add Station", MiStrings::c_station_description);

                    if (ui::draw_menu_item(MSMICONS_DOT_CIRCLE " at ground")) {
                        add_station_at_location(
                            shared_->upd_committable_model_graph(), el, Vector3{});
                    }
                    ui::draw_tooltip_if_item_hovered(
                        "Add Station", MiStrings::c_station_description);

                    if (dynamic_cast<const Mesh*>(&el)) {
                        if (ui::draw_menu_item(MSMICONS_BORDER_ALL " at bounds center")) {
                            add_station_at_location(
                                shared_->upd_committable_model_graph(),
                                el,
                                centroid_of(
                                    el.calc_bounds(shared_->get_model_graph()).value_or(AABB{})));
                        }
                        ui::draw_tooltip_if_item_hovered(
                            "Add Station", MiStrings::c_station_description);
                    }

                    ui::end_menu();
                }
            } else {
                if (ui::draw_menu_item(MSMICONS_MAP_PIN " Station")) {
                    add_station_at_location(
                        shared_->upd_committable_model_graph(),
                        el,
                        el.get_pos(shared_->get_model_graph()));
                }
                ui::draw_tooltip_if_item_hovered("Add Station", MiStrings::c_station_description);
            }
        }
        // ~ScopeExit: implicitly calls ui::pop_id()
    }

    void draw_nothing_actions()
    {
        if (ui::draw_menu_item(MSMICONS_CUBE " Add Meshes")) {
            shared_->prompt_user_for_mesh_files_and_push_them_onto_mesh_loader();
        }
        ui::draw_tooltip_if_item_hovered("Add Meshes to the model", MiStrings::c_mesh_description);

        if (ui::begin_menu(MSMICONS_PLUS " Add Other")) {
            draw_add_other_menu_items();

            ui::end_menu();
        }
    }

    void draw_mi_object_actions(MiObject& el, const Vector3& click_pos)
    {
        if (ui::draw_menu_item(MSMICONS_CAMERA " Focus camera on this")) {
            shared_->focus_camera_on(
                centroid_of(el.calc_bounds(shared_->get_model_graph()).value_or(AABB{})));
        }
        ui::draw_tooltip_if_item_hovered(
            "Focus camera on this scene element",
            "Focuses the scene camera on this element. This is useful for tracking the camera around that particular object in the scene");

        if (ui::begin_menu(MSMICONS_PLUS " Add")) {
            draw_add_other_to_mi_object_actions(el, click_pos);
            ui::end_menu();
        }

        if (const auto* body = dynamic_cast<const MiBody*>(&el)) {
            if (ui::draw_menu_item(MSMICONS_LINK " Join to")) {
                transition_to_choosing_joint_parent(*body);
            }
            ui::draw_tooltip_if_item_hovered(
                "Creating Joints",
                "Create a joint from this body (the \"child\") to some other body in the model (the \"parent\").\n\nAll bodies in an OpenSim model must eventually connect to ground via joints. If no joint is added to the body then OpenSim Creator will automatically add a WeldJoint between the body and ground.");
        }

        if (el.can_delete()) {
            if (ui::draw_menu_item(MSMICONS_TRASH " Delete")) {
                delete_object(shared_->upd_committable_model_graph(), el.get_id());
                garbage_collect_stale_refs();
                ui::close_current_popup();
            }
            ui::draw_tooltip_if_item_hovered(
                "Delete",
                "Deletes the component from the model. Deletion is undo-able (use the undo/redo feature). Anything attached to this element (e.g. joints, meshes) will also be deleted.");
        }
    }

    // draw the "Translate" menu for any generic `MiObject`
    void draw_translate_menu(MiObject& el)
    {
        if (not el.can_change_position()) {
            return;  // can't change its position
        }

        if (not ui::begin_menu(MSMICONS_ARROWS_ALT " Translate")) {
            return;  // top-level menu isn't open
        }

        ui::push_style_var(ui::StyleVar::ItemSpacing, {10.0f, 10.0f});

        for (int i = 0, len = el.get_num_cross_references(); i < len; ++i) {
            std::string label = "To " + el.get_cross_reference_label(i);
            if (ui::draw_menu_item(label)) {
                try_translate_object_to_another_object(
                    shared_->upd_committable_model_graph(),
                    el.get_id(),
                    el.get_cross_reference_connectee_id(i));
            }
        }

        if (ui::draw_menu_item("To (select something)")) {
            transition_to_choosing_which_element_to_translate_to(el);
        }

        if (el.get_num_cross_references() == 2) {
            std::string label = "Between " + el.get_cross_reference_label(0) + " and " +
                                el.get_cross_reference_label(1);
            if (ui::draw_menu_item(label)) {
                UID a = el.get_cross_reference_connectee_id(0);
                UID b = el.get_cross_reference_connectee_id(1);
                try_translate_between_two_objects(
                    shared_->upd_committable_model_graph(), el.get_id(), a, b);
            }
        }

        if (ui::draw_menu_item("Between two scene elements")) {
            transition_to_choosing_elements_to_translate_between(el);
        }

        if (ui::draw_menu_item("Between two mesh points")) {
            transition_to_translating_element_along_two_mesh_points(el);
        }

        if (ui::draw_menu_item("To mesh bounds center")) {
            transition_to_translating_element_to_mesh_bounds_center(el);
        }
        ui::draw_tooltip_if_item_hovered(
            "Translate to mesh bounds center",
            "Translates the given element to the center of the selected mesh's bounding box. The bounding box is the smallest box that contains all mesh vertices");

        if (ui::draw_menu_item("To mesh average center")) {
            transition_to_translating_element_to_mesh_average_center(el);
        }
        ui::draw_tooltip_if_item_hovered(
            "Translate to mesh average center",
            "Translates the given element to the average center point of vertices in the selected mesh.\n\nEffectively, this adds each vertex location in the mesh, divides the sum by the number of vertices in the mesh, and sets the translation of the given object to that location.");

        if (ui::draw_menu_item("To mesh mass center")) {
            transition_to_translating_element_to_mesh_mass_center(el);
        }
        ui::draw_tooltip_if_item_hovered(
            "Translate to mesh mess center",
            "Translates the given element to the mass center of the selected mesh.\n\nCAREFUL: the algorithm used to do this heavily relies on your triangle winding (i.e. normals) being correct and your mesh being a closed surface. If your mesh doesn't meet these requirements, you might get strange results (apologies: the only way to get around that problems involves complicated voxelization and leak-detection algorithms :( )");

        ui::pop_style_var();
        ui::end_menu();
    }

    // draw the "Reorient" menu for any generic `MiObject`
    void draw_reorient_menu(MiObject& el)
    {
        if (!el.can_change_rotation()) {
            return;  // can't change its rotation
        }

        if (not ui::begin_menu(MSMICONS_REDO " Reorient")) {
            return;  // top-level menu isn't open
        }

        ui::push_style_var(ui::StyleVar::ItemSpacing, {10.0f, 10.0f});

        {
            auto draw_menu_content = [&](int axis) {
                for (int i = 0, len = el.get_num_cross_references(); i < len; ++i) {
                    std::string label = "Towards " + el.get_cross_reference_label(i);

                    if (ui::draw_menu_item(label)) {
                        point_axis_towards(
                            shared_->upd_committable_model_graph(),
                            el.get_id(),
                            axis,
                            el.get_cross_reference_connectee_id(i));
                    }
                }

                if (ui::draw_menu_item("Towards (select something)")) {
                    transition_to_choosing_which_element_to_point_axis_towards(el, axis);
                }

                if (ui::draw_menu_item("Along line between (select two elements)")) {
                    transition_to_choosing_two_elements_to_align_axis_along(el, axis);
                }

                if (ui::draw_menu_item("90 degress")) {
                    rotate_axis(shared_->upd_committable_model_graph(), el, axis, 90_deg);
                }

                if (ui::draw_menu_item("180 degrees")) {
                    rotate_axis(shared_->upd_committable_model_graph(), el, axis, 180_deg);
                }

                if (ui::draw_menu_item("Along two mesh points")) {
                    transition_to_orienting_element_along_two_mesh_points(el, axis);
                }
            };

            if (ui::begin_menu("x")) {
                draw_menu_content(0);
                ui::end_menu();
            }

            if (ui::begin_menu("y")) {
                draw_menu_content(1);
                ui::end_menu();
            }

            if (ui::begin_menu("z")) {
                draw_menu_content(2);
                ui::end_menu();
            }
        }

        if (ui::draw_menu_item("copy")) {
            transition_to_copying_something_elses_orientation(el);
        }

        if (ui::draw_menu_item("reset")) {
            el.set_xform(
                shared_->get_model_graph(),
                Transform{.translation = el.get_pos(shared_->get_model_graph())});
            shared_->commit_current_model_graph("reset " + el.get_label() + " orientation");
        }

        ui::pop_style_var();
        ui::end_menu();
    }

    // draw the "Mass" editor for a `BodyEl`
    void draw_mass_editor(const MiBody& body_el)
    {
        auto cur_mass = static_cast<float>(body_el.get_mass());
        if (ui::draw_float_input("Mass", &cur_mass, 0.0f, 0.0f, "%.6f")) {
            shared_->upd_model_graph()
                .upd_by_id<MiBody>(body_el.get_id())
                .set_mass(static_cast<double>(cur_mass));
        }
        if (ui::is_item_deactivated_after_edit()) {
            shared_->commit_current_model_graph("changed body mass");
        }
        ui::same_line();
        ui::draw_help_marker(
            "Mass",
            "The mass of the body. OpenSim defines this as 'unitless'; however, models conventionally use kilograms.");
    }

    // draw the "Joint Type" editor for a `JointEl`
    void draw_joint_type_editor(const MiJoint& joint_el)
    {
        if (ui::begin_combobox("Joint Type", joint_el.get_specific_type_name())) {
            for (const auto& joint : opyn::get_component_registry<OpenSim::Joint>()) {
                if (ui::draw_selectable(
                        joint.name(), joint.name() == joint_el.get_specific_type_name())) {

                    shared_->upd_model_graph()
                        .upd_by_id<MiJoint>(joint_el.get_id())
                        .set_specific_type_name(joint.name());
                    shared_->commit_current_model_graph("changed joint type");
                }
            }
            ui::end_combobox();
        }
        ui::same_line();
        ui::draw_help_marker(
            "Joint Type",
            "This is the type of joint that should be added into the OpenSim model. The joint's type dictates what types of motion are permitted around the joint center. See the official OpenSim documentation for an explanation of each joint type.");
    }

    // draw the "Reassign Connection" menu, which lets users change an element's cross reference
    void draw_reassign_crossref_menu(MiObject& el)
    {
        int n_refs = el.get_num_cross_references();

        if (n_refs == 0) {
            return;
        }

        if (ui::begin_menu(MSMICONS_EXTERNAL_LINK_ALT " Reassign Connection")) {
            ui::push_style_var(ui::StyleVar::ItemSpacing, {10.0f, 10.0f});

            for (int i = 0; i < n_refs; ++i) {
                CStringView label = el.get_cross_reference_label(i);
                if (ui::draw_menu_item(label)) {
                    transition_to_reassigning_cross_ref(el, i);
                }
            }

            ui::pop_style_var();
            ui::end_menu();
        }
    }

    void action_prompt_user_to_save_mesh_as_obj(const osc::Mesh& mesh)
    {
        // prompt user for a save location
        App::upd().prompt_user_to_save_file_with_extension_async(
            [mesh](std::optional<std::filesystem::path> p) {
                if (not p) {
                    return;  // user cancelled out of the prompt
                }

                const OBJMetadata obj_metadata{
                    App::get().application_name_with_version_and_buildid(),
                };

                // write transformed mesh to output
                try {
                    std::ofstream ofs;
                    ofs.exceptions(std::ofstream::failbit | std::ofstream::badbit);
                    ofs.open(*p, std::ios_base::out | std::ios_base::trunc | std::ios_base::binary);

                    OBJ::write(ofs, mesh, obj_metadata, OBJWriterFlag::NoWriteNormals);
                } catch (std::exception& e) {
                    log_error("error saving obj output to {}: {}", p->string(), e.what());
                }
            },
            "obj");
    }

    void action_prompt_user_to_save_mesh_as_stl(const osc::Mesh& mesh)
    {
        // prompt user for a save location
        App::upd().prompt_user_to_save_file_with_extension_async(
            [mesh](std::optional<std::filesystem::path> p) {
                if (not p) {
                    return;  // user cancelled out of the prompt
                }

                const STLMetadata stl_metadata{
                    App::get().application_name_with_version_and_buildid(),
                };

                // write transformed mesh to output
                try {
                    std::ofstream ofs;
                    ofs.exceptions(std::ofstream::failbit | std::ofstream::badbit);
                    ofs.open(*p, std::ios_base::out | std::ios_base::trunc | std::ios_base::binary);

                    STL::write(ofs, mesh, stl_metadata);
                } catch (std::exception& e) {
                    log_error("error saving stl output to {}: {}", p->string(), e.what());
                }
            },
            "stl");
    }

    void draw_save_mesh_menu(const MiMesh& el)
    {
        if (ui::begin_menu(MSMICONS_FILE_EXPORT " Export")) {
            ui::draw_text_disabled("With Respect to:");
            ui::draw_separator();
            for (const MiObject& mi_object : shared_->get_model_graph().iter()) {
                if (ui::begin_menu(mi_object.get_label())) {
                    ui::draw_text_disabled("Format:");
                    ui::draw_separator();

                    if (ui::draw_menu_item(".obj")) {
                        const Transform mi_object_to_ground =
                            mi_object.get_x_form(shared_->get_model_graph());
                        const Transform mesh_vert_to_ground = el.get_x_form();
                        const Matrix4x4 mesh_vert_to_mi_object_vert =
                            inverse_matrix4x4_cast(mi_object_to_ground) *
                            matrix4x4_cast(mesh_vert_to_ground);

                        osc::Mesh mesh = el.get_mesh_data();
                        mesh.transform_vertices(mesh_vert_to_mi_object_vert);
                        action_prompt_user_to_save_mesh_as_obj(mesh);
                    }

                    if (ui::draw_menu_item(".stl")) {
                        const Transform mi_object_to_ground =
                            mi_object.get_x_form(shared_->get_model_graph());
                        const Transform mesh_vert_to_ground = el.get_x_form();
                        const Matrix4x4 mesh_vert_to_mi_object_vert =
                            inverse_matrix4x4_cast(mi_object_to_ground) *
                            matrix4x4_cast(mesh_vert_to_ground);

                        osc::Mesh mesh = el.get_mesh_data();
                        mesh.transform_vertices(mesh_vert_to_mi_object_vert);
                        action_prompt_user_to_save_mesh_as_stl(mesh);
                    }

                    ui::end_menu();
                }
            }
            ui::end_menu();
        }
    }

    void draw_context_menu_spacer() { ui::draw_dummy({0.0f, 0.0f}); }

    // draw context menu content for when user right-clicks nothing
    void draw_nothing_context_menu_content()
    {
        draw_nothing_context_menu_content_header();
        draw_context_menu_spacer();
        draw_nothing_actions();
    }

    // draw context menu content for a `GroundEl`
    void draw_context_menu_content(MiGround& el, const Vector3& click_pos)
    {
        draw_mi_object_context_menu_content_header(el);
        draw_context_menu_spacer();
        draw_mi_object_actions(el, click_pos);
    }

    // draw context menu content for a `BodyEl`
    void draw_context_menu_content(MiBody& el, const Vector3& click_pos)
    {
        draw_mi_object_context_menu_content_header(el);

        draw_context_menu_spacer();

        draw_mi_object_prop_editors(el);
        draw_mass_editor(el);

        draw_context_menu_spacer();
        ui::draw_separator();
        draw_context_menu_spacer();

        draw_translate_menu(el);
        draw_reorient_menu(el);
        draw_reassign_crossref_menu(el);
        draw_mi_object_actions(el, click_pos);
    }

    // draw context menu content for an `MiMesh`
    void draw_context_menu_content(MiMesh& el, const Vector3& click_pos)
    {
        draw_mi_object_context_menu_content_header(el);

        draw_context_menu_spacer();

        draw_mi_object_prop_editors(el);

        draw_context_menu_spacer();
        ui::draw_separator();
        draw_context_menu_spacer();

        draw_translate_menu(el);
        draw_reorient_menu(el);
        draw_save_mesh_menu(el);
        draw_reassign_crossref_menu(el);
        draw_mi_object_actions(el, click_pos);
    }

    // draw context menu content for a `JointEl`
    void draw_context_menu_content(MiJoint& el, const Vector3& click_pos)
    {
        draw_mi_object_context_menu_content_header(el);

        draw_context_menu_spacer();

        draw_mi_object_prop_editors(el);
        draw_joint_type_editor(el);

        draw_context_menu_spacer();
        ui::draw_separator();
        draw_context_menu_spacer();

        draw_translate_menu(el);
        draw_reorient_menu(el);
        draw_reassign_crossref_menu(el);
        draw_mi_object_actions(el, click_pos);
    }

    // draw context menu content for a `MiStation`
    void draw_context_menu_content(MiStation& el, const Vector3& click_pos)
    {
        draw_mi_object_context_menu_content_header(el);

        draw_context_menu_spacer();

        draw_mi_object_prop_editors(el);

        draw_context_menu_spacer();
        ui::draw_separator();
        draw_context_menu_spacer();

        draw_translate_menu(el);
        draw_reorient_menu(el);
        draw_reassign_crossref_menu(el);
        draw_mi_object_actions(el, click_pos);
    }

    // draw context menu content for some scene element
    void draw_context_menu_content(MiObject& el, const Vector3& click_pos)
    {
        std::visit(
            Overload{
                [this, &click_pos](MiGround& el) {
                    this->draw_context_menu_content(el, click_pos);
                },
                [this, &click_pos](MiMesh& el) { this->draw_context_menu_content(el, click_pos); },
                [this, &click_pos](MiBody& el) { this->draw_context_menu_content(el, click_pos); },
                [this, &click_pos](MiJoint& el) { this->draw_context_menu_content(el, click_pos); },
                [this, &click_pos](MiStation& el) {
                    this->draw_context_menu_content(el, click_pos);
                },
            },
            el.to_variant());
    }

    // draw a context menu for the current state (if applicable)
    void draw_context_menu_content()
    {
        if (not maybe_opened_context_menu_) {
            // context menu not open, but just draw the "nothing" menu
            ui::push_id(UID::empty());
            const ScopeExit g{[] {
                ui::pop_id();
            }};
            draw_nothing_context_menu_content();
        } else if (maybe_opened_context_menu_.id == MiIDs::right_clicked_nothing()) {
            // context menu was opened on "nothing" specifically
            ui::push_id(UID::empty());
            const ScopeExit g{[] {
                ui::pop_id();
            }};
            draw_nothing_context_menu_content();
        } else if (
            MiObject* el =
                shared_->upd_model_graph().try_upd_by_id(maybe_opened_context_menu_.id)) {
            // context menu was opened on a scene element that exists in the modelgraph
            ui::push_id(el->get_id());
            const ScopeExit g{[] {
                ui::pop_id();
            }};
            draw_context_menu_content(*el, maybe_opened_context_menu_.pos);
        }

        // context menu should be closed under these conditions
        if (ui::any_of_keys_pressed({Key::Return, Key::Escape})) {
            maybe_opened_context_menu_.reset();
            ui::close_current_popup();
        }
    }

    // draw the content of the (undo/redo) "History" panel
    void draw_history_panel_content()
    {
        UndoRedoPanel::draw_content(shared_->upd_committable_model_graph());
    }

    void draw_navigator_element(const MiClass& c)
    {
        MiDocument& mg = shared_->upd_model_graph();

        ui::draw_text("{} {}", c.get_icon_utf8(), c.get_name_pluralized());
        ui::same_line();
        ui::draw_help_marker(c.get_name_pluralized(), c.get_description());
        ui::draw_vertical_spacer(5.0f / 15.0f);
        ui::indent();

        bool empty = true;
        for (const MiObject& el : mg.iter()) {
            if (el.get_class() != c) {
                continue;
            }

            empty = false;

            UID id = el.get_id();
            int styles = 0;

            if (id == maybe_hover_.id) {
                ui::push_style_color(ui::ColorVar::Text, OSCColors::hovered());
                ++styles;
            } else if (shared_->is_selected(id)) {
                ui::push_style_color(ui::ColorVar::Text, OSCColors::selected());
                ++styles;
            }

            ui::draw_text(el.get_label());

            ui::pop_style_color(styles);

            if (ui::is_item_hovered()) {
                maybe_hover_ = {id, {}};
            }

            if (ui::is_item_clicked(MouseButton::Left)) {
                if (not ui::is_shift_down()) {
                    shared_->upd_model_graph().de_select_all();
                }
                shared_->upd_model_graph().select(id);
            }

            if (ui::is_item_clicked(MouseButton::Right)) {
                maybe_opened_context_menu_ = MeshImporterHover{id, {}};
                ui::open_popup("##maincontextmenu");
                App::upd().request_redraw();
            }
        }

        if (empty) {
            ui::draw_text_disabled("(no {})", c.get_name_pluralized());
        }
        ui::unindent();
    }

    void draw_navigator_panel_content()
    {
        for (const MiClass& c : get_scene_el_classes()) {
            draw_navigator_element(c);
            ui::draw_vertical_spacer(5.0f / 15.0f);
        }

        // a navigator element might have opened the context menu in the navigator panel
        //
        // this can happen when the user right-clicks something in the navigator
        if (ui::begin_popup("##maincontextmenu")) {
            draw_context_menu_content();
            ui::end_popup();
        }
    }

    void draw_add_other_menu_items()
    {
        ui::push_style_var(ui::StyleVar::ItemSpacing, {10.0f, 10.0f});

        if (ui::draw_menu_item(MSMICONS_CUBE " Meshes")) {
            shared_->prompt_user_for_mesh_files_and_push_them_onto_mesh_loader();
        }
        ui::draw_tooltip_if_item_hovered("Add Meshes", MiStrings::c_mesh_description);

        if (ui::draw_menu_item(MSMICONS_CIRCLE " Body")) {
            add_body(shared_->upd_committable_model_graph());
        }
        ui::draw_tooltip_if_item_hovered("Add Body", MiStrings::c_body_description);

        if (ui::draw_menu_item(MSMICONS_MAP_PIN " Station")) {
            MiDocument& mg = shared_->upd_model_graph();
            auto& e = mg.emplace<MiStation>(
                UID{}, MiIDs::ground(), Vector3{}, MiStation::get_class().generate_name());
            mg.select_only(e);
        }
        ui::draw_tooltip_if_item_hovered("Add Station", MiStation::get_class().get_description());

        ui::pop_style_var();
    }

    void draw_3d_viewer_overlay_top_bar()
    {
        int imgui_id = 0;

        if (ui::draw_button(MSMICONS_CUBE " Add Meshes")) {
            shared_->prompt_user_for_mesh_files_and_push_them_onto_mesh_loader();
        }
        ui::draw_tooltip_if_item_hovered("Add Meshes to the model", MiStrings::c_mesh_description);

        ui::same_line();

        ui::draw_button(MSMICONS_PLUS " Add Other");
        ui::draw_tooltip_if_item_hovered("Add components to the model");

        if (ui::begin_popup_context_menu("##additemtoscenepopup", ui::PopupFlag::MouseButtonLeft)) {
            draw_add_other_menu_items();
            ui::end_popup();
        }

        ui::same_line();

        ui::draw_button(MSMICONS_PAINT_ROLLER " Colors");
        ui::draw_tooltip_if_item_hovered(
            "Change scene display colors",
            "This only changes the decroative display colors of model elements in this screen. Color changes are not saved to the exported OpenSim model. Changing these colors can be handy for spotting things, or constrasting scene elements more strongly");

        if (ui::begin_popup_context_menu(
                "##addpainttoscenepopup", ui::PopupFlag::MouseButtonLeft)) {
            std::span<const Color> colors = shared_->colors();
            std::span<const char* const> labels = shared_->get_color_labels();
            OSC_ASSERT(colors.size() == labels.size() && "every color should have a label");

            for (size_t i = 0; i < colors.size(); ++i) {
                Color color_val = colors[i];
                ui::push_id(imgui_id++);
                if (ui::draw_rgba_color_editor(labels[i], color_val)) {
                    shared_->set_color(i, color_val);
                }
                ui::pop_id();
            }
            ui::end_popup();
        }

        ui::same_line();

        ui::draw_button(MSMICONS_EYE " Visibility");
        ui::draw_tooltip_if_item_hovered(
            "Change what's visible in the 3D scene",
            "This only changes what's visible in this screen. Visibility options are not saved to the exported OpenSim model. Changing these visibility options can be handy if you have a lot of overlapping/intercalated scene elements");

        if (ui::begin_popup_context_menu(
                "##changevisibilitypopup", ui::PopupFlag::MouseButtonLeft)) {
            std::span<const bool> visibilities = shared_->get_visibility_flags();
            std::span<const char* const> labels = shared_->get_visibility_flag_labels();
            OSC_ASSERT(
                visibilities.size() == labels.size() &&
                "every visibility flag should have a label");

            for (size_t i = 0; i < visibilities.size(); ++i) {
                bool v = visibilities[i];
                ui::push_id(imgui_id++);
                if (ui::draw_checkbox(labels[i], &v)) {
                    shared_->set_visibility_flag(i, v);
                }
                ui::pop_id();
            }
            ui::end_popup();
        }

        ui::same_line();

        ui::draw_button(MSMICONS_LOCK " Interactivity");
        ui::draw_tooltip_if_item_hovered(
            "Change what your mouse can interact with in the 3D scene",
            "This does not prevent being able to edit the model - it only affects whether you can click that type of element in the 3D scene. Combining these flags with visibility and custom colors can be handy if you have heavily overlapping/intercalated scene elements.");

        if (ui::begin_popup_context_menu(
                "##changeinteractionlockspopup", ui::PopupFlag::MouseButtonLeft)) {
            std::span<const bool> interactables = shared_->get_ineractivity_flags();
            std::span<const char* const> labels = shared_->get_interactivity_flag_labels();
            OSC_ASSERT(interactables.size() == labels.size());

            for (size_t i = 0; i < interactables.size(); ++i) {
                bool v = interactables[i];
                ui::push_id(imgui_id++);
                if (ui::draw_checkbox(labels[i], &v)) {
                    shared_->set_interactivity_flag(i, v);
                }
                ui::pop_id();
            }
            ui::end_popup();
        }

        ui::same_line();

        ui::draw_gizmo_operation_selector(
            gizmo_,
            true,
            true,
            true,
            MSMICONS_ARROWS_ALT,
            MSMICONS_REDO,
            MSMICONS_EXPAND_ARROWS_ALT);

        ui::push_style_var(ui::StyleVar::ItemSpacing, {0.0f, 0.0f});
        ui::same_line();
        ui::pop_style_var();

        // local/global dropdown
        ui::draw_gizmo_mode_selector(gizmo_);
        ui::same_line();

        // scale factor
        {
            const CStringView tooltip_title = "Change scene scale factor";
            const CStringView tooltip_desc =
                "This rescales *some* elements in the scene. Specifically, the ones that have no 'size', such as body frames, joint frames, and the chequered floor texture.\n\nChanging this is handy if you are working on smaller or larger models, where the size of the (decorative) frames and floor are too large/small compared to the model you are working on.\n\nThis is purely decorative and does not affect the exported OpenSim model in any way.";

            float sf = shared_->get_scene_scale_factor();
            ui::set_next_item_width(ui::calc_text_size("1000.00").x());
            if (ui::draw_float_input("scene scale factor", &sf)) {
                shared_->set_scene_scale_factor(sf);
            }
            ui::draw_tooltip_if_item_hovered(tooltip_title, tooltip_desc);
        }

        ui::same_line();

        if (ui::draw_button(MSMICONS_RECYCLE " Reload Meshes")) {
            shared_->reload_meshes();
        }
        ui::draw_tooltip_body_only_if_item_hovered("Reloads all meshes in the scene from disk");
    }

    std::optional<AABB> calc_scene_aabb() const
    {
        return bounding_aabb_of(
            drawables_buffer_, [](const DrawableThing& drawable) -> std::optional<AABB> {
                if (drawable.id != MiIDs::empty()) {
                    return calc_bounds(drawable);
                }
                return std::nullopt;
            });
    }

    void draw_3d_viewer_overlay_bottom_bar()
    {
        ui::push_id("##3DViewerOverlay");

        // draw axis overlay in the bottom-left
        {
            CameraViewAxes axes;

            const Vector2 window_padding = ui::get_style_panel_padding();
            const Vector2 axes_top_left = shared_->scene_ui_rect().ypd_bottom_left() +
                                          Vector2{window_padding.x(), -window_padding.y()} -
                                          Vector2{0.0f, axes.dimensions().y()};
            ui::set_cursor_ui_position(axes_top_left);
            if (axes.draw(shared_->upd_camera_controller(), shared_->get_camera())) {
                shared_->get_camera_controller().update_camera(shared_->upd_camera());
            }
        }

        ui::set_cursor_ui_position(
            shared_->scene_ui_rect().ypd_bottom_left() + Vector2{100.0f, -55.0f});

        if (ui::draw_button(MSMICONS_SEARCH_MINUS)) {
            shared_->upd_camera_controller().radius *= 1.2f;
            shared_->get_camera_controller().update_camera(shared_->upd_camera());
        }
        ui::draw_tooltip_if_item_hovered("Zoom Out");

        ui::same_line();

        if (ui::draw_button(MSMICONS_SEARCH_PLUS)) {
            shared_->upd_camera_controller().radius *= 0.8f;
            shared_->get_camera_controller().update_camera(shared_->upd_camera());
        }
        ui::draw_tooltip_if_item_hovered("Zoom In");

        ui::same_line();

        if (ui::draw_button(MSMICONS_EXPAND_ARROWS_ALT)) {
            if (const std::optional<AABB> scene_aabb = calc_scene_aabb()) {
                shared_->upd_camera_controller().focus_on(
                    *scene_aabb, shared_->get_camera(), aspect_ratio_of(shared_->scene_dims()));
                shared_->get_camera_controller().update_camera(shared_->upd_camera());
            }
        }
        ui::draw_tooltip_if_item_hovered(
            "Autoscale Scene",
            "Zooms camera to try and fit everything in the scene into the viewer");

        ui::same_line();

        if (ui::draw_button("X")) {
            shared_->upd_camera_controller().theta = 90_deg;
            shared_->upd_camera_controller().phi = 0_deg;
            shared_->get_camera_controller().update_camera(shared_->upd_camera());
        }
        if (ui::is_item_clicked(MouseButton::Right)) {
            shared_->upd_camera_controller().theta = -90_deg;
            shared_->upd_camera_controller().phi = 0_deg;
            shared_->get_camera_controller().update_camera(shared_->upd_camera());
        }
        ui::draw_tooltip_if_item_hovered(
            "Face camera facing along X",
            "Right-clicking faces it along X, but in the opposite direction");

        ui::same_line();

        if (ui::draw_button("Y")) {
            shared_->upd_camera_controller().theta = 0_deg;
            shared_->upd_camera_controller().phi = 90_deg;
            shared_->get_camera_controller().update_camera(shared_->upd_camera());
        }
        if (ui::is_item_clicked(MouseButton::Right)) {
            shared_->upd_camera_controller().theta = 0_deg;
            shared_->upd_camera_controller().phi = -90_deg;
            shared_->get_camera_controller().update_camera(shared_->upd_camera());
        }
        ui::draw_tooltip_if_item_hovered(
            "Face camera facing along Y",
            "Right-clicking faces it along Y, but in the opposite direction");

        ui::same_line();

        if (ui::draw_button("Z")) {
            shared_->upd_camera_controller().theta = 0_deg;
            shared_->upd_camera_controller().phi = 0_deg;
            shared_->get_camera_controller().update_camera(shared_->upd_camera());
        }
        if (ui::is_item_clicked(MouseButton::Right)) {
            shared_->upd_camera_controller().theta = 180_deg;
            shared_->upd_camera_controller().phi = 0_deg;
            shared_->get_camera_controller().update_camera(shared_->upd_camera());
        }
        ui::draw_tooltip_if_item_hovered(
            "Face camera facing along Z",
            "Right-clicking faces it along Z, but in the opposite direction");

        ui::same_line();

        if (ui::draw_button(MSMICONS_CAMERA)) {
            shared_->reset_camera();
        }
        ui::draw_tooltip_if_item_hovered(
            "Reset camera",
            "Resets the camera to its default position (the position it's in when the wizard is first loaded)");

        ui::pop_id();
    }

    void draw_3d_viewer_overlay_convert_to_open_sim_model_button()
    {
        ui::push_style_var(ui::StyleVar::FramePadding, {10.0f, 10.0f});

        constexpr CStringView main_button_text = "Convert to OpenSim Model " MSMICONS_ARROW_RIGHT;
        constexpr CStringView setting_button_text = MSMICONS_COG;
        constexpr Vector2 spacing_between_main_and_settings_buttons = {1.0f, 0.0f};
        constexpr Vector2 margin = {25.0f, 35.0f};

        const Vector2 main_button_dims = ui::calc_button_size(main_button_text);
        const Vector2 setting_button_dims = ui::calc_button_size(setting_button_text);
        const Vector2 viewport_bottom_right = shared_->scene_ui_rect().ypd_bottom_right();

        const Vector2 button_top_left = {
            viewport_bottom_right.x() -
                (margin.x() + spacing_between_main_and_settings_buttons.x() +
                 setting_button_dims.x() + main_button_dims.x()),
            viewport_bottom_right.y() - (margin.y() + main_button_dims.y()),
        };

        ui::set_cursor_ui_position(button_top_left);
        ui::push_style_color(ui::ColorVar::Button, Color::dark_green());
        if (ui::draw_button(main_button_text)) {
            shared_->try_create_output_model();
        }
        ui::pop_style_color();

        ui::pop_style_var();
        ui::draw_tooltip_if_item_hovered(
            "Convert current scene to an OpenSim Model",
            "This will attempt to convert the current scene into an OpenSim model, followed by showing the model in OpenSim Creator's OpenSim model editor screen.\n\nYour progress in this tab will remain untouched.");

        ui::push_style_var(ui::StyleVar::FramePadding, {10.0f, 10.0f});
        ui::same_line(0.0f, spacing_between_main_and_settings_buttons.x());
        ui::draw_button(setting_button_text);
        ui::pop_style_var();

        if (ui::begin_popup_context_menu("##settingspopup", ui::PopupFlag::MouseButtonLeft)) {
            const ModelCreationFlags flags = shared_->get_model_creation_flags();

            {
                bool v = flags & ModelCreationFlags::ExportStationsAsMarkers;
                if (ui::draw_checkbox("Export Stations as Markers", &v)) {
                    const ModelCreationFlags new_flags =
                        v ? flags + ModelCreationFlags::ExportStationsAsMarkers
                          : flags - ModelCreationFlags::ExportStationsAsMarkers;
                    shared_->set_model_creation_flags(new_flags);
                }
            }

            ui::end_popup();
        }
    }

    void draw_3d_viewer_overlay()
    {
        draw_3d_viewer_overlay_top_bar();
        draw_3d_viewer_overlay_bottom_bar();
        draw_3d_viewer_overlay_convert_to_open_sim_model_button();
    }

    void draw_mi_object_tooltip(const MiObject& e) const
    {
        ui::begin_tooltip_nowrap();
        ui::draw_text("{} {}", e.get_class().get_icon_utf8(), e.get_label());
        ui::same_line();
        ui::draw_text_disabled(get_context_menu_sub_header_text(shared_->get_model_graph(), e));
        ui::end_tooltip_nowrap();
    }

    void draw_hover_tooltip()
    {
        if (not maybe_hover_) {
            return;  // nothing is hovered
        }

        if (const MiObject* e = shared_->get_model_graph().try_get_by_id(maybe_hover_.id)) {
            draw_mi_object_tooltip(*e);
        }
    }

    // draws 3D manipulator overlays (drag handles, etc.)
    void draw_selection3_d_manipulator_gizmos()
    {
        if (not shared_->has_selection()) {
            return;  // can only manipulate if selecting something
        }

        // if the user isn't *currently* manipulating anything, create an
        // up-to-date manipulation matrix
        //
        // this is so that we can at least *show* the manipulation axes, and
        // because the user might start manipulating during this frame
        if (not gizmo_.is_using()) {
            auto it = shared_->get_current_selection().begin();
            auto end = shared_->get_current_selection().end();

            if (it == end) {
                return;  // sanity exit
            }

            const MiDocument& mg = shared_->get_model_graph();

            int n = 0;

            Transform ras = mg.get_x_form_by_id(*it);
            ++it;
            ++n;

            while (it != end) {
                const Transform t = mg.get_x_form_by_id(*it);
                ras.translation += t.translation;
                ras.rotation += t.rotation;
                ras.scale += t.scale;
                ++it;
                ++n;
            }

            ras.translation /= static_cast<float>(n);
            ras.rotation /= static_cast<float>(n);
            ras.scale /= static_cast<float>(n);
            ras.rotation = normalize(ras.rotation);

            gizmo_model_matrix_ = matrix4x4_cast(ras);
        }

        // else: is using OR nselected > 0 (so draw it)

        Rect scene_rect = shared_->scene_ui_rect();

        const auto user_manipulation = gizmo_.draw(
            gizmo_model_matrix_,
            shared_->get_camera().view_matrix(),
            shared_->get_camera().projection_matrix(aspect_ratio_of(scene_rect)),
            scene_rect);

        if (gizmo_.was_using() and not gizmo_.is_using()) {
            // the user stopped editing, so save it and re-render
            shared_->commit_current_model_graph("manipulated selection");
            App::upd().request_redraw();
        }

        if (not user_manipulation) {
            return;  // no user manipulation this frame, exit early
        }

        for (UID id : shared_->get_current_selection()) {
            MiObject& el = shared_->upd_model_graph().upd_by_id(id);
            switch (gizmo_.operation()) {
            case ui::GizmoOperation::Rotate:
                el.apply_rotation(
                    shared_->get_model_graph(),
                    extract_eulers_xyz(user_manipulation->rotation),
                    Vector3{gizmo_model_matrix_[3]});
                break;
            case ui::GizmoOperation::Translate: {
                el.apply_translation(shared_->get_model_graph(), user_manipulation->translation);
                break;
            }
            case ui::GizmoOperation::Scale:
                el.apply_scale(shared_->get_model_graph(), user_manipulation->scale);
                break;
            default: break;
            }
        }
    }

    // perform a hovertest on the current 3D scene to determine what the user's mouse is over
    MeshImporterHover hovertest_scene(const std::vector<DrawableThing>& drawables)
    {
        if (not shared_->is_render_hovered()) {
            return maybe_hover_;
        }

        if (gizmo_.is_using()) {
            return MeshImporterHover{};
        }

        return shared_->do_hovertest(drawables);
    }

    // handle any side effects for current user mouse hover
    void handle_current_hover()
    {
        if (not shared_->is_render_hovered()) {
            return;  // nothing hovered
        }

        const bool lc_clicked = ui::is_mouse_released_without_dragging(MouseButton::Left);
        const bool shift_down = ui::is_shift_down();
        const bool alt_down = ui::is_alt_down();
        const bool is_using_gizmo = gizmo_.is_using();

        if (not maybe_hover_ and lc_clicked and not is_using_gizmo and not shift_down) {
            // user clicked in some empty part of the screen: clear selection
            shared_->de_select_all();
        } else if (maybe_hover_ and lc_clicked and not is_using_gizmo) {
            // user clicked hovered thing: select hovered thing
            if (not shift_down) {
                // user wasn't holding SHIFT, so clear selection
                shared_->de_select_all();
            }

            if (alt_down) {
                // ALT: only select the thing the mouse is over
                select_just_hover();
            } else {
                // NO ALT: select the "grouped items"
                select_anything_grouped_with_hover();
            }
        }
    }

    // generate 3D scene drawables for current state
    std::vector<DrawableThing>& generate_drawables()
    {
        drawables_buffer_.clear();

        for (const MiObject& e : shared_->get_model_graph().iter()) {
            shared_->append_drawables(e, drawables_buffer_);
        }

        if (shared_->is_showing_floor()) {
            drawables_buffer_.push_back(shared_->generate_floor_drawable());
        }

        return drawables_buffer_;
    }

    // draws main 3D viewer panel
    void draw_3d_viewer()
    {
        shared_->set_content_region_avail_as_scene_rect();

        std::vector<DrawableThing>& mi_objects = generate_drawables();

        // hovertest the generated geometry
        maybe_hover_ = hovertest_scene(mi_objects);
        handle_current_hover();

        // assign rim highlights based on hover
        for (DrawableThing& dt : mi_objects) {
            dt.flags = compute_flags(shared_->get_model_graph(), dt.id, maybe_hover_.id);
        }

        // draw 3D scene (effectively, as an ui::Image)
        shared_->draw_scene(mi_objects);
        if (shared_->is_render_hovered() and
            ui::is_mouse_released_without_dragging(MouseButton::Right) and not gizmo_.is_using()) {

            maybe_opened_context_menu_ = maybe_hover_;
            ui::open_popup("##maincontextmenu");
        }

        bool ctx_menu_showing = false;
        if (ui::begin_popup("##maincontextmenu")) {
            ctx_menu_showing = true;
            draw_context_menu_content();
            ui::end_popup();
        }

        if (shared_->is_render_hovered() and maybe_hover_ and
            (ctx_menu_showing ? maybe_hover_.id != maybe_opened_context_menu_.id : true)) {

            draw_hover_tooltip();
        }

        // draw overlays/gizmos
        draw_selection3_d_manipulator_gizmos();
        shared_->draw_connection_lines(maybe_hover_);
    }

    void draw_main_menu_file_menu()
    {
        if (ui::begin_menu("File")) {
            if (ui::draw_menu_item(MSMICONS_FILE " New", ModifierKey::Ctrl | Key::N)) {
                shared_->request_new_mesh_importer_tab();
            }

            ui::draw_separator();

            if (ui::draw_menu_item(MSMICONS_FOLDER_OPEN " Import", ModifierKey::Ctrl | Key::O)) {
                shared_->open_osim_file_as_model_graph();
            }
            ui::draw_tooltip_if_item_hovered(
                "Import osim into mesh importer",
                "Try to import an existing osim file into the mesh importer.\n\nBEWARE: the mesh importer is *not* an OpenSim model editor. The import process will delete information from your osim in order to 'jam' it into this screen. The main purpose of this button is to export/import mesh editor scenes, not to edit existing OpenSim models.");

            if (ui::draw_menu_item(MSMICONS_SAVE " Export", ModifierKey::Ctrl | Key::S)) {
                shared_->export_model_graph_as_osim_file();
            }
            ui::draw_tooltip_if_item_hovered(
                "Export mesh impoter scene to osim",
                "Try to export the current mesh importer scene to an osim.\n\nBEWARE: the mesh importer scene may not map 1:1 onto an OpenSim model, so re-importing the scene *may* change a few things slightly. The main utility of this button is to try and save some progress in the mesh importer.");

            if (ui::draw_menu_item(
                    MSMICONS_SAVE " Export As", ModifierKey::Ctrl | ModifierKey::Shift | Key::S)) {
                shared_->export_as_model_graph_as_osim_file();
            }
            ui::draw_tooltip_if_item_hovered(
                "Export mesh impoter scene to osim",
                "Try to export the current mesh importer scene to an osim.\n\nBEWARE: the mesh importer scene may not map 1:1 onto an OpenSim model, so re-importing the scene *may* change a few things slightly. The main utility of this button is to try and save some progress in the mesh importer.");

            ui::draw_separator();

            if (ui::draw_menu_item(MSMICONS_FOLDER_OPEN " Import Stations from CSV")) {
                auto popup = std::make_shared<ImportStationsFromCSVPopup>(
                    &owner(), "Import Stations from CSV", [state = shared_](auto data) {
                        action_import_landmarks(
                            state->upd_committable_model_graph(), data.landmarks, data.maybe_label);
                    });
                popup->open();
                popup_manager_.push_back(std::move(popup));
            }

            ui::draw_separator();

            if (ui::draw_menu_item(MSMICONS_TIMES " Close", ModifierKey::Ctrl | Key::W)) {
                shared_->request_close();
            }

            if (ui::draw_menu_item(MSMICONS_TIMES_CIRCLE " Quit", ModifierKey::Ctrl | Key::Q)) {
                App::upd().request_quit();
            }

            ui::end_menu();
        }
    }

    void draw_main_menu_edit_menu()
    {
        if (ui::begin_menu("Edit")) {
            if (ui::draw_menu_item(
                    MSMICONS_UNDO " Undo",
                    ModifierKey::Ctrl | Key::Z,
                    false,
                    shared_->can_undo_current_model_graph())) {
                shared_->undo_current_model_graph();
            }
            if (ui::draw_menu_item(
                    MSMICONS_REDO " Redo",
                    ModifierKey::Ctrl | ModifierKey::Shift | Key::Z,
                    false,
                    shared_->can_redo_current_model_graph())) {
                shared_->redo_current_model_graph();
            }
            ui::end_menu();
        }
    }

    void draw_main_menu_window_menu()
    {

        if (ui::begin_menu("Window")) {
            for (size_t i = 0; i < shared_->num_toggleable_panels(); ++i) {
                bool is_enabled = shared_->is_nth_panel_enabled(i);
                if (ui::draw_menu_item(shared_->get_nth_panel_name(i), std::nullopt, is_enabled)) {
                    shared_->set_nth_panel_enabled(i, !is_enabled);
                }
            }
            ui::end_menu();
        }
    }

    void draw_main_menu_about_menu() { MainMenuAboutTab{}.on_draw(); }

    // draws main 3D viewer, or a modal (if one is active)
    void draw_main_viewer_panel_or_modal()
    {
        if (maybe_3d_viewer_modal_) {
            // ensure it stays alive - even if it pops itself during the drawcall
            const std::shared_ptr<MeshImporterUILayer> ptr = maybe_3d_viewer_modal_;

            // open it "over" the whole UI as a "modal" - so that the user can't click things
            // outside of the panel
            ui::open_popup("##visualizermodalpopup");
            ui::set_next_panel_size(shared_->scene_dims());
            ui::set_next_panel_ui_position(shared_->scene_ui_rect().ypd_top_left());
            ui::push_style_var(ui::StyleVar::PanelPadding, {0.0f, 0.0f});

            const ui::PanelFlags modal_flags = {
                ui::PanelFlag::AlwaysAutoResize,
                ui::PanelFlag::NoTitleBar,
                ui::PanelFlag::NoMove,
                ui::PanelFlag::NoResize,
            };

            if (ui::begin_popup_modal("##visualizermodalpopup", nullptr, modal_flags)) {
                ui::pop_style_var();
                ptr->on_draw();
                ui::end_popup();
            } else {
                ui::pop_style_var();
            }
        } else {
            ui::push_style_var(ui::StyleVar::PanelPadding, {0.0f, 0.0f});
            if (ui::begin_panel("wizard_3dViewer")) {
                ui::pop_style_var();
                draw_3d_viewer();
                ui::set_cursor_panel_position(
                    ui::get_cursor_start_panel_position() + Vector2{10.0f, 10.0f});
                draw_3d_viewer_overlay();
            } else {
                ui::pop_style_var();
            }
            ui::end_panel();
        }
    }

    // data shared between states
    std::shared_ptr<MeshImporterSharedState> shared_;

    // buffer that's filled with drawable geometry during a drawcall
    std::vector<DrawableThing> drawables_buffer_;

    // (maybe) hover + world space location of the hover
    MeshImporterHover maybe_hover_;

    // (maybe) the scene element that the user opened a context menu for
    MeshImporterHover maybe_opened_context_menu_;

    // (maybe) the next state the host screen should transition to
    std::shared_ptr<MeshImporterUILayer> maybe_3d_viewer_modal_;

    // Gizmo state
    ui::Gizmo gizmo_;
    Matrix4x4 gizmo_model_matrix_ = identity<Matrix4x4>();

    // manager for active modal popups (importer popups, etc.)
    PopupManager popup_manager_;
};

osc::MeshImporterTab::MeshImporterTab(Widget* parent) :

    Tab{std::make_unique<Impl>(*this, parent)}
{}
osc::MeshImporterTab::MeshImporterTab(Widget* parent, std::vector<std::filesystem::path> files) :

    Tab{std::make_unique<Impl>(*this, parent, std::move(files))}
{}
bool osc::MeshImporterTab::impl_is_unsaved() const
{
    return private_data().is_unsaved();
}
std::future<TabSaveResult> osc::MeshImporterTab::impl_try_save()
{
    return private_data().try_save();
}
void osc::MeshImporterTab::impl_on_mount()
{
    private_data().on_mount();
}
void osc::MeshImporterTab::impl_on_unmount()
{
    private_data().on_unmount();
}
bool osc::MeshImporterTab::impl_on_event(Event& e)
{
    return private_data().on_event(e);
}
void osc::MeshImporterTab::impl_on_tick()
{
    private_data().on_tick();
}
void osc::MeshImporterTab::impl_on_draw_main_menu()
{
    private_data().draw_main_menu();
}
void osc::MeshImporterTab::impl_on_draw()
{
    private_data().on_draw();
}
