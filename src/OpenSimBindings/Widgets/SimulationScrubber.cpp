#include "SimulationScrubber.hpp"

#include "src/OpenSimBindings/MiddlewareAPIs/SimulatorUIAPI.hpp"
#include "src/OpenSimBindings/Simulation.hpp"
#include "src/OpenSimBindings/SimulationClock.hpp"

#include <imgui.h>
#include <IconsFontAwesome5.h>

#include <memory>
#include <string>
#include <string_view>
#include <utility>

class osc::SimulationScrubber::Impl final {
public:

    Impl(
        std::string_view label,
        SimulatorUIAPI* simulatorAPI,
        std::shared_ptr<Simulation> simulation) :

        m_Label{std::move(label)},
        m_SimulatorAPI{std::move(simulatorAPI)},
        m_Simulation{std::move(simulation)}
    {
    }

    void draw()
    {
        SimulationClock::time_point const tStart = m_Simulation->getStartTime();
        SimulationClock::time_point const tEnd = m_Simulation->getEndTime();
        SimulationClock::time_point const tCur = m_SimulatorAPI->getSimulationScrubTime();

        // play/pause buttons
        if (tCur >= tEnd)
        {
            if (ImGui::Button(ICON_FA_REDO))
            {
                m_SimulatorAPI->setSimulationScrubTime(tStart);
                m_SimulatorAPI->setSimulationPlaybackState(true);
            }
        }
        else if (!m_SimulatorAPI->getSimulationPlaybackState())
        {
            if (ImGui::Button(ICON_FA_PLAY))
            {
                m_SimulatorAPI->setSimulationPlaybackState(true);
            }
        }
        else
        {
            if (ImGui::Button(ICON_FA_PAUSE))
            {
                m_SimulatorAPI->setSimulationPlaybackState(false);
            }
        }

        // scubber slider
        ImGui::SameLine();
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);

        float v = static_cast<float>(tCur.time_since_epoch().count());
        bool const userScrubbed = ImGui::SliderFloat("##scrubber",
            &v,
            static_cast<float>(tStart.time_since_epoch().count()),
            static_cast<float>(tEnd.time_since_epoch().count()),
            "%.2f",
            ImGuiSliderFlags_AlwaysClamp
        );

        if (userScrubbed)
        {
            m_SimulatorAPI->setSimulationScrubTime(SimulationClock::start() + SimulationClock::duration{static_cast<double>(v)});
        }

        if (ImGui::IsItemHovered())
        {
            ImGui::BeginTooltip();
            ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
            ImGui::TextUnformatted("Left-Click: Change simulation time being shown");
            ImGui::TextUnformatted("Ctrl-Click: Type in the simulation time being shown");
            ImGui::PopTextWrapPos();
            ImGui::EndTooltip();
        }
    }

    bool onEvent(SDL_Event const&)
    {
        return false;
    }

private:
    std::string m_Label;
    SimulatorUIAPI* m_SimulatorAPI;
    std::shared_ptr<Simulation> m_Simulation;
};


// public API (PIMPL)

osc::SimulationScrubber::SimulationScrubber(
    std::string_view label,
    SimulatorUIAPI* simulatorAPI,
    std::shared_ptr<Simulation> simulation) :

    m_Impl{std::make_unique<Impl>(std::move(label), std::move(simulatorAPI), std::move(simulation))}
{
}

osc::SimulationScrubber::SimulationScrubber(SimulationScrubber&&) noexcept = default;
osc::SimulationScrubber& osc::SimulationScrubber::operator=(SimulationScrubber&&) noexcept = default;
osc::SimulationScrubber::~SimulationScrubber() noexcept = default;

void osc::SimulationScrubber::draw()
{
    m_Impl->draw();
}
