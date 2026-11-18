#include "simulation_scrubber.h"

#include <libopensimcreator/documents/simulation/simulation.h>
#include <libopensimcreator/documents/simulation/simulation_clock.h>
#include <libopensimcreator/platform/msmicons.h>
#include <libopensimcreator/ui/simulation/simulator_ui_api.h>

#include <liboscar/ui/oscimgui.h>
#include <liboscar/utilities/enum_helpers.h>

#include <memory>
#include <string>
#include <string_view>
#include <utility>

class osc::SimulationScrubber::Impl final {
public:
    Impl(
        std::string_view label,
        SimulatorUIAPI* simulator_api,
        std::shared_ptr<const Simulation> simulation) :

        label_{label},
        simulator_api_{simulator_api},
        simulation_{std::move(simulation)}
    {}

    void on_draw()
    {
        draw_backwards_buttons();
        ui::same_line();

        draw_play_or_pause_or_replay_button();
        ui::same_line();

        draw_forwards_buttons();
        ui::same_line();

        draw_loop_button();
        ui::same_line();

        draw_playback_speed_selector();
        ui::same_line();

        draw_start_time_text();
        ui::same_line();

        draw_scrubber();
        ui::same_line();

        draw_end_time_text();

        // don't end with same_line, because this might be composed into
        // a multiline UI
    }

private:
    void draw_backwards_buttons()
    {
        if (ui::draw_button(MSMICONS_FAST_BACKWARD)) {
            simulator_api_->set_simulation_scrub_time(simulation_->get_start_time());
        }
        ui::draw_tooltip_if_item_hovered("Go to First State");
        ui::same_line();

        if (ui::draw_button(MSMICONS_STEP_BACKWARD)) {
            simulator_api_->step_back();
        }
        ui::draw_tooltip_if_item_hovered("Previous State");
    }

    void draw_loop_button()
    {
        static_assert(num_options_v<SimulationUILoopingState> == 2);
        bool looping =
            simulator_api_->get_simulation_looping_state() == SimulationUILoopingState::Looping;
        if (ui::draw_checkbox("loop", &looping)) {
            if (looping) {
                simulator_api_->set_simulation_looping_state(SimulationUILoopingState::Looping);
            } else {
                simulator_api_->set_simulation_looping_state(SimulationUILoopingState::PlayOnce);
            }
        }
    }

    void draw_play_or_pause_or_replay_button()
    {
        const SimulationClock::time_point t_start = simulation_->get_start_time();
        const SimulationClock::time_point t_end = simulation_->get_end_time();
        const SimulationClock::time_point t_cur = simulator_api_->get_simulation_scrub_time();
        const SimulationUIPlaybackState state = simulator_api_->get_simulation_playback_state();

        static_assert(num_options_v<SimulationUIPlaybackState> == 2);
        if (state == SimulationUIPlaybackState::Playing) {
            // if playing, the only option is to stop

            if (ui::draw_button(MSMICONS_PAUSE)) {
                simulator_api_->set_simulation_playback_state(SimulationUIPlaybackState::Stopped);
            }
            ui::draw_tooltip_if_item_hovered("Pause (Space)");
        } else if (state == SimulationUIPlaybackState::Stopped) {
            // if stopped, show either a REDO button (i.e. re-run from the beginning) or
            // a PLAY button (i.e. un-pause)

            if (t_cur >= t_end) {
                if (ui::draw_button(MSMICONS_REDO)) {
                    simulator_api_->set_simulation_scrub_time(t_start);
                    simulator_api_->set_simulation_playback_state(
                        SimulationUIPlaybackState::Playing);
                }
                ui::draw_tooltip_if_item_hovered("Replay (Space)");
            } else {
                if (ui::draw_button(MSMICONS_PLAY)) {
                    simulator_api_->set_simulation_playback_state(
                        SimulationUIPlaybackState::Playing);  // i.e. unpause
                }
                ui::draw_tooltip_if_item_hovered("Play (Space)");
            }
        }
    }

    void draw_forwards_buttons()
    {
        if (ui::draw_button(MSMICONS_STEP_FORWARD)) {
            simulator_api_->step_forward();
        }
        ui::draw_tooltip_if_item_hovered("Next State");

        ui::same_line();

        if (ui::draw_button(MSMICONS_FAST_FORWARD)) {
            simulator_api_->set_simulation_scrub_time(simulation_->get_end_time());
        }
        ui::draw_tooltip_if_item_hovered("Go to Last State");
    }

    void draw_start_time_text()
    {
        const SimulationClock::time_point t_start = simulation_->get_start_time();
        ui::draw_text_disabled("{:.2f}", t_start.time_since_epoch().count());
    }

    void draw_playback_speed_selector()
    {
        ui::set_next_item_width(
            ui::calc_text_size("0.000x").x() + 2.0f * ui::get_style_frame_padding().x());
        float speed = simulator_api_->get_simulation_playback_speed();
        ui::draw_float_input("speed", &speed, 0.0f, 0.0f, "%.3f");
        if (ui::is_item_deactivated_after_edit()) {
            simulator_api_->set_simulation_playback_speed(speed);
        }
    }

    void draw_scrubber()
    {
        const SimulationClock::time_point t_start = simulation_->get_start_time();
        const SimulationClock::time_point t_end = simulation_->get_end_time();
        const SimulationClock::time_point t_cur = simulator_api_->get_simulation_scrub_time();

        ui::set_next_item_width(ui::get_font_base_size() * 20.0f);
        float v = static_cast<float>(t_cur.time_since_epoch().count());
        const bool user_scrubbed = ui::draw_float_circular_slider(
            "##scrubber",
            &v,
            static_cast<float>(t_start.time_since_epoch().count()),
            static_cast<float>(t_end.time_since_epoch().count()),
            "%.2f",
            ui::SliderFlag::AlwaysClamp);
        ui::same_line();

        if (user_scrubbed) {
            simulator_api_->set_simulation_scrub_time(
                SimulationClock::start() + SimulationClock::duration{static_cast<double>(v)});
        }

        if (ui::is_item_hovered()) {
            ui::begin_tooltip();
            ui::draw_text("Left-Click: Change simulation time being shown");
            ui::draw_text("Ctrl-Click: Type in the simulation time being shown");
            ui::end_tooltip();
        }
    }

    void draw_end_time_text()
    {
        const SimulationClock::time_point t_end = simulation_->get_end_time();
        ui::draw_text_disabled("{:.2f}", t_end.time_since_epoch().count());
    }

    std::string label_;
    SimulatorUIAPI* simulator_api_;
    std::shared_ptr<const Simulation> simulation_;
};

osc::SimulationScrubber::SimulationScrubber(
    std::string_view label,
    SimulatorUIAPI* simulator_api,
    std::shared_ptr<const Simulation> simulation) :

    impl_{std::make_unique<Impl>(label, simulator_api, std::move(simulation))}
{}
osc::SimulationScrubber::SimulationScrubber(SimulationScrubber&&) noexcept = default;
osc::SimulationScrubber& osc::SimulationScrubber::operator=(SimulationScrubber&&) noexcept =
    default;
osc::SimulationScrubber::~SimulationScrubber() noexcept = default;
void osc::SimulationScrubber::on_draw()
{
    impl_->on_draw();
}
