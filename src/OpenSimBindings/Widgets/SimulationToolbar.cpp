#include "SimulationToolbar.hpp"

#include "src/Bindings/ImGuiHelpers.hpp"
#include "src/OpenSimBindings/Widgets/SimulationScrubber.hpp"
#include "src/OpenSimBindings/Simulation.hpp"

#include <imgui.h>

#include <memory>
#include <string>
#include <string_view>
#include <utility>

class osc::SimulationToolbar::Impl final {
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
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {5.0f, 5.0f});
        float const height = ImGui::GetFrameHeight() + 2.0f*ImGui::GetStyle().WindowPadding.y;
        ImGuiWindowFlags const flags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings;
        if (osc::BeginMainViewportTopBar(m_Label, height, flags))
        {
            drawContent();
        }
        ImGui::End();
        ImGui::PopStyleVar();
    }

private:
    void drawContent()
    {
        m_Scrubber.draw();
    }

    std::string m_Label;
    SimulatorUIAPI* m_SimulatorAPI;
    std::shared_ptr<Simulation> m_Simulation;

    SimulationScrubber m_Scrubber{"##SimulationScrubber", m_SimulatorAPI, m_Simulation};
};


// public API (PIMPL)

osc::SimulationToolbar::SimulationToolbar(
    std::string_view label,
    SimulatorUIAPI* simulatorAPI,
    std::shared_ptr<Simulation> simulation) :

    m_Impl{std::make_unique<Impl>(std::move(label), std::move(simulatorAPI), std::move(simulation))}
{
}
osc::SimulationToolbar::SimulationToolbar(SimulationToolbar&&) noexcept = default;
osc::SimulationToolbar& osc::SimulationToolbar::operator=(SimulationToolbar&&) noexcept = default;
osc::SimulationToolbar::~SimulationToolbar() noexcept = default;

void osc::SimulationToolbar::draw()
{
    m_Impl->draw();
}