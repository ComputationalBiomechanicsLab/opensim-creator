#include "preview_experimental_data_tab.h"

#include <libopensimcreator/documents/model/model_state_pair_with_shared_environment.h>
#include <libopensimcreator/documents/model/undoable_model_actions.h>
#include <libopensimcreator/documents/model/undoable_model_state_pair.h>
#include <libopensimcreator/platform/msmicons.h>
#include <libopensimcreator/ui/shared/basic_widgets.h>
#include <libopensimcreator/ui/shared/component_context_menu.h>
#include <libopensimcreator/ui/shared/coordinate_editor_panel.h>
#include <libopensimcreator/ui/shared/model_status_bar.h>
#include <libopensimcreator/ui/shared/model_viewer_panel.h>
#include <libopensimcreator/ui/shared/model_viewer_panel_parameters.h>
#include <libopensimcreator/ui/shared/model_viewer_panel_right_click_event.h>
#include <libopensimcreator/ui/shared/navigator_panel.h>
#include <libopensimcreator/ui/shared/open_component_context_menu_event.h>
#include <libopensimcreator/ui/shared/output_watches_panel.h>
#include <libopensimcreator/ui/shared/properties_panel.h>

#include <libopynsim/documents/experimental_data/annotated_motion.h>
#include <libopynsim/documents/experimental_data/file_backed_storage.h>
#include <libopynsim/documents/model/model_state_pair.h>
#include <libopynsim/documents/file_filters.h>
#include <libopynsim/utilities/open_sim_helpers.h>
#include <liboscar/graphics/scene/scene_cache.h>
#include <liboscar/maths/closed_interval.h>
#include <liboscar/maths/vector.h>
#include <liboscar/platform/app.h>
#include <liboscar/platform/log.h>
#include <liboscar/ui/icon_cache.h>
#include <liboscar/ui/oscimgui.h>
#include <liboscar/ui/events/open_named_panel_event.h>
#include <liboscar/ui/events/open_popup_event.h>
#include <liboscar/ui/panels/log_viewer_panel.h>
#include <liboscar/ui/panels/panel_manager.h>
#include <liboscar/ui/panels/perf_panel.h>
#include <liboscar/ui/popups/popup_manager.h>
#include <liboscar/ui/tabs/tab_private.h>
#include <liboscar/ui/widgets/window_menu.h>
#include <OpenSim/Common/Object.h>
#include <OpenSim/Simulation/Model/Force.h>
#include <OpenSim/Simulation/Model/Model.h>
#include <OpenSim/Simulation/Model/ModelComponent.h>

#include <filesystem>
#include <memory>
#include <optional>
#include <exception>
#include <string_view>
#include <span>
#include <vector>

using namespace osc;

// Top-level UI state that's share-able between various panels in the
// preview experimental data UI.
namespace
{
    class PreviewExperimentalDataUiState final {
    public:
        // Customizes what happens when the UI is reinitialized from its
        // on-disk data.
        enum class ReinitializationFlag {
            None = 0,
            RecalculateTimeRange = 1 << 0,
            NUM_FLAGS = 1,
        };
        using ReinitializationFlags = Flags<ReinitializationFlag>;

        std::shared_ptr<ModelStatePairWithSharedEnvironment> upd_shared_model_ptr() const
        {
            return model_;
        }
        opyn::ModelStatePair& upd_model() { return *model_; }

        void on_tick()
        {
            // ensure the model is scrubbed to the current scrub time
            //
            // this might not be the case if (e.g.) an edit was made by an action that
            // re-finalizes the model at t=0, so use the version number to track potential
            // situations where that might've happened (#932)
            if (model_->get_state().getTime() != scrub_time_) {
                set_scrub_time(scrub_time_);
            }
        }

        bool is_model_loaded() const { return opyn::has_input_file_name(model_->get_model()); }

        void load_model_file(const std::filesystem::path& p)
        {
            model_->load_model(p);
            reinitialize_model_from_backing_data(
                "loaded model", ReinitializationFlag::RecalculateTimeRange);
            tab_name_override_ = p.filename().string();
        }

        void reload_all(
            std::string_view label = "reloaded model",
            ReinitializationFlags flags = ReinitializationFlag::None)
        {
            // reload/reset model
            if (opyn::has_input_file_name(model_->get_model())) {
                SceneCache dummy;
                action_reload_osim_from_disk(*model_, dummy);
            } else {
                model_->reset_model();
            }

            // if applicable, reload associated trajectory
            if (associated_trajectory_) {
                associated_trajectory_->reload_from_disk(model_->get_model());
            }

            // reinitialize everything else
            reinitialize_model_from_backing_data(label, flags);
        }

