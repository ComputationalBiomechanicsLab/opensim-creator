#include "model_warper_tab.h"

#include <libopensimcreator/documents/model/undoable_model_actions.h>
#include <libopensimcreator/documents/model/undoable_model_state_pair.h>
#include <libopensimcreator/platform/msmicons.h>
#include <libopensimcreator/platform/recent_files.h>
#include <libopensimcreator/ui/model_editor/model_editor_tab.h>
#include <libopensimcreator/ui/shared/basic_widgets.h>
#include <libopensimcreator/ui/shared/main_menu.h>
#include <libopensimcreator/ui/shared/model_viewer_panel.h>
#include <libopensimcreator/ui/shared/model_viewer_panel_parameters.h>
#include <libopensimcreator/ui/shared/object_properties_editor.h>

#include <libopynsim/documents/custom_components/in_memory_mesh.h>
#include <libopynsim/documents/landmarks/landmark_helpers.h>
#include <libopynsim/documents/landmarks/maybe_named_landmark_pair.h>
#include <libopynsim/documents/model/basic_model_state_pair.h>
#include <libopynsim/documents/model/model_state_pair.h>
#include <libopynsim/documents/file_filters.h>
#include <libopynsim/graphics/open_sim_decoration_generator.h>
#include <libopynsim/solvers/model_warper/all_scaling_step_types.h>
#include <libopynsim/solvers/model_warper/model_warper_v3_document.h>
#include <libopynsim/solvers/model_warper/scaling_document_validation_message.h>
#include <libopynsim/solvers/model_warper/scaling_state.h>
#include <libopynsim/solvers/model_warper/scaling_step.h>
#include <libopynsim/utilities/open_sim_helpers.h>
#include <libopynsim/utilities/simbody_x_oscar.h>
#include <libopynsim/tps3d.h>
#include <liboscar/formats/obj.h>
#include <liboscar/graphics/orbit_camera_controller.h>
#include <liboscar/maths/math_helpers.h>
#include <liboscar/maths/transform_functions.h>
#include <liboscar/platform/app.h>
#include <liboscar/platform/log.h>
#include <liboscar/ui/events/open_tab_event.h>
#include <liboscar/ui/events/reset_ui_context_event.h>
#include <liboscar/ui/panels/log_viewer_panel.h>
#include <liboscar/ui/panels/panel_manager.h>
#include <liboscar/ui/panels/perf_panel.h>
#include <liboscar/ui/tabs/tab_private.h>
#include <liboscar/ui/widgets/redo_button.h>
#include <liboscar/ui/widgets/undo_button.h>
#include <liboscar/ui/widgets/window_menu.h>
#include <liboscar/utilities/assertions.h>
#include <liboscar/utilities/typelist.h>
#include <liboscar/utilities/undo_redo.h>
#include <OpenSim/Simulation/Model/Frame.h>
#include <OpenSim/Simulation/Model/Model.h>
#include <OpenSim/Simulation/Wrap/WrapCylinder.h>
#include <OpenSim/Simulation/Wrap/WrapEllipsoid.h>

#include <algorithm>
#include <filesystem>
#include <memory>
#include <optional>
#include <ranges>
#include <sstream>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

using namespace osc;
using namespace opyn;

// Controller datastructures (i.e. middleware between the UI and the underlying model).
namespace
{
    // Top-level UI state that's shared between panels/widget that the UI manipulates.
    class ModelWarperV3UIState final : public std::enable_shared_from_this<ModelWarperV3UIState> {
    public:
        explicit ModelWarperV3UIState(Widget* parent) :
            parent_{parent}
        {}

        // Should be regularly called by the UI once per frame.
        void on_tick()
        {
            try {
                if (not deferred_actions_.empty()) {
                    for (auto& deferred_action : deferred_actions_) {
                        deferred_action(*this);
                    }
                    deferred_actions_.clear();

                    update_scaled_model();
                }
            } catch (const std::exception& ex) {
                log_error("error processing deferred actions: {}", ex.what());
            }
        }

        // Returns a shared readonly pointer to the top-level model warping document.
        std::shared_ptr<const ModelWarperV3Document> get_document_ptr() const
        {
            return scaling_state_->scratch().get_scaling_document_ptr();
        }

