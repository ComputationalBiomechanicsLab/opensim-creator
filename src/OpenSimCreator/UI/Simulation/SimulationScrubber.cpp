#include "SimulationScrubber.h"

#include <OpenSimCreator/Documents/Simulation/Simulation.h>
#include <OpenSimCreator/Documents/Simulation/SimulationClock.h>
#include <OpenSimCreator/UI/Simulation/ISimulatorUIAPI.h>

#include <IconsFontAwesome5.h>
#include <oscar/UI/oscimgui.h>
#include <oscar/UI/ImGuiHelpers.h>
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
        std::shared_ptr<Simulation const> simulation) :

        m_Label{label},
        m_SimulatorAPI{simulatorAPI},
        m_Simulation{std::move(simulation)}
    {
    }

    void onDraw()
    {
        drawBackwardsButtons();
        ui::SameLine();

        drawPlayOrPauseOrReplayButton();
        ui::SameLine();

        drawForwardsButtons();
        ui::SameLine();

        drawLoopButton();
        ui::SameLine();

        drawPlaybackSpeedSelector();
        ui::SameLine();

        drawStartTimeText();
        ui::SameLine();

        drawScrubber();
        ui::SameLine();

        drawEndTimeText();

        // don't end with SameLine, because this might be composed into
        // a multiline UI
    }

private:
    void drawBackwardsButtons()
    {
        if (ui::Button(ICON_FA_FAST_BACKWARD))
        {
            m_SimulatorAPI->setSimulationScrubTime(m_Simulation->getStartTime());
        }
        ui::DrawTooltipIfItemHovered("Go to First State");
        ui::SameLine();

        if (ui::Button(ICON_FA_STEP_BACKWARD))
        {
            m_SimulatorAPI->stepBack();
        }
        ui::DrawTooltipIfItemHovered("Previous State");
    }

    void drawLoopButton()
    {
        static_assert(NumOptions<SimulationUILoopingState>() == 2);
        bool looping = m_SimulatorAPI->getSimulationLoopingState() == SimulationUILoopingState::Looping;
        if (ui::Checkbox("loop", &looping)) {
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
        SimulationClock::time_point const tStart = m_Simulation->getStartTime();
        SimulationClock::time_point const tEnd = m_Simulation->getEndTime();
        SimulationClock::time_point const tCur = m_SimulatorAPI->getSimulationScrubTime();
        SimulationUIPlaybackState const state = m_SimulatorAPI->getSimulationPlaybackState();

        static_assert(NumOptions<SimulationUIPlaybackState>() == 2);
        if (state == SimulationUIPlaybackState::Playing) {
            // if playing, the only option is to stop

            if (ui::Button(ICON_FA_PAUSE)) {
                m_SimulatorAPI->setSimulationPlaybackState(SimulationUIPlaybackState::Stopped);
            }
            ui::DrawTooltipIfItemHovered("Pause");
        }
        else if (state == SimulationUIPlaybackState::Stopped) {
            // if stopped, show either a REDO button (i.e. re-run from the beginning) or
            // a PLAY button (i.e. un-pause)

            if (tCur >= tEnd) {
                if (ui::Button(ICON_FA_REDO)) {
                    m_SimulatorAPI->setSimulationScrubTime(tStart);
                    m_SimulatorAPI->setSimulationPlaybackState(SimulationUIPlaybackState::Playing);
                }
                ui::DrawTooltipIfItemHovered("Replay");
            }
            else {
                if (ui::Button(ICON_FA_PLAY)) {
                    m_SimulatorAPI->setSimulationPlaybackState(SimulationUIPlaybackState::Playing);  // i.e. unpause
                }
                ui::DrawTooltipIfItemHovered("Play");
            }
        }
    }

    void drawForwardsButtons()
    {
        if (ui::Button(ICON_FA_STEP_FORWARD))
        {
            m_SimulatorAPI->stepForward();
        }
        ui::DrawTooltipIfItemHovered("Next State");

        ui::SameLine();

        if (ui::Button(ICON_FA_FAST_FORWARD))
        {
            m_SimulatorAPI->setSimulationScrubTime(m_Simulation->getEndTime());
        }
        ui::DrawTooltipIfItemHovered("Go to Last State");
    }

    void drawStartTimeText()
    {
        SimulationClock::time_point const tStart = m_Simulation->getStartTime();
        ui::TextDisabled("%.2f", static_cast<float>(tStart.time_since_epoch().count()));
    }

    void drawPlaybackSpeedSelector()
    {
        ui::SetNextItemWidth(ui::CalcTextSize("0.000x").x + 2.0f*ui::GetStyleFramePadding().x);
        float speed = m_SimulatorAPI->getSimulationPlaybackSpeed();
        if (ui::InputFloat("speed", &speed, 0.0f, 0.0f, "%.3f", ImGuiInputTextFlags_EnterReturnsTrue))
        {
            m_SimulatorAPI->setSimulationPlaybackSpeed(speed);
        }
    }

    void drawScrubber()
    {
        SimulationClock::time_point const tStart = m_Simulation->getStartTime();
        SimulationClock::time_point const tEnd = m_Simulation->getEndTime();
        SimulationClock::time_point const tCur = m_SimulatorAPI->getSimulationScrubTime();

        ui::SetNextItemWidth(ui::GetFontSize() * 20.0f);
        float v = static_cast<float>(tCur.time_since_epoch().count());
        bool const userScrubbed = ui::SliderFloat("##scrubber",
            &v,
            static_cast<float>(tStart.time_since_epoch().count()),
            static_cast<float>(tEnd.time_since_epoch().count()),
            "%.2f",
            ImGuiSliderFlags_AlwaysClamp
        );
        ui::SameLine();

        if (userScrubbed)
        {
            m_SimulatorAPI->setSimulationScrubTime(SimulationClock::start() + SimulationClock::duration{static_cast<double>(v)});
        }

        if (ui::IsItemHovered())
        {
            ui::BeginTooltip();
            ui::TextUnformatted("Left-Click: Change simulation time being shown");
            ui::TextUnformatted("Ctrl-Click: Type in the simulation time being shown");
            ui::EndTooltip();
        }
    }

    void drawEndTimeText()
    {
        SimulationClock::time_point const tEnd = m_Simulation->getEndTime();
        ui::TextDisabled("%.2f", static_cast<float>(tEnd.time_since_epoch().count()));
    }

    std::string m_Label;
    ISimulatorUIAPI* m_SimulatorAPI;
    std::shared_ptr<Simulation const> m_Simulation;
};


// public API (PIMPL)

osc::SimulationScrubber::SimulationScrubber(
    std::string_view label,
    ISimulatorUIAPI* simulatorAPI,
    std::shared_ptr<Simulation const> simulation) :

    m_Impl{std::make_unique<Impl>(label, simulatorAPI, std::move(simulation))}
{
}

osc::SimulationScrubber::SimulationScrubber(SimulationScrubber&&) noexcept = default;
osc::SimulationScrubber& osc::SimulationScrubber::operator=(SimulationScrubber&&) noexcept = default;
osc::SimulationScrubber::~SimulationScrubber() noexcept = default;

void osc::SimulationScrubber::onDraw()
{
    m_Impl->onDraw();
}
