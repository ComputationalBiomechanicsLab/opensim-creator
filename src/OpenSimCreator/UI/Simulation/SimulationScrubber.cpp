#include "SimulationScrubber.h"

#include <OpenSimCreator/Documents/Simulation/Simulation.h>
#include <OpenSimCreator/Documents/Simulation/SimulationClock.h>
#include <OpenSimCreator/UI/Simulation/ISimulatorUIAPI.h>

#include <oscar/Platform/IconCodepoints.h>
#include <oscar/UI/oscimgui.h>
#include <oscar/Utils/EnumHelpers.h>

#include <memory>
#include <string>
#include <string_view>
#include <utility>

class osc::SimulationScrubber::Impl final {
public:
    Impl(
        std::string_view label,
        ISimulatorUIAPI* simulatorAPI,
        std::shared_ptr<const Simulation> simulation) :

        m_Label{label},
        m_SimulatorAPI{simulatorAPI},
        m_Simulation{std::move(simulation)}
    {}

    void onDraw()
    {
        drawBackwardsButtons();
        ui::same_line();

        drawPlayOrPauseOrReplayButton();
        ui::same_line();

        drawForwardsButtons();
        ui::same_line();

        drawLoopButton();
        ui::same_line();

        drawPlaybackSpeedSelector();
        ui::same_line();

        drawStartTimeText();
        ui::same_line();

        drawScrubber();
        ui::same_line();

        drawEndTimeText();

        // don't end with same_line, because this might be composed into
        // a multiline UI
    }

private:
    void drawBackwardsButtons()
    {
        if (ui::draw_button(OSC_ICON_FAST_BACKWARD))
        {
            m_SimulatorAPI->setSimulationScrubTime(m_Simulation->getStartTime());
        }
        ui::draw_tooltip_if_item_hovered("Go to First State");
        ui::same_line();

        if (ui::draw_button(OSC_ICON_STEP_BACKWARD))
        {
            m_SimulatorAPI->stepBack();
        }
        ui::draw_tooltip_if_item_hovered("Previous State");
    }

    void drawLoopButton()
    {
        static_assert(num_options<SimulationUILoopingState>() == 2);
        bool looping = m_SimulatorAPI->getSimulationLoopingState() == SimulationUILoopingState::Looping;
        if (ui::draw_checkbox("loop", &looping)) {
            if (looping) {
                m_SimulatorAPI->setSimulationLoopingState(SimulationUILoopingState::Looping);
            }
            else {
                m_SimulatorAPI->setSimulationLoopingState(SimulationUILoopingState::PlayOnce);
            }
        }
    }

    void drawPlayOrPauseOrReplayButton()
    {
        const SimulationClock::time_point tStart = m_Simulation->getStartTime();
        const SimulationClock::time_point tEnd = m_Simulation->getEndTime();
        const SimulationClock::time_point tCur = m_SimulatorAPI->getSimulationScrubTime();
        const SimulationUIPlaybackState state = m_SimulatorAPI->getSimulationPlaybackState();

        static_assert(num_options<SimulationUIPlaybackState>() == 2);
        if (state == SimulationUIPlaybackState::Playing) {
            // if playing, the only option is to stop

            if (ui::draw_button(OSC_ICON_PAUSE)) {
                m_SimulatorAPI->setSimulationPlaybackState(SimulationUIPlaybackState::Stopped);
            }
            ui::draw_tooltip_if_item_hovered("Pause (Space)");
        }
        else if (state == SimulationUIPlaybackState::Stopped) {
            // if stopped, show either a REDO button (i.e. re-run from the beginning) or
            // a PLAY button (i.e. un-pause)

            if (tCur >= tEnd) {
                if (ui::draw_button(OSC_ICON_REDO)) {
                    m_SimulatorAPI->setSimulationScrubTime(tStart);
                    m_SimulatorAPI->setSimulationPlaybackState(SimulationUIPlaybackState::Playing);
                }
                ui::draw_tooltip_if_item_hovered("Replay (Space)");
            }
            else {
                if (ui::draw_button(OSC_ICON_PLAY)) {
                    m_SimulatorAPI->setSimulationPlaybackState(SimulationUIPlaybackState::Playing);  // i.e. unpause
                }
                ui::draw_tooltip_if_item_hovered("Play (Space)");
            }
        }
    }

    void drawForwardsButtons()
    {
        if (ui::draw_button(OSC_ICON_STEP_FORWARD))
        {
            m_SimulatorAPI->stepForward();
        }
        ui::draw_tooltip_if_item_hovered("Next State");

        ui::same_line();

        if (ui::draw_button(OSC_ICON_FAST_FORWARD))
        {
            m_SimulatorAPI->setSimulationScrubTime(m_Simulation->getEndTime());
        }
        ui::draw_tooltip_if_item_hovered("Go to Last State");
    }

    void drawStartTimeText()
    {
        const SimulationClock::time_point tStart = m_Simulation->getStartTime();
        ui::draw_text_disabled("%.2f", static_cast<float>(tStart.time_since_epoch().count()));
    }

    void drawPlaybackSpeedSelector()
    {
        ui::set_next_item_width(ui::calc_text_size("0.000x").x + 2.0f*ui::get_style_frame_padding().x);
        float speed = m_SimulatorAPI->getSimulationPlaybackSpeed();
        if (ui::draw_float_input("speed", &speed, 0.0f, 0.0f, "%.3f", ui::TextInputFlag::EnterReturnsTrue))
        {
            m_SimulatorAPI->setSimulationPlaybackSpeed(speed);
        }
    }

    void drawScrubber()
    {
        const SimulationClock::time_point tStart = m_Simulation->getStartTime();
        const SimulationClock::time_point tEnd = m_Simulation->getEndTime();
        const SimulationClock::time_point tCur = m_SimulatorAPI->getSimulationScrubTime();

        ui::set_next_item_width(ui::get_font_size() * 20.0f);
        float v = static_cast<float>(tCur.time_since_epoch().count());
        const bool userScrubbed = ui::draw_float_slider("##scrubber",
            &v,
            static_cast<float>(tStart.time_since_epoch().count()),
            static_cast<float>(tEnd.time_since_epoch().count()),
            "%.2f",
            ui::SliderFlag::AlwaysClamp
        );
        ui::same_line();

        if (userScrubbed)
        {
            m_SimulatorAPI->setSimulationScrubTime(SimulationClock::start() + SimulationClock::duration{static_cast<double>(v)});
        }

        if (ui::is_item_hovered())
        {
            ui::begin_tooltip();
            ui::draw_text_unformatted("Left-Click: Change simulation time being shown");
            ui::draw_text_unformatted("Ctrl-Click: Type in the simulation time being shown");
            ui::end_tooltip();
        }
    }

    void drawEndTimeText()
    {
        const SimulationClock::time_point tEnd = m_Simulation->getEndTime();
        ui::draw_text_disabled("%.2f", static_cast<float>(tEnd.time_since_epoch().count()));
    }

    std::string m_Label;
    ISimulatorUIAPI* m_SimulatorAPI;
    std::shared_ptr<const Simulation> m_Simulation;
};


osc::SimulationScrubber::SimulationScrubber(
    std::string_view label,
    ISimulatorUIAPI* simulatorAPI,
    std::shared_ptr<const Simulation> simulation) :

    m_Impl{std::make_unique<Impl>(label, simulatorAPI, std::move(simulation))}
{}
osc::SimulationScrubber::SimulationScrubber(SimulationScrubber&&) noexcept = default;
osc::SimulationScrubber& osc::SimulationScrubber::operator=(SimulationScrubber&&) noexcept = default;
osc::SimulationScrubber::~SimulationScrubber() noexcept = default;

void osc::SimulationScrubber::onDraw()
{
    m_Impl->onDraw();
}