        bool has_scaling_steps() const { return scaling_state_->scratch().has_scaling_steps(); }
        auto iterate_scaling_steps() const
        {
            return scaling_state_->scratch().iterate_scaling_steps();
        }
        void add_scaling_step_deferred(std::unique_ptr<ScalingStep> step)
        {
            deferred_actions_.emplace_back([s = std::shared_ptr<ScalingStep>{std::move(step)}](
                                               ModelWarperV3UIState& state) mutable {
                state.scaling_state_->upd_scratch().add_scaling_step(
                    std::unique_ptr<ScalingStep>{s->clone()});
                state.scaling_state_->commit_scratch("Add scaling step");
            });
        }
        void erase_scaling_step_deferred(const ScalingStep& step)
        {
            deferred_actions_.emplace_back(
                [path = step.getAbsolutePath()](ModelWarperV3UIState& state) {
                    if (state.scaling_state_->upd_scratch().erase_scaling_step(path)) {
                        state.scaling_state_->commit_scratch("Erase scaling step");
                    }
                });
        }
        std::vector<ScalingStepValidationMessage> validate_step(const ScalingStep& step)
        {
            return step.validate(
                scaling_cache_,
                scaling_state_->scratch().get_effective_scaling_parameters(),
                scaling_state_->scratch().get_source_model());
        }

        bool has_scaling_parameters() const
        {
            return scaling_state_->scratch().has_scaling_parameter_declarations();
        }
        ScalingParameters get_effective_scaling_parameters() const
        {
            return scaling_state_->scratch().get_effective_scaling_parameters();
        }
        void set_scaling_parameter_value_deferred(
            const std::string& scaling_param_name, ScalingParameterValue new_value)
        {
            deferred_actions_.emplace_back(
                [scaling_param_name, new_value](ModelWarperV3UIState& state) {
                    if (state.scaling_state_->upd_scratch().set_scaling_parameter_override(
                            scaling_param_name, new_value)) {
                        state.scaling_state_->commit_scratch("Set scaling parameter");
                    }
                });
        }

        std::shared_ptr<ModelStatePair> source_model()
        {
            return scaling_state_->upd_scratch().get_source_model_ptr();
        }

        // result model stuff (note: might not be available if there's validation issues)
        using ScaledModelOrValidationErrorsOrScalingErrors = std::variant<
            std::shared_ptr<ModelStatePair>,                // successfully scaled model
            std::vector<ScalingDocumentValidationMessage>,  // validation messages
            std::string>;                                   // runtime scaling error message
        ScaledModelOrValidationErrorsOrScalingErrors scaled_model_or_document_validation_messages()
        {
            if (scaling_error_message_) {
                return scaling_error_message_.value();
            } else if (auto validation_messages =
                           scaling_state_->scratch().get_enabled_scaling_step_validation_messages(
                               scaling_cache_);
                       not validation_messages.empty()) {
                return validation_messages;
            } else {
                return scaled_model_;
            }
        }

        bool can_export_warped_model() const { return not scaling_error_message_.has_value(); }

        void export_warped_model_to_model_editor()
        {
            if (not can_export_warped_model()) {
                return;  // can't warp
            }

            auto scaling_result = scaled_model_or_document_validation_messages();
            if (not std::holds_alternative<std::shared_ptr<ModelStatePair>>(scaling_result)) {
                log_error("cannot export scaled model: could not create a warped model");
                return;
            }

            const OpenSim::Model& source_model = scaling_state_->scratch().get_source_model();
            const auto model_filesystem_location = try_find_input_file(source_model);
            if (not model_filesystem_location) {
                log_error(
                    "cannot export scaled model: can't figure out where the source model is on-disk");
                return;
            }

            std::shared_ptr<ModelStatePair> scaled =
                std::get<std::shared_ptr<ModelStatePair>>(scaling_result);

            std::unique_ptr<OpenSim::Model> copy =
                std::make_unique<OpenSim::Model>(scaled->get_model());
            initialize_model(*copy);
            initialize_state(*copy);
            {
                // TODO/FIXME/HACK: this code was thrown together to solve an immediate problem
                // of being able to export warped models, but it isn't very clean or robust (#1003).
                auto in_memory_meshes = scaled->get_model().getComponentList<InMemoryMesh>();
                if (in_memory_meshes.begin() != in_memory_meshes.end()) {
                    const auto warped_geometry_dir = try_get_warped_geometry_directory();
                    if (not warped_geometry_dir) {
                        log_error(
                            "cannot export scaled model: can't figure out where to save the warped meshes");
                        return;
                    }

                    // If the warped geometry directory doesn't exist yet, create it.
                    if (not std::filesystem::exists(*warped_geometry_dir)) {
                        std::filesystem::create_directories(*warped_geometry_dir);
                    }

                    // Export `InMemoryMesh`es to disk (#1003)
                    for (const InMemoryMesh& mesh : in_memory_meshes) {
                        // Figure out output file name.
                        const auto& input_mesh =
                            source_model.getComponent<OpenSim::Mesh>(mesh.getAbsolutePath());
                        const auto warped_mesh_abs_path = std::filesystem::weakly_canonical(
                            *warped_geometry_dir / std::filesystem::path{input_mesh.get_mesh_file()}
                                                       .filename()
                                                       .replace_extension(".obj"));

                        // Write warped mesh data to disk in an OBJ format.
                        {
                            std::ofstream obj_stream{warped_mesh_abs_path, std::ios::trunc};
                            obj_stream.exceptions(std::ios::badbit | std::ios::failbit);
                            OBJ::write(
                                obj_stream, mesh.osc_mesh(), OBJMetadata{"osc-model-warper"});
                        }

                        // Overwrite the `InMemoryMesh` in `copy`
                        auto& copy_mesh = copy->updComponent<InMemoryMesh>(mesh.getAbsolutePath());
                        auto new_mesh = std::make_unique<OpenSim::Mesh>();
                        new_mesh->set_mesh_file(warped_mesh_abs_path.string());
                        overwrite_geometry(*copy, copy_mesh, std::move(new_mesh));
                    }
                }
            }

            // Ensure the scaled model doesn't refer to the same filepath as the source
            // model, because it's  quite dangerous if the user can save over their source
            // model easily (#1146).
            copy->setInputFileName("");

            // Make the model undo-able
            auto undoable_model = std::make_unique<UndoableModelStatePair>(std::move(copy));

            // If the user requests it, bake the SDFs after finishing everything else
            if (get_bake_sd_fs()) {
                action_bake_station_defined_frames(*undoable_model);
            }

            auto editor = std::make_unique<ModelEditorTab>(parent_, std::move(undoable_model));
            App::post_event<OpenTabEvent>(*parent_, std::move(editor));
        }

        // camera stuff
        bool is_camera_controller_linked() const { return link_cameras_; }
        void set_camera_controller_linked(bool v) { link_cameras_ = v; }
        bool is_only_camera_controller_rotation_linked() const { return only_link_rotation_; }
        void set_only_camera_controller_rotation_linked(bool v) { only_link_rotation_ = v; }

        bool update_local_controller_if_linked(OrbitCameraController& camera_controller)
        {
            if (not is_camera_controller_linked()) {
                return false;
            }

            if (is_only_camera_controller_rotation_linked()) {
                camera_controller.theta = camera_controller_.theta;
                camera_controller.phi = camera_controller_.phi;
            } else {
                camera_controller = camera_controller_;
            }
            return true;
        }
        void update_shared_controller_if_linked(const OrbitCameraController& downstream_controller)
        {
            if (not is_camera_controller_linked()) {
                return;
            }
            if (is_only_camera_controller_rotation_linked()) {
                camera_controller_.phi = downstream_controller.phi;
                camera_controller_.theta = downstream_controller.theta;
            } else {
                camera_controller_ = downstream_controller;
            }
        }

        // undo/redo stuff
        std::shared_ptr<UndoRedoBase> get_undo_redo_ptr() { return scaling_state_; }

        // warped geometry dir stuff
        std::optional<std::filesystem::path> try_get_warped_geometry_directory() const
        {
            if (maybe_custom_warped_geometry_directory_) {
                return maybe_custom_warped_geometry_directory_;  // prio for user choice (#1046).
            }

            const OpenSim::Model& source_model = scaling_state_->scratch().get_source_model();
            const auto model_filesystem_location = try_find_input_file(source_model);
            if (model_filesystem_location) {
                return model_filesystem_location->parent_path() / "WarpedGeometry";
            }

            return std::nullopt;  // can't figure out where to save it :(
        }

        // SDF baking stuff
        bool get_bake_sd_fs() const { return bake_station_defined_frames_; }
        void set_bake_sd_fs(bool v) { bake_station_defined_frames_ = v; }

        // actions
        void action_create_new_source_model()
        {
            scaling_state_->upd_scratch().reset_source_model();
            update_scaled_model();
            scaling_state_->commit_scratch("Create new source model");
        }

        void action_open_osim_or_prompt_user(std::optional<std::filesystem::path> path)
        {
            if (path) {
                action_open_osim(*path);
            } else {
                if (not shared_from_this()) {
                    log_critical(
                        "can't open osim file selection dialog because the UI state isn't shared");
                    return;
                }

                App::upd().prompt_user_to_select_file_async(
                    [state = shared_from_this()](const FileDialogResponse& response) {
                        if (not state) {
                            return;  // Something bad has happened.
                        }
                        if (response.size() != 1) {
                            return;  // Error, cancellation, or the user somehow selected >1 file.
                        }
                        state->action_open_osim(response.front());
                    },
                    get_model_file_filters());
            }
        }

        void action_prompt_user_to_select_warped_geometry_directory()
        {
            App::upd().prompt_user_to_select_directory_async(
                [state = shared_from_this()](const FileDialogResponse& response) {
                    if (not state) {
                        return;  // Can't continue.
                    }
                    if (response.size() != 1) {
                        return;  // Error, cancellation, or the user somehow selected >1 file.
                    }

                    state->maybe_custom_warped_geometry_directory_ = response.front();
                });
        }

        void action_open_osim(const std::filesystem::path& path)
        {
            App::singleton<RecentFiles>()->push_back(path);
            scaling_state_->upd_scratch().load_source_model_from_osim(path);
            update_scaled_model();
            scaling_state_->commit_scratch("Loaded osim file");
        }

        void action_create_new_scaling_document()
        {
            scaling_state_->upd_scratch().reset_scaling_document();
            update_scaled_model();
            scaling_state_->commit_scratch("Create new scaling document");
        }

        void action_open_scaling_document()
        {
            if (not shared_from_this()) {
                log_critical(
                    "cannot open a file selection dialog because the UI state isn't reference-counted");
                return;
            }

            App::upd().prompt_user_to_select_file_async(
                [state = shared_from_this()](const FileDialogResponse& response) {
                    if (not state) {
                        return;  // Can't continue.
                    }
                    if (response.size() != 1) {
                        return;  // Error, cancellation, or the user somehow selected >1 file.
                    }

                    state->scaling_state_->upd_scratch().load_scaling_document(response.front());
                    state->update_scaled_model();
                    state->scaling_state_->commit_scratch("Loaded scaling document");
                },
                get_open_sim_xml_file_filters());
        }

        void action_save_scaling_document()
        {
            if (const auto existing_path =
                    scaling_state_->scratch().scaling_document_filesystem_location()) {
                scaling_state_->upd_scratch().get_scaling_document_ptr()->save_to(*existing_path);
                return;  // Document saved to existing filesystem location.
            }

            // Else: prompt the user to save it
            App::upd().prompt_user_to_save_file_with_extension_async(
                [doc = scaling_state_->upd_scratch().get_scaling_document_ptr()](
                    std::optional<std::filesystem::path> p) {
                    if (not p) {
                        return;  // user cancelled out of the prompt
                    }
                    doc->save_to(*p);
                },
                "xml");
        }

        void action_apply_object_edit_to_scaling_document(const ObjectPropertyEdit& edit)
        {
            scaling_state_->upd_scratch().apply_scaling_object_property_edit(edit);
            update_scaled_model();
            scaling_state_->commit_scratch("change scaling property");
        }

        void action_disable_scaling_step(const OpenSim::ComponentPath& path)
        {
            scaling_state_->upd_scratch().disable_scaling_step(path);
            update_scaled_model();
            scaling_state_->commit_scratch("disable scaling step");
        }

        void action_rollback() { scaling_state_->rollback(); }

        void action_retry_scaling_deferred()
        {
            deferred_actions_.emplace_back(
                [](ModelWarperV3UIState& state) mutable { state.update_scaled_model(); });
        }

        bool can_undo() const { return scaling_state_->can_undo(); }
        void action_undo() { scaling_state_->undo(); }

    private:
        void update_scaled_model()
        {
            try {
                scaling_cache_.clear();  // Prevents over-caching
                auto scaled_model =
                    scaling_state_->scratch().try_generate_scaled_model(scaling_cache_);
                if (not scaled_model) {
                    return;
                }

                scaled_model_ = std::move(scaled_model);
                scaling_error_message_.reset();
            } catch (const std::exception& ex) {
                scaling_error_message_ = ex.what();
            }
        }

        Widget* parent_ = nullptr;

        std::shared_ptr<UndoRedo<ScalingState>> scaling_state_ =
            std::make_shared<UndoRedo<ScalingState>>();

        ScalingCache scaling_cache_;
        std::shared_ptr<BasicModelStatePair> scaled_model_ =
            std::make_shared<BasicModelStatePair>();
        std::optional<std::string> scaling_error_message_;

        std::vector<std::function<void(ModelWarperV3UIState&)>> deferred_actions_;
        bool link_cameras_ = true;
        bool only_link_rotation_ = false;
        OrbitCameraController camera_controller_;

        std::optional<std::filesystem::path> maybe_custom_warped_geometry_directory_;
        bool bake_station_defined_frames_ = false;
    };
}

namespace
{
    Color ui_color(const ScalingStepValidationMessage& message)
    {
        switch (message.get_state()) {
        case ScalingStepValidationState::Warning: return Color::orange();
        case ScalingStepValidationState::Error: [[fallthrough]];
        default: return Color::muted_red();
        }
    }

    // source model 3D viewer
    class ModelWarperV3SourceModelViewerPanel final : public ModelViewerPanel {
    public:
        explicit ModelWarperV3SourceModelViewerPanel(
            Widget* parent, std::string_view label, std::shared_ptr<ModelWarperV3UIState> state) :

            ModelViewerPanel{
                parent,
                label,
                ModelViewerPanelParameters{state->source_model()},
                ModelViewerPanelFlag::NoHittest},
            state_{std::move(state)}
        {}

    private:
        void impl_draw_content() override
        {
            // if linked, read linked camera controller into local camera
            if (state_->update_local_controller_if_linked(upd_orbit_camera_controller())) {
                get_orbit_camera_controller().update_camera(upd_camera());
            }

            set_model_state(state_->source_model());
            ModelViewerPanel::impl_draw_content();

            // if linked, local controller may have been updated, push update to linked controller
            state_->update_shared_controller_if_linked(get_orbit_camera_controller());
        }

        std::shared_ptr<ModelWarperV3UIState> state_;
    };

    // result model 3D viewer
    class ModelWarperV3ResultModelViewerPanel final : public ModelViewerPanel {
    public:
        ModelWarperV3ResultModelViewerPanel(
            Widget* parent, std::string_view label, std::shared_ptr<ModelWarperV3UIState> state) :

            ModelViewerPanel{
                parent,
                label,
                ModelViewerPanelParameters{state->source_model()},
                ModelViewerPanelFlag::NoHittest},
            state_{std::move(state)}
        {}

    private:
        void impl_draw_content() override
        {
            std::visit(
                Overload{
                    [this](const std::shared_ptr<ModelStatePair>& scaled_model) {
                        draw_scaled_model_visualization(scaled_model);
                    },
                    [this](const std::vector<ScalingDocumentValidationMessage>& messages) {
                        draw_validation_error_message(messages);
                    },
                    [this](const std::string& scaling_error_message) {
                        draw_scaling_error_message(scaling_error_message);
                    },
                },
                state_->scaled_model_or_document_validation_messages());
        }

        void draw_scaled_model_visualization(const std::shared_ptr<ModelStatePair>& scaled_model)
        {
            // if linked, read linked camera controller into local camera
            if (state_->update_local_controller_if_linked(upd_orbit_camera_controller())) {
                get_orbit_camera_controller().update_camera(upd_camera());
            }

            set_model_state(scaled_model);
            ModelViewerPanel::impl_draw_content();

            // if linked, local controller may have been updated, push update to linked controller
            state_->update_shared_controller_if_linked(get_orbit_camera_controller());
        }

        void draw_validation_error_message(
            std::span<const ScalingDocumentValidationMessage> messages)
        {
            const float content_height = static_cast<float>(messages.size() + 2) *
                                         ui::get_text_line_height_in_current_panel();
            const float region_height = ui::get_content_region_available().y();
            const float top = 0.5f * (region_height - content_height);

            ui::set_cursor_panel_position({0.0f, top});

            // header line
            {
                std::stringstream ss;
                ss << "Cannot show model: " << messages.size() << " validation error"
                   << (messages.size() > 1 ? "s" : "") << " detected:";
                ui::draw_text_centered(std::move(ss).str());
            }

            // error line(s)
            int id = 0;
            for (const auto& message : messages) {
                ui::push_id(id++);

                ui::push_style_color(ui::ColorVar::Text, ui_color(message.payload));
                std::stringstream ss;
                ss << message.source_scaling_step_abs_path.getComponentName() << ": "
                   << message.payload.get_message();
                ui::draw_text_bullet_pointed(std::move(ss).str());
                ui::pop_style_color();

                ui::same_line();
                if (ui::draw_small_button("Disable Scaling Step")) {
                    state_->action_disable_scaling_step(message.source_scaling_step_abs_path);
                }

                ui::pop_id();
            }
        }

        void draw_scaling_error_message(CStringView message)
        {
            const float h = ui::get_content_region_available().y();
            const float line_height = ui::get_text_line_height_in_current_panel();
            constexpr float num_lines = 3.0f;
            const float top = 0.5f * (h - num_lines * line_height);

            ui::set_cursor_panel_position({0.0f, top});
            ui::draw_text_centered("An error occured while trying to scale the model:");
            ui::draw_text_centered(message);
            if (ui::draw_button_centered(MSMICONS_RECYCLE " Retry Scaling")) {
                state_->action_retry_scaling_deferred();
            }
        }

        std::shared_ptr<ModelWarperV3UIState> state_;
    };

    class WarpedModelExporterPopup final : public Popup {
    public:
        explicit WarpedModelExporterPopup(Widget* parent) :
            Popup{parent, "hello, popup world!"}
        {}

    private:
        void impl_draw_content() override { ui::draw_text("hello, popup content!"); }
    };

    // main toolbar
    class ModelWarperV3Toolbar final : public Widget {
    public:
        explicit ModelWarperV3Toolbar(
            Widget* parent, std::string_view label, std::shared_ptr<ModelWarperV3UIState> state) :
            Widget{parent},
            state_{std::move(state)}
        {
            set_name(label);
        }

    private:
        void impl_on_draw() override
        {
            if (begin_toolbar(name())) {
                draw_content();
            }
            ui::end_panel();
        }

        void draw_content()
        {
            int id = 0;

            ui::push_id(id++);
            ui::draw_vertical_separator();
            ui::same_line();
            ui::draw_text("Source Model: ");
            ui::same_line();
            if (ui::draw_button(MSMICONS_FILE)) {
                state_->action_create_new_source_model();
            }
            ui::same_line();
            draw_open_model_button_with_recent_files_dropdown([this](auto maybe_selection) {
                state_->action_open_osim_or_prompt_user(std::move(maybe_selection));
            });
            ui::same_line();
            ui::draw_vertical_separator();
            ui::pop_id();

            ui::push_id(id++);
            ui::same_line();
            ui::draw_text("Scaling Document: ");
            ui::same_line();
            if (ui::draw_button(MSMICONS_FILE)) {
                state_->action_create_new_scaling_document();
            }
            ui::same_line();
            if (ui::draw_button(MSMICONS_FOLDER_OPEN)) {
                state_->action_open_scaling_document();
            }
            ui::same_line();
            if (ui::draw_button(MSMICONS_SAVE)) {
                state_->action_save_scaling_document();
            }
            ui::same_line();
            ui::draw_vertical_separator();
            ui::pop_id();

            ui::push_id(id++);
            ui::same_line();
            undo_button_.on_draw();
            ui::pop_id();
            ui::push_id(id++);
            ui::same_line();
            redo_button_.on_draw();
            ui::same_line();
            ui::draw_vertical_separator();
            ui::pop_id();

            // Draw green "Warp Model" button
            {
                ui::push_id(id++);
                ui::same_line();
                bool disabled = false;
                if (not state_->can_export_warped_model()) {
                    ui::begin_disabled();
                    disabled = true;
                }

                ui::push_style_color(ui::ColorVar::Button, Color::dark_green());
                if (ui::draw_button(MSMICONS_PLAY " Export Warped Model")) {
                    state_->export_warped_model_to_model_editor();
                }
                ui::add_screenshot_annotation_to_last_drawn_item("Export Warped Model Button");
                ui::pop_style_color();
                ui::same_line(0.0f, 1.0f);
                if (ui::draw_button(MSMICONS_COG)) {
                    ui::open_popup("##WarpedModelExportModifiers");
                }

                if (ui::begin_popup("##WarpedModelExportModifiers")) {

                    // UI for warped geometry directory toggle
                    const auto geom_dir = state_->try_get_warped_geometry_directory();
                    if (geom_dir) {
                        ui::draw_text("Warped Geometry Directory: {}", geom_dir->string());
                    } else {
                        ui::draw_text("Warped Geometry Directory: UNKNOWN");
                    }
                    ui::same_line();
                    if (ui::draw_small_button("change")) {
                        state_->action_prompt_user_to_select_warped_geometry_directory();
                    }

                    // UI for baking `StationDefinedFrame`s during warping
                    {
                        bool bake_sd_fs = state_->get_bake_sd_fs();
                        if (ui::draw_checkbox("Bake StationDefinedFrames", &bake_sd_fs)) {
                            state_->set_bake_sd_fs(bake_sd_fs);
                        }
                    }

                    ui::end_popup();
                }
                if (disabled) {
                    ui::end_disabled();
                }
                ui::same_line();
                ui::draw_vertical_separator();
                ui::pop_id();
            }

            ui::push_id(id++);
            ui::same_line();
            if (bool v = state_->is_camera_controller_linked();
                ui::draw_checkbox("link cameras", &v)) {

                state_->set_camera_controller_linked(v);
            }

            ui::same_line();
            if (bool v = state_->is_only_camera_controller_rotation_linked();
                ui::draw_checkbox("only link rotation", &v)) {

                state_->set_only_camera_controller_rotation_linked(v);
            }
            ui::pop_id();
        }

        std::shared_ptr<ModelWarperV3UIState> state_;
        UndoButton undo_button_{this, state_->get_undo_redo_ptr(), MSMICONS_UNDO};
        RedoButton redo_button_{this, state_->get_undo_redo_ptr(), MSMICONS_REDO};
    };

    // control panel (design, set parameters, etc.)
    class ModelWarperV3ControlPanel final : public Panel {
    public:
        explicit ModelWarperV3ControlPanel(
            Widget* parent,
            std::string_view panel_name,
            std::shared_ptr<ModelWarperV3UIState> state) :

            Panel{parent, panel_name},
            state_{std::move(state)}
        {}

    private:
        void impl_draw_content() override
        {
            draw_scaling_parameters();
            ui::draw_vertical_spacer(0.75f);
            draw_scaling_steps();
        }

        void draw_scaling_parameters()
        {
            ui::draw_text_centered("Scaling Parameters");
            ui::draw_separator();
            ui::draw_vertical_spacer(0.5f);
            if (state_->has_scaling_parameters()) {
                if (ui::begin_table("##ScalingParameters", 2)) {
                    ui::table_setup_column("Name");
                    ui::table_setup_column("Value");
                    ui::table_headers_row();

                    int id = 0;
                    for (const auto& [name, value] : state_->get_effective_scaling_parameters()) {
                        ui::push_id(id++);
                        ui::table_next_row();
                        ui::table_set_column_index(0);
                        ui::draw_text(name);
                        ui::table_set_column_index(1);
                        auto value_copy{value};
                        ui::draw_double_input("##valueeditor", &value_copy, 0.0, 0.0, "%.6f");
                        if (ui::is_item_deactivated_after_edit()) {
                            state_->set_scaling_parameter_value_deferred(name, value_copy);
                        }
                        ui::pop_id();
                    }
                    ui::end_table();
                }
            } else {
                ui::draw_text_disabled_and_centered("No Scaling Parameters.");
                ui::draw_text_disabled_and_centered(
                    "(scaling parameters are normally implicitly added by scaling steps)");
            }
        }

        void draw_scaling_steps()
        {
            ui::draw_text_centered("Scaling Steps");
            ui::draw_separator();
            ui::draw_vertical_spacer(0.5f);

            if (state_->has_scaling_steps()) {
                size_t i = 0;
                for (const ScalingStep& step : state_->iterate_scaling_steps()) {
                    ui::push_id(step.getAbsolutePathString());
                    draw_scaling_step(i, step);
                    ui::pop_id();
                    ++i;
                }
            } else {
                ui::draw_text_disabled_and_centered("No scaling steps.");
                ui::draw_text_disabled_and_centered("(the model will be left unmodified)");
            }

            ui::draw_vertical_spacer(0.25f);
            draw_add_scaling_step_context_button();
        }

        void draw_scaling_step(size_t step_index, const ScalingStep& step)
        {
            // draw collapsing header, don't render content if it's collapsed
            {
                std::stringstream header;
                header << '#' << step_index + 1 << ": " << step.label();
                if (not ui::draw_collapsing_header(
                        std::move(header).str(), ui::TreeNodeFlag::DefaultOpen)) {
                    return;  // header is collapsed
                }
            }
            // else: header isn't collapsed

            ui::draw_help_marker(step.getDescription());

            // draw deletion button
            {
                constexpr auto deletion_button_icon = MSMICONS_TRASH;

                ui::same_line();

                const Vector2 old_cursor_pos = ui::get_cursor_panel_position();
                const float end_x = old_cursor_pos.x() + ui::get_content_region_available().x();

                const Vector2 new_cursor_pos = {
                    end_x - ui::calc_button_size(deletion_button_icon).x(), old_cursor_pos.y()};
                ui::set_cursor_panel_position(new_cursor_pos);
                if (ui::draw_small_button(deletion_button_icon)) {
                    state_->erase_scaling_step_deferred(step);
                }
            }

            // draw validation messages
            {
                const auto messages = state_->validate_step(step);
                if (not messages.empty()) {
                    ui::draw_vertical_spacer(0.2f);
                    ui::indent();
                    for (const ScalingStepValidationMessage& message : messages) {
                        ui::push_style_color(ui::ColorVar::Text, ui_color(message));
                        ui::draw_bullet_point();
                        if (const auto prop_name = message.try_get_property_name()) {
                            ui::draw_text("{}: {}", *prop_name, message.get_message());
                        } else {
                            ui::draw_text(message.get_message());
                        }
                        ui::pop_style_color();
                    }
                    ui::unindent();
                    ui::draw_vertical_spacer(0.2f);
                }
            }

            // draw property editors
            ui::indent(1.0f * ui::get_text_line_height_in_current_panel());
            {
                const auto path = step.getAbsolutePathString();
                const auto doc_ptr = state_->get_document_ptr();
                const auto [it, inserted] =
                    step_property_editors_.try_emplace(path, this, doc_ptr, [doc_ptr, path] {
                        return find_component(*doc_ptr, path);
                    });
                if (inserted) {
                    it->second.insert_in_blacklist("components");
                }
                if (auto object_edit = it->second.on_draw()) {
                    state_->action_apply_object_edit_to_scaling_document(
                        std::move(object_edit).value());
                }
            }
            ui::unindent(1.0f * ui::get_text_line_height_in_current_panel());
            ui::draw_vertical_spacer(0.5f);
        }

        void draw_add_scaling_step_context_button()
        {
            ui::draw_button(
                MSMICONS_PLUS "Add Scaling Step",
                {ui::get_content_region_available().x(), ui::calc_button_size("").y()});
            if (ui::begin_popup_context_menu(
                    "##AddScalingStepPopupMenu", ui::PopupFlag::MouseButtonLeft)) {
                for (const auto& ptr : get_scaling_step_prototypes()) {
                    ui::push_id(ptr.get());
                    if (ui::draw_selectable(ptr->label())) {
                        state_->add_scaling_step_deferred(
                            std::unique_ptr<ScalingStep>{ptr->clone()});
                    }
                    ui::draw_tooltip_if_item_hovered(
                        ptr->label(), ptr->getDescription(), ui::HoveredFlag::DelayNormal);
                    ui::pop_id();
                }
                ui::end_popup();
            }
        }

        std::shared_ptr<ModelWarperV3UIState> state_;
        std::unordered_map<std::string, ObjectPropertiesEditor> step_property_editors_;
    };
}

class osc::ModelWarperTab::Impl final : public TabPrivate {
public:
    static CStringView static_label() { return "OpenSim/ModelWarperV3"; }

    explicit Impl(Tab& owner, Widget* parent) :
        TabPrivate{owner, parent, static_label()}
    {
        panel_manager_->register_toggleable_panel(
            "Control Panel", [state = state_](Widget* parent, std::string_view panel_name) {
                return std::make_shared<ModelWarperV3ControlPanel>(parent, panel_name, state);
            });
        panel_manager_->register_toggleable_panel(
            "Source Model", [state = state_](Widget* parent, std::string_view panel_name) {
                return std::make_shared<ModelWarperV3SourceModelViewerPanel>(
                    parent, panel_name, state);
            });
        panel_manager_->register_toggleable_panel(
            "Result Model", [state = state_](Widget* parent, std::string_view panel_name) {
                return std::make_shared<ModelWarperV3ResultModelViewerPanel>(
                    parent, panel_name, state);
            });
        panel_manager_->register_toggleable_panel(
            "Log", [](Widget* parent, std::string_view panel_name) {
                return std::make_shared<LogViewerPanel>(parent, panel_name);
            });
        panel_manager_->register_toggleable_panel(
            "Performance", [](Widget* parent, std::string_view panel_name) {
                return std::make_shared<PerfPanel>(parent, panel_name);
            });
    }

    void on_mount() { panel_manager_->on_mount(); }

    void on_unmount() { panel_manager_->on_unmount(); }

    void on_tick()
    {
        state_->on_tick();
        panel_manager_->on_tick();
    }

    void on_draw_main_menu()
    {
        window_menu_.on_draw();
        about_tab_.on_draw();
    }

    void on_draw()
    {
        try {
            ui::enable_dockspace_over_main_window();
            panel_manager_->on_draw();
            toolbar_.on_draw();
            exception_thrown_last_frame_ = false;
        } catch (const std::exception& ex) {
            log_error(
                "an exception was thrown, probably due to an error in the document: {}", ex.what());

            // The exception might've left the UI context in an invalid state, so reset it.
            log_error("resetting the UI");
            App::notify<ResetUIContextEvent>(*parent());

            if (std::exchange(exception_thrown_last_frame_, true)) {
                // An exception was also thrown last frame - try to undo the state to try
                // and fix the problem for the user.
                if (state_->can_undo()) {
                    log_error("attempting to fix the problem by undo-ing the document");
                    state_->action_undo();
                } else {
                    // Undoing isn't possible, so we must defer this error to a higher level
                    // of the system.
                    log_critical("the document cannot be undone, elevating the exception");
                    throw;
                }
            } else {
                // No exception thrown last frame, it's likely that the exception is due to
                // an edit to the scratch space last frame, so try to softly roll the model
                // back.
                log_error("rolling back the document");
                state_->action_rollback();
            }
        }
    }

private:
    std::shared_ptr<ModelWarperV3UIState> state_ =
        std::make_shared<ModelWarperV3UIState>(&this->owner());

    std::shared_ptr<PanelManager> panel_manager_ = std::make_shared<PanelManager>(&owner());
    WindowMenu window_menu_{&owner(), panel_manager_};
    MainMenuAboutTab about_tab_;
    ModelWarperV3Toolbar toolbar_{&this->owner(), "##ModelWarperV3Toolbar", state_};

    bool exception_thrown_last_frame_ = false;
};

CStringView osc::ModelWarperTab::id()
{
    return Impl::static_label();
}

osc::ModelWarperTab::ModelWarperTab(Widget* parent) :
    Tab{std::make_unique<Impl>(*this, parent)}
{}
void osc::ModelWarperTab::impl_on_mount()
{
    private_data().on_mount();
}
void osc::ModelWarperTab::impl_on_unmount()
{
    private_data().on_unmount();
}
void osc::ModelWarperTab::impl_on_tick()
{
    private_data().on_tick();
}
void osc::ModelWarperTab::impl_on_draw_main_menu()
{
    private_data().on_draw_main_menu();
}
void osc::ModelWarperTab::impl_on_draw()
{
    private_data().on_draw();
}
