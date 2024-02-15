#include "SimulationScrubber.h"

#include <OpenSimCreator/Documents/Simulation/Simulation.h>
#include <OpenSimCreator/Documents/Simulation/SimulationClock.h>
#include <OpenSimCreator/UI/Simulation/ISimulatorUIAPI.h>

#include <IconsFontAwesome5.h>
#include <oscar/UI/oscimgui.h>
#include <oscar/UI/ImGuiHelpers.h>

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
        ImGui::SameLine();

        drawPlayOrPauseOrReplayButton();
        ImGui::SameLine();

        drawForwardsButtons();
        ImGui::SameLine();

        drawPlaybackSpeedSelector();
        ImGui::SameLine();

        drawStartTimeText();
        ImGui::SameLine();

        drawScrubber();
        ImGui::SameLine();

        drawEndTimeText();

        // don't end with SameLine, because this might be composed into
        // a multiline UI
    }

private:
    void drawBackwardsButtons()
    {
        if (ImGui::Button(ICON_FA_FAST_BACKWARD))
        {
            m_SimulatorAPI->setSimulationScrubTime(m_Simulation->getStartTime());
        }
        DrawTooltipIfItemHovered("Go to First State");
        ImGui::SameLine();

        if (ImGui::Button(ICON_FA_STEP_BACKWARD))
        {
            m_SimulatorAPI->stepBack();
        }
        DrawTooltipIfItemHovered("Previous State");
    }

    void drawPlayOrPauseOrReplayButton()
    {
        SimulationClock::time_point const tStart = m_Simulation->getStartTime();
        SimulationClock::time_point const tEnd = m_Simulation->getEndTime();
        SimulationClock::time_point const tCur = m_SimulatorAPI->getSimulationScrubTime();

        // play/pause
        if (tCur >= tEnd)
        {
            if (ImGui::Button(ICON_FA_REDO))
            {
                m_SimulatorAPI->setSimulationScrubTime(tStart);
                m_SimulatorAPI->setSimulationPlaybackState(true);
            }
            DrawTooltipIfItemHovered("Replay");
        }
        else if (!m_SimulatorAPI->getSimulationPlaybackState())
        {
            if (ImGui::Button(ICON_FA_PLAY))
            {
                m_SimulatorAPI->setSimulationPlaybackState(true);
            }
            DrawTooltipIfItemHovered("Play");
        }
        else
        {
            if (ImGui::Button(ICON_FA_PAUSE))
            {
                m_SimulatorAPI->setSimulationPlaybackState(false);
            }
            DrawTooltipIfItemHovered("Pause");
        }
    }

    void drawForwardsButtons()
    {
        if (ImGui::Button(ICON_FA_STEP_FORWARD))
        {
            m_SimulatorAPI->stepForward();
        }
        DrawTooltipIfItemHovered("Next State");

        ImGui::SameLine();

        if (ImGui::Button(ICON_FA_FAST_FORWARD))
        {
            m_SimulatorAPI->setSimulationScrubTime(m_Simulation->getEndTime());
        }
        DrawTooltipIfItemHovered("Go to Last State");
    }

    void drawStartTimeText()
    {
        SimulationClock::time_point const tStart = m_Simulation->getStartTime();
        ImGui::TextDisabled("%.2f", static_cast<float>(tStart.time_since_epoch().count()));
    }

    void drawPlaybackSpeedSelector()
    {
        ImGui::SetNextItemWidth(ImGui::CalcTextSize("0.000x").x + 2.0f*ImGui::GetStyle().FramePadding.x);
        float speed = m_SimulatorAPI->getSimulationPlaybackSpeed();
        if (ImGui::InputFloat("speed", &speed, 0.0f, 0.0f, "%.3f", ImGuiInputTextFlags_EnterReturnsTrue))
        {
            m_SimulatorAPI->setSimulationPlaybackSpeed(speed);
        }
    }

    void drawScrubber()
    {
        SimulationClock::time_point const tStart = m_Simulation->getStartTime();
        SimulationClock::time_point const tEnd = m_Simulation->getEndTime();
        SimulationClock::time_point const tCur = m_SimulatorAPI->getSimulationScrubTime();

        ImGui::SetNextItemWidth(ImGui::GetFontSize() * 20.0f);
        float v = static_cast<float>(tCur.time_since_epoch().count());
        bool const userScrubbed = ImGui::SliderFloat("##scrubber",
            &v,
            static_cast<float>(tStart.time_since_epoch().count()),
            static_cast<float>(tEnd.time_since_epoch().count()),
            "%.2f",
            ImGuiSliderFlags_AlwaysClamp
        );
        ImGui::SameLine();

        if (userScrubbed)
        {
            m_SimulatorAPI->setSimulationScrubTime(SimulationClock::start() + SimulationClock::duration{static_cast<double>(v)});
        }

        if (ImGui::IsItemHovered())
        {
            BeginTooltip();
            ImGui::TextUnformatted("Left-Click: Change simulation time being shown");
            ImGui::TextUnformatted("Ctrl-Click: Type in the simulation time being shown");
            EndTooltip();
        }
    }

    void drawEndTimeText()
    {
        SimulationClock::time_point const tEnd = m_Simulation->getEndTime();
        ImGui::TextDisabled("%.2f", static_cast<float>(tEnd.time_since_epoch().count()));
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