        void load_model_trajectory_file(const std::filesystem::path& path)
        {
            associated_trajectory_ = opyn::FileBackedStorage{model_->get_model(), path};
            reload_all("loaded trajactory", ReinitializationFlag::RecalculateTimeRange);
            tab_name_override_ = path.filename().string();
        }

        void load_motion_files(std::span<const std::filesystem::path> paths)
        {
            if (paths.empty()) {
                return;
            }

            associated_motion_files_.insert(
                associated_motion_files_.end(), paths.begin(), paths.end());
            reload_all(
                paths.size() == 1 ? "loaded motion" : "loaded motions",
                ReinitializationFlag::RecalculateTimeRange);
            tab_name_override_ = paths.back().filename().string();
        }

        void load_xml_as_open_sim_document(std::span<const std::filesystem::path> paths)
        {
            if (paths.empty()) {
                return;
            }

            associated_xml_documents_.insert(
                associated_xml_documents_.end(), paths.begin(), paths.end());
            reload_all(
                paths.size() == 1 ? "loaded XML document" : "loaded XML documents",
                ReinitializationFlag::RecalculateTimeRange);
            tab_name_override_ = paths.back().filename().string();
        }

        ClosedInterval<float> get_time_range() const { return time_range_; }

        void set_time_range(ClosedInterval<float> new_time_range) { time_range_ = new_time_range; }

        double get_scrub_time() const { return scrub_time_; }

        void set_scrub_time(double new_time)
        {
            SimTK::State& state = model_->upd_model().updWorkingState();
            state.setTime(new_time);

            if (associated_trajectory_) {
                opyn::update_state_from_storage_time(
                    model_->upd_model(),
                    state,
                    associated_trajectory_->mapper(),
                    associated_trajectory_->storage(),
                    new_time);
                // m_Model->updModel().assemble(state);
                // m_Model->updModel().equilibrateMuscles(state);
                model_->get_model().realizeReport(state);
            } else {
                // no associated motion: only change the time part of the state and re-realize
                model_->upd_model().equilibrateMuscles(state);
                model_->upd_model().realizeDynamics(state);
            }
            scrub_time_ = static_cast<float>(new_time);
        }

        void rollback_model() { model_->rollback(); }

        std::optional<std::string_view> get_tab_name_override() const { return tab_name_override_; }

    private:
        void reinitialize_model_from_backing_data(
            std::string_view label, ReinitializationFlags flags)
        {
            std::optional<ClosedInterval<float>> data_time_range;

            // hide forces that are computed from the model, because it's assumed that the
            // user only wants to visualize forces that come from externally-supplied data
            if (model_->get_model().countNumComponents() > 0) {
                for (auto& force : model_->upd_model().updComponentList<OpenSim::Force>()) {
                    force.set_appliesForce(false);
                }
            }

            // (re)load associated trajectory
            if (associated_trajectory_) {
                opyn::initialize_model(model_->upd_model());

                associated_trajectory_->reload_from_disk(model_->get_model());
                data_time_range = osc::bounding_interval_of(
                    data_time_range, associated_trajectory_->time_range());
            }

            // (re)load motions
            for (const std::filesystem::path& path : associated_motion_files_) {
                const auto& motion =
                    opyn::add_model_component<opyn::AnnotatedMotion>(model_->upd_model(), path);
                data_time_range = osc::bounding_interval_of(data_time_range, motion.time_range());
            }

            // (re)load associated XML files (e.g. `ExternalLoads`)
            for (const std::filesystem::path& path : associated_xml_documents_) {
                auto ptr = std::unique_ptr<OpenSim::Object>{
                    OpenSim::Object::makeObjectFromFile(path.string())};
                if (dynamic_cast<OpenSim::ModelComponent*>(ptr.get())) {
                    model_->upd_model().addModelComponent(
                        dynamic_cast<OpenSim::ModelComponent*>(ptr.release()));
                }
            }

            // care: state initialization is dependent on `m_AssociatedTrajectory`
            opyn::initialize_model(model_->upd_model());
            opyn::initialize_state(model_->upd_model());
            model_->commit(label);

            if (data_time_range and (flags & ReinitializationFlag::RecalculateTimeRange)) {
                time_range_ = *data_time_range;
                // Ensure scrub lands within the new time range.
                scrub_time_ = clamp(scrub_time_, time_range_);
            }
            set_scrub_time(scrub_time_);
        }

        std::shared_ptr<UndoableModelStatePair> model_ = std::make_shared<UndoableModelStatePair>();
        std::optional<opyn::FileBackedStorage> associated_trajectory_;
        std::vector<std::filesystem::path> associated_motion_files_;
        std::vector<std::filesystem::path> associated_xml_documents_;
        ClosedInterval<float> time_range_ = {0.0f, 10.0f};
        float scrub_time_ = 0.0f;
        std::optional<std::string> tab_name_override_;
    };

    class PreviewExperimentalDataTabToolbar final {
    public:
        explicit PreviewExperimentalDataTabToolbar(
            std::shared_ptr<PreviewExperimentalDataUiState> ui_state) :

            ui_state_{std::move(ui_state)}
        {}

        void on_draw()
        {
            if (begin_toolbar("##PreviewExperimentalDataToolbar", Vector2{5.0f, 5.0f})) {
                // load/reload etc.
                {
                    if (ui::draw_button("load model")) {
                        App::upd().prompt_user_to_select_file_async(
                            [state = ui_state_](const FileDialogResponse& response) {
                                if (response.size() != 1) {
                                    // Error, cancellation, or more than one file somehow selected.
                                    return;
                                }

                                // Guard against loading errors by emitting a log message rather
                                // than potentially crashing the main thread (#1068).
                                try {
                                    state->load_model_file(response.front());
                                } catch (const std::exception& ex) {
                                    log_error("error detected: {}", ex.what());
                                    log_error("rolling back model");
                                    state->rollback_model();
                                }
                            },
                            opyn::get_model_file_filters());
                    }

                    ui::same_line();
                    if (not ui_state_->is_model_loaded()) {
                        ui::begin_disabled();
                    }
                    if (ui::draw_button("load model trajectory sto/mot file")) {
                        App::upd().prompt_user_to_select_file_async(
                            [state = ui_state_](const FileDialogResponse& response) {
                                if (response.size() != 1) {
                                    // Error, cancellation, or more than one file somehow selected.
                                    return;
                                }

                                // Guard against loading errors by emitting a log message rather
                                // than potentially crashing the main thread (#1068).
                                try {
                                    state->load_model_trajectory_file(response.front());
                                } catch (const std::exception& ex) {
                                    log_error("error detected: {}", ex.what());
                                    log_error("rolling back model");
                                    state->rollback_model();
                                }
                            },
                            opyn::get_motion_file_filters());
                    }
                    if (not ui_state_->is_model_loaded()) {
                        ui::end_disabled();
                    }

                    ui::same_line();
                    if (ui::draw_button("load sto/mot/trc file")) {
                        App::upd().prompt_user_to_select_file_async(
                            [state = ui_state_](const FileDialogResponse& response) {
                                // Guard against loading errors by emitting a log message rather
                                // than potentially crashing the main thread (#1068).
                                try {
                                    state->load_motion_files(response);
                                } catch (const std::exception& ex) {
                                    log_error("error detected: {}", ex.what());
                                    log_error("rolling back model");
                                    state->rollback_model();
                                }
                            },
                            opyn::get_motion_file_filters_including_trc(),
                            std::nullopt,
                            true);
                    }

                    if (not ui_state_->is_model_loaded()) {
                        ui::begin_disabled();
                    }
                    ui::same_line();
                    if (ui::draw_button("load OpenSim XML")) {
                        App::upd().prompt_user_to_select_file_async(
                            [state = ui_state_](const FileDialogResponse& response) {
                                // Guard against loading errors by emitting a log message rather
                                // than potentially crashing the main thread (#1068).
                                try {
                                    state->load_xml_as_open_sim_document(response);
                                } catch (const std::exception& ex) {
                                    log_error("error detected: {}", ex.what());
                                    log_error("rolling back model");
                                    state->rollback_model();
                                }
                            },
                            opyn::get_open_sim_xml_file_filters());
                    }
                    if (not ui_state_->is_model_loaded()) {
                        ui::end_disabled();
                    }

                    ui::same_line();
                    if (ui::draw_button(MSMICONS_RECYCLE " reload all")) {
                        ui_state_->reload_all();
                    }
                }

                // scrubber
                ui::draw_same_line_with_vertical_separator();
                {
                    ClosedInterval<float> tr = ui_state_->get_time_range();
                    ui::set_next_item_width(ui::calc_text_size("<= xxxx").x());
                    if (ui::draw_float_input("<=", &tr.lower)) {
                        ui_state_->set_time_range(tr);
                    }

                    ui::same_line();
                    auto t = static_cast<float>(ui_state_->get_scrub_time());
                    ui::set_next_item_width(
                        ui::calc_text_size(
                            "----------------------------------------------------------------")
                            .x());
                    if (ui::draw_float_slider(
                            "t",
                            &t,
                            static_cast<float>(tr.lower),
                            static_cast<float>(tr.upper),
                            "%.6f")) {
                        ui_state_->set_scrub_time(t);
                    }

                    ui::same_line();
                    ui::draw_text("<=");
                    ui::same_line();
                    ui::set_next_item_width(ui::calc_text_size("<= xxxx").x());
                    if (ui::draw_float_input("##<=", &tr.upper)) {
                        ui_state_->set_time_range(tr);
                    }
                }

                // scaling, visualization toggles.
                ui::draw_same_line_with_vertical_separator();
                {
                    draw_scene_scale_factor_editor_controls(ui_state_->upd_model());
                    if (not icon_cache_) {
                        icon_cache_ = App::singleton<IconCache>(
                            App::resource_loader().with_prefix("OpenSimCreator/icons/"),
                            ui::get_text_line_height_in_current_panel() / 128.0f,
                            App::get().highest_device_pixel_ratio());
                    }
                    ui::same_line();
                    draw_all_decoration_toggle_buttons(ui_state_->upd_model(), *icon_cache_);
                }

                ui::draw_same_line_with_vertical_separator();
            }
            ui::end_panel();
        }

    private:
        std::shared_ptr<PreviewExperimentalDataUiState> ui_state_;
        std::shared_ptr<IconCache> icon_cache_;
    };
}

class osc::PreviewExperimentalDataTab::Impl final : public TabPrivate {
public:
    explicit Impl(PreviewExperimentalDataTab& owner, Widget* parent) :
        TabPrivate{owner, parent, c_default_tab_name}
    {
        panel_manager_->register_toggleable_panel(
            "Navigator", [this](Widget* parent, std::string_view panel_name) {
                return std::make_shared<NavigatorPanel>(
                    parent,
                    panel_name,
                    ui_state_->upd_shared_model_ptr(),
                    [this, parent](const OpenSim::ComponentPath& p) {
                        auto popup = std::make_unique<ComponentContextMenu>(
                            parent, "##componentcontextmenu", ui_state_->upd_shared_model_ptr(), p);
                        App::post_event<OpenPopupEvent>(*parent, std::move(popup));
                    });
            });
        panel_manager_->register_toggleable_panel(
            "Properties", [this](Widget* parent, std::string_view panel_name) {
                return std::make_shared<PropertiesPanel>(
                    parent, panel_name, ui_state_->upd_shared_model_ptr());
            });
        panel_manager_->register_toggleable_panel(
            "Log", [](Widget* parent, std::string_view panel_name) {
                return std::make_shared<LogViewerPanel>(parent, panel_name);
            });
        panel_manager_->register_toggleable_panel(
            "Coordinates", [this](Widget* parent, std::string_view panel_name) {
                return std::make_shared<CoordinateEditorPanel>(
                    parent, panel_name, ui_state_->upd_shared_model_ptr());
            });
        panel_manager_->register_toggleable_panel(
            "Performance", [](Widget* parent, std::string_view panel_name) {
                return std::make_shared<PerfPanel>(parent, panel_name);
            });
        panel_manager_->register_toggleable_panel(
            "Output Watches", [this](Widget* parent, std::string_view panel_name) {
                return std::make_shared<OutputWatchesPanel>(
                    parent, panel_name, ui_state_->upd_shared_model_ptr());
            });
        panel_manager_->register_spawnable_panel(
            "viewer",
            [this](Widget* parent, std::string_view panel_name) {
                auto on_right_click = [model = ui_state_->upd_shared_model_ptr(),
                                       parent,
                                       menu_name = std::string{panel_name} + "_contextmenu",
                                       editor_api =
                                           this](const ModelViewerPanelRightClickEvent& e) {
                    auto popup = std::make_unique<ComponentContextMenu>(
                        parent, menu_name, model, e.component_abs_path_or_empty);

                    App::post_event<OpenPopupEvent>(editor_api->owner(), std::move(popup));
                };
                ModelViewerPanelParameters panel_params{
                    ui_state_->upd_shared_model_ptr(), on_right_click};

                return std::make_shared<ModelViewerPanel>(parent, panel_name, panel_params);
            },
            1  // have one viewer open at the start
        );
    }

    void on_mount()
    {
        panel_manager_->on_mount();
        popup_manager_.on_mount();
    }

    void on_unmount()
    {
        // m_PopupManager.on_unmount();
        panel_manager_->on_unmount();
    }

    bool on_event(Event& e)
    {
        if (auto* open_popup_event = dynamic_cast<OpenPopupEvent*>(&e)) {
            if (open_popup_event->has_popup()) {
                auto popup = open_popup_event->take_popup();
                popup->set_parent(&owner());
                popup->open();
                popup_manager_.push_back(std::move(popup));
                return true;
            }
        } else if (const auto* named_panel = dynamic_cast<OpenNamedPanelEvent*>(&e)) {
            panel_manager_->set_toggleable_panel_activated(named_panel->panel_name(), true);
            return true;
        } else if (
            const auto* context_menu_event = dynamic_cast<OpenComponentContextMenuEvent*>(&e)) {

            auto popup = std::make_unique<ComponentContextMenu>(
                &owner(),
                "##componentcontextmenu",
                ui_state_->upd_shared_model_ptr(),
                context_menu_event->path());
            App::post_event<OpenPopupEvent>(owner(), std::move(popup));
            return true;
        }
        return false;
    }

    void on_tick()
    {
        ui_state_->on_tick();
        panel_manager_->on_tick();

        if (const auto override = ui_state_->get_tab_name_override()) {
            std::stringstream ss;
            ss << MSMICONS_BEZIER_CURVE << ' ' << *override;
            set_name(std::move(ss).str());
        } else {
            set_name(c_default_tab_name);
        }
    }

    void on_draw_main_menu() { window_menu_.on_draw(); }

    void on_draw()
    {
        try {
            ui::enable_dockspace_over_main_window();
            toolbar_.on_draw();
            panel_manager_->on_draw();
            status_bar_.on_draw();
            popup_manager_.on_draw();
            threw_exception_last_frame_ = false;
        } catch (const std::exception& ex) {
            if (threw_exception_last_frame_) {
                throw;
            }

            threw_exception_last_frame_ = true;
            log_error("error detected: {}", ex.what());
            log_error("rolling back model");
            ui_state_->rollback_model();
        }
    }

private:
    static constexpr std::string_view c_default_tab_name =
        MSMICONS_BEZIER_CURVE " Experimental Data";

    std::shared_ptr<PreviewExperimentalDataUiState> ui_state_ =
        std::make_shared<PreviewExperimentalDataUiState>();
    std::shared_ptr<PanelManager> panel_manager_ = std::make_shared<PanelManager>(&owner());
    PreviewExperimentalDataTabToolbar toolbar_{ui_state_};
    WindowMenu window_menu_{&owner(), panel_manager_};
    ModelStatusBar status_bar_{parent(), ui_state_->upd_shared_model_ptr()};
    PopupManager popup_manager_;
    bool threw_exception_last_frame_ = false;
};

CStringView osc::PreviewExperimentalDataTab::id()
{
    return "OpenSim/Experimental/PreviewExperimentalData";
}

osc::PreviewExperimentalDataTab::PreviewExperimentalDataTab(Widget* parent) :
    Tab{std::make_unique<Impl>(*this, parent)}
{}
void osc::PreviewExperimentalDataTab::impl_on_mount()
{
    private_data().on_mount();
}
void osc::PreviewExperimentalDataTab::impl_on_unmount()
{
    private_data().on_unmount();
}
bool osc::PreviewExperimentalDataTab::impl_on_event(Event& e)
{
    return private_data().on_event(e);
}
void osc::PreviewExperimentalDataTab::impl_on_tick()
{
    private_data().on_tick();
}
void osc::PreviewExperimentalDataTab::impl_on_draw_main_menu()
{
    private_data().on_draw_main_menu();
}
void osc::PreviewExperimentalDataTab::impl_on_draw()
{
    private_data().on_draw();
}
