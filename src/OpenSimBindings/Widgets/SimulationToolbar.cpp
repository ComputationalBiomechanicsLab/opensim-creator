#include "SimulationToolbar.hpp"

#include "src/Bindings/ImGuiHelpers.hpp"
#include "src/OpenSimBindings/Widgets/SimulationScrubber.hpp"
#include "src/OpenSimBindings/OpenSimHelpers.hpp"
#include "src/OpenSimBindings/Simulation.hpp"
#include "src/Platform/Styling.hpp"

#include <IconsFontAwesome5.h>
#include <imgui.h>
#include <imgui_internal.h>

#include <memory>
#include <string>
#include <string_view>
#include <utility>

namespace
{
    glm::vec4 CalcStatusColor(osc::SimulationStatus status)
    {
        switch (status)
        {
        case osc::SimulationStatus::Initializing:
        case osc::SimulationStatus::Running:
            return OSC_NEUTRAL_RGBA;
        case osc::SimulationStatus::Completed:
            return OSC_POSITIVE_RGBA;
        case osc::SimulationStatus::Cancelled:
        case osc::SimulationStatus::Error:
            return OSC_NEGATIVE_RGBA;
        default:
            return ImGui::GetStyle().Colors[ImGuiCol_Text];
        }
    }
}


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
        drawScaleFactorGroup();

        ImGui::SameLine();
        ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
        ImGui::SameLine();

        m_Scrubber.draw();

        ImGui::SameLine();
        ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
        ImGui::SameLine();

        drawSimulationStatusGroup();
    }

    void drawScaleFactorGroup()
    {
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, {0.0f, 0.0f});
        ImGui::TextUnformatted(ICON_FA_EXPAND_ALT);
        osc::DrawTooltipIfItemHovered("Scene Scale Factor", "Rescales decorations in the model by this amount. Changing this can be handy when working on extremely small/large models.");
        ImGui::SameLine();

        {
            float scaleFactor = m_Simulation->getFixupScaleFactor();
            ImGui::SetNextItemWidth(ImGui::CalcTextSize("0.00000").x);
            if (ImGui::InputFloat("##scaleinput", &scaleFactor))
            {
                m_Simulation->setFixupScaleFactor(scaleFactor);
            }
        }
        ImGui::PopStyleVar();

        // TODO: can't autoscale without a state
    }

    void drawSimulationStatusGroup()
    {
        SimulationStatus const status = m_Simulation->getStatus();
        ImGui::TextDisabled("simulator status:");
        ImGui::SameLine();
        ImGui::PushStyleColor(ImGuiCol_Text, CalcStatusColor(status));
        ImGui::TextUnformatted(GetAllSimulationStatusStrings()[static_cast<size_t>(status)]);
        ImGui::PopStyleColor();
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