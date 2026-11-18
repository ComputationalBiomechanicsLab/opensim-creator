#include "simulation_tab.h"

#include <libopensimcreator/documents/simulation/abstract_simulation.h>
#include <libopensimcreator/documents/simulation/simulation.h>
#include <libopensimcreator/documents/simulation/simulation_clock.h>
#include <libopensimcreator/documents/simulation/simulation_model_state_pair.h>
#include <libopensimcreator/documents/simulation/simulation_report.h>
#include <libopensimcreator/platform/msmicons.h>
#include <libopensimcreator/ui/shared/component_context_menu.h>
#include <libopensimcreator/ui/shared/coordinate_editor_panel.h>
#include <libopensimcreator/ui/shared/model_status_bar.h>
#include <libopensimcreator/ui/shared/model_viewer_panel.h>
#include <libopensimcreator/ui/shared/model_viewer_panel_parameters.h>
#include <libopensimcreator/ui/shared/model_viewer_panel_right_click_event.h>
#include <libopensimcreator/ui/shared/navigator_panel.h>
#include <libopensimcreator/ui/shared/open_component_context_menu_event.h>
#include <libopensimcreator/ui/shared/properties_panel.h>
#include <libopensimcreator/ui/simulation/simulator_ui_api.h>
#include <libopensimcreator/ui/simulation/output_plots_panel.h>
#include <libopensimcreator/ui/simulation/simulation_details_panel.h>
#include <libopensimcreator/ui/simulation/simulation_tab_main_menu.h>
#include <libopensimcreator/ui/simulation/simulation_toolbar.h>
#include <libopensimcreator/ui/simulation/simulation_ui_playback_state.h>

#include <libopynsim/documents/output_extractors/component_output_extractor.h>
#include <libopynsim/documents/output_extractors/shared_output_extractor.h>
#include <liboscar/platform/app.h>
#include <liboscar/platform/events/event.h>
#include <liboscar/platform/events/key_event.h>
#include <liboscar/platform/os.h>
#include <liboscar/ui/oscimgui.h>
#include <liboscar/ui/events/open_named_panel_event.h>
#include <liboscar/ui/events/open_popup_event.h>
#include <liboscar/ui/panels/log_viewer_panel.h>
#include <liboscar/ui/panels/panel_manager.h>
#include <liboscar/ui/panels/perf_panel.h>
#include <liboscar/ui/popups/popup_manager.h>
#include <liboscar/ui/tabs/tab_private.h>
#include <liboscar/utilities/enum_helpers.h>
#include <liboscar/utilities/perf.h>
#include <OpenSim/Common/Component.h>
#include <OpenSim/Simulation/Model/Model.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdio>
#include <filesystem>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

using namespace osc;

namespace
{
    int get_next_simulation_number()
    {
        static std::atomic<int> s_simulation_number = 1;
        return s_simulation_number++;
    }
}

class osc::SimulationTab::Impl final : public TabPrivate, public SimulatorUIAPI {
public:
    explicit Impl(SimulationTab& owner, Widget* parent, std::shared_ptr<Simulation> simulation) :

        TabPrivate{
            owner,
            parent,
            MSMICONS_PLAY " Simulation_" + std::to_string(get_next_simulation_number())},
        simulation_{std::move(simulation)}
    {
        panel_manager_->register_toggleable_panel(
            "Navigator", [this](Widget* parent, std::string_view panel_name) {
                return std::make_shared<NavigatorPanel>(
                    parent,
                    panel_name,
                    shown_model_state_,
                    [this, parent](const OpenSim::ComponentPath& p) {
                        auto popup = std::make_shared<ComponentContextMenu>(
                            parent,
                            "##componentcontextmenu",
                            shown_model_state_,
                            p.toString(),
                            // #922: shouldn't open in simulator screen
                            ComponentContextMenuFlag::NoPlotVsCoordinate);
                        popup->open();
                        popup_manager_.push_back(std::move(popup));
                    });
            });
        panel_manager_->register_toggleable_panel(
            "Properties", [this](Widget* parent, std::string_view panel_name) {
                return std::make_shared<PropertiesPanel>(parent, panel_name, shown_model_state_);
            });
        panel_manager_->register_toggleable_panel(
            "Log", [](Widget* parent, std::string_view panel_name) {
                return std::make_shared<LogViewerPanel>(parent, panel_name);
            });
        panel_manager_->register_toggleable_panel(
            "Coordinates", [this](Widget* parent, std::string_view panel_name) {
                return std::make_shared<CoordinateEditorPanel>(
                    parent, panel_name, shown_model_state_);
            });
        panel_manager_->register_toggleable_panel(
            "Performance", [](Widget* parent, std::string_view panel_name) {
                return std::make_shared<PerfPanel>(parent, panel_name);
            });
        panel_manager_->register_toggleable_panel(
            "Output Watches", [this](Widget* parent, std::string_view panel_name) {
                return std::make_shared<OutputPlotsPanel>(
                    parent, panel_name, simulation_->try_upd_environment(), this);
            });
        panel_manager_->register_toggleable_panel(
            "Simulation Details", [this](Widget* parent, std::string_view panel_name) {
                return std::make_shared<SimulationDetailsPanel>(
                    parent, panel_name, this, simulation_);
            });
        panel_manager_->register_spawnable_panel(
            "viewer",
            [this](Widget* parent, std::string_view panel_name) {
                ModelViewerPanelParameters params{
                    shown_model_state_,
                    [this, parent, menu_name = std::string{panel_name} + "_contextmenu"](
                        const ModelViewerPanelRightClickEvent& e) {
                        auto popup = std::make_shared<ComponentContextMenu>(
                            parent,
                            menu_name,
                            shown_model_state_,
                            OpenSim::ComponentPath{e.component_abs_path_or_empty},
                            // #922: shouldn't open in simulator screen
                            ComponentContextMenuFlag::NoPlotVsCoordinate);
                        popup->open();
                        popup_manager_.push_back(std::move(popup));
                    },
                };

                return std::make_shared<ModelViewerPanel>(parent, panel_name, std::move(params));
            },
            1  // by default, open one viewer
        );
    }

    void on_mount()
    {
        App::upd().make_main_loop_waiting();
        popup_manager_.on_mount();
        panel_manager_->on_mount();
    }

    void on_unmount()
    {
        panel_manager_->on_unmount();
        App::upd().make_main_loop_polling();
    }

    void on_tick()
    {
        if (playback_state_ == SimulationUIPlaybackState::Playing) {

            const SimulationClock::time_point playback_pos = impl_get_simulation_scrub_time();

            if ((playback_speed_ >= 0.0f and playback_pos < simulation_->get_end_time()) or
                (playback_speed_ < 0.0f and playback_pos > simulation_->get_start_time())) {

                // if there's still something to playback, ensure the screen is re-rendered to show
                App::upd().request_redraw();
            } else if (looping_state_ == SimulationUILoopingState::Looping) {
                // there's nothing to playback, but the UI wants to loop the playback, so loop it
                set_simulation_scrub_time(simulation_->get_start_time());
            } else {
                // there's nothing to playback, so put playback into the stopped state
                playback_start_simtime_ = playback_pos;
                playback_state_ = SimulationUIPlaybackState::Stopped;
            }
        }

        panel_manager_->on_tick();
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
                &this->owner(),
                "##componentcontextmenu",
                shown_model_state_,
                context_menu_event->path().toString(),
                // #922: shouldn't open in simulator screen
                ComponentContextMenuFlag::NoPlotVsCoordinate);
            App::post_event<OpenPopupEvent>(owner(), std::move(popup));
            return true;
        }

        if (e.type() == EventType::KeyDown) {
            if (dynamic_cast<const KeyEvent&>(e).combination() == Key::Space) {
                toggle_playback_mode();
                return true;
            }
        }
        return false;
    }

    void on_draw_main_menu() { main_menu_.on_draw(); }

    void on_draw()
    {
        ui::enable_dockspace_over_main_window();

        draw_content();
    }

private:
    void toggle_playback_mode()
    {
        static_assert(num_options_v<SimulationUIPlaybackState> == 2);
        if (playback_state_ == SimulationUIPlaybackState::Playing) {
            // pause
            set_simulation_playback_state(SimulationUIPlaybackState::Stopped);
        } else if (get_simulation_scrub_time() >= simulation_->get_end_time()) {
            // replay
            set_simulation_scrub_time(simulation_->get_start_time());
            set_simulation_playback_state(SimulationUIPlaybackState::Playing);
        } else {
            // unpause
            set_simulation_playback_state(SimulationUIPlaybackState::Playing);
        }
    }

    std::optional<SimulationReport> try_find_nth_report_after(
        SimulationClock::time_point t, int offset = 0)
    {
        const ptrdiff_t num_simulation_reports = simulation_->get_num_reports();

        if (num_simulation_reports == 0) {
            return std::nullopt;
        }

        ptrdiff_t zeroeth_report_index = static_cast<ptrdiff_t>(num_simulation_reports) - 1;
        for (ptrdiff_t i = 0; i < num_simulation_reports; ++i) {
            const SimulationReport r = simulation_->get_simulation_report(i);
            if (r.get_time() >= t) {
                zeroeth_report_index = i;
                break;
            }
        }

        const ptrdiff_t report_index = zeroeth_report_index + offset;
        if (0 <= report_index and report_index < num_simulation_reports) {
            return simulation_->get_simulation_report(report_index);
        } else {
            return std::nullopt;
        }
    }

    const AbstractSimulation& impl_get_simulation() const override { return *simulation_; }

    AbstractSimulation& impl_upd_simulation() override { return *simulation_; }

    SimulationUIPlaybackState impl_get_simulation_playback_state() override
    {
        return playback_state_;
    }

    void impl_set_simulation_playback_state(SimulationUIPlaybackState new_state) override
    {
        if (new_state == SimulationUIPlaybackState::Playing) {
            playback_start_wall_time_ = std::chrono::steady_clock::now();
            playback_state_ = new_state;
        } else {
            playback_start_simtime_ = get_simulation_scrub_time();
            playback_state_ = new_state;
        }
    }

    SimulationUILoopingState impl_get_simulation_looping_state() const override
    {
        return looping_state_;
    }

    void impl_set_simulation_looping_state(SimulationUILoopingState s) override
    {
        looping_state_ = s;
    }

    float impl_get_simulation_playback_speed() override { return playback_speed_; }

    void impl_set_simulation_playback_speed(float v) override { playback_speed_ = v; }

    SimulationClock::time_point impl_get_simulation_scrub_time() override
    {
        if (playback_state_ == SimulationUIPlaybackState::Stopped) {
            return playback_start_simtime_;
        }

        // else: map the computer's wall time onto simulation time

        const ptrdiff_t n_reports = simulation_->get_num_reports();
        if (n_reports <= 0) {
            return simulation_->get_start_time();
        } else {
            const auto wall_now = std::chrono::steady_clock::now();
            const auto wall_dur = wall_now - playback_start_wall_time_;

            const SimulationClock::duration sim_dur =
                playback_speed_ * SimulationClock::duration{wall_dur};
            const SimulationClock::time_point sim_now = playback_start_simtime_ + sim_dur;
            const SimulationClock::time_point sim_earliest =
                simulation_->get_simulation_report(0).get_time();
            const SimulationClock::time_point sim_latest =
                simulation_->get_simulation_report(n_reports - 1).get_time();

            if (sim_now < sim_earliest) {
                return sim_earliest;
            } else if (sim_now > sim_latest) {
                return sim_latest;
            } else {
                return sim_now;
            }
        }
    }

    void impl_set_simulation_scrub_time(SimulationClock::time_point t) override
    {
        playback_start_simtime_ = t;
        playback_start_wall_time_ = std::chrono::steady_clock::now();
    }

    void impl_step_back() override
    {
        const std::optional<SimulationReport> maybe_prev =
            try_find_nth_report_after(get_simulation_scrub_time(), -1);
        if (maybe_prev) {
            set_simulation_scrub_time(maybe_prev->get_time());
        }
    }

    void impl_step_forward() override
    {
        const std::optional<SimulationReport> maybe_next =
            try_find_nth_report_after(get_simulation_scrub_time(), 1);
        if (maybe_next) {
            set_simulation_scrub_time(maybe_next->get_time());
        }
    }

    std::optional<SimulationReport> impl_try_select_report_based_on_scrubbing() override
    {
        return try_find_nth_report_after(get_simulation_scrub_time());
    }

    SimulationModelStatePair* impl_try_get_current_simulation_state() override
    {
        return shown_model_state_.get();
    }

    void draw_content()
    {
        toolbar_.on_draw();

        // only draw content if a simulation report is available
        const std::optional<SimulationReport> maybe_report = try_select_report_based_on_scrubbing();
        if (maybe_report) {
            shown_model_state_->set_simulation(simulation_);
            shown_model_state_->set_simulation_report(*maybe_report);

            OSC_PERF("draw simulation screen");
            panel_manager_->on_draw();
            status_bar_.on_draw();
            popup_manager_.on_draw();
        } else {
            ui::begin_panel("Waiting for simulation");
            ui::draw_text_disabled("(waiting for first simulation state)");
            ui::end_panel();

            // and show the log, so that the user can see any errors from the integrator (#628)
            //
            // this might be less necessary once the integrator correctly reports errors to
            // this UI panel (#625)
            LogViewerPanel p{&owner(), "Log"};
            p.on_draw();
        }
    }

    // underlying simulation being shown
    std::shared_ptr<Simulation> simulation_;

    // the modelstate that's being shown in the UI, based on scrubbing etc.
    //
    // if possible (i.e. there's a simulation report available), will be set each frame
    std::shared_ptr<SimulationModelStatePair> shown_model_state_ =
        std::make_shared<SimulationModelStatePair>();

    // scrubbing state
    SimulationUIPlaybackState playback_state_ = SimulationUIPlaybackState::Playing;
    SimulationUILoopingState looping_state_ = SimulationUILoopingState::PlayOnce;
    float playback_speed_ = 1.0f;
    SimulationClock::time_point playback_start_simtime_ = simulation_->get_start_time();
    std::chrono::steady_clock::time_point playback_start_wall_time_ =
        std::chrono::steady_clock::now();

    // manager for toggleable and spawnable UI panels
    std::shared_ptr<PanelManager> panel_manager_ = std::make_shared<PanelManager>(&owner());

    // non-toggleable UI panels/menus/toolbars
    SimulationTabMainMenu main_menu_{&owner(), simulation_, panel_manager_};
    SimulationToolbar toolbar_{"##SimulationToolbar", this, simulation_};
    ModelStatusBar status_bar_{&owner(), shown_model_state_};

    // manager for popups that are open in this tab
    PopupManager popup_manager_;
};

osc::SimulationTab::SimulationTab(Widget* parent, std::shared_ptr<Simulation> simulation) :

    Tab{std::make_unique<Impl>(*this, parent, std::move(simulation))}
{}
void osc::SimulationTab::impl_on_mount()
{
    private_data().on_mount();
}
void osc::SimulationTab::impl_on_unmount()
{
    private_data().on_unmount();
}
bool osc::SimulationTab::impl_on_event(Event& e)
{
    return private_data().on_event(e);
}
void osc::SimulationTab::impl_on_tick()
{
    private_data().on_tick();
}
void osc::SimulationTab::impl_on_draw_main_menu()
{
    private_data().on_draw_main_menu();
}
void osc::SimulationTab::impl_on_draw()
{
    private_data().on_draw();
}
