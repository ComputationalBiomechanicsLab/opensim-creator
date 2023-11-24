#include "SimulationToolbar.hpp"

#include <OpenSimCreator/Documents/Simulation/Simulation.hpp>
#include <OpenSimCreator/UI/Widgets/BasicWidgets.hpp>
#include <OpenSimCreator/UI/Widgets/SimulationScrubber.hpp>
#include <OpenSimCreator/Utils/OpenSimHelpers.hpp>

#include <IconsFontAwesome5.h>
#include <imgui.h>
#include <imgui_internal.h>
#include <oscar/Bindings/ImGuiHelpers.hpp>
#include <oscar/Graphics/Color.hpp>
#include <oscar/Maths/Vec2.hpp>
#include <oscar/Maths/Vec4.hpp>

#include <memory>
#include <string>
#include <string_view>
#include <utility>

namespace
{
    osc::Color CalcStatusColor(osc::SimulationStatus status)
    {
        switch (status)
        {
        case osc::SimulationStatus::Initializing:
        case osc::SimulationStatus::Running:
            return osc::Color::mutedBlue();
        case osc::SimulationStatus::Completed:
            return osc::Color::darkGreen();
        case osc::SimulationStatus::Cancelled:
        case osc::SimulationStatus::Error:
            return osc::Color::red();
        default:
            return osc::Color{osc::Vec4{ImGui::GetStyle().Colors[ImGuiCol_Text]}};
        }
    }
}


class osc::SimulationToolbar::Impl final {
public:
    Impl(
        std::string_view label,
        SimulatorUIAPI* simulatorAPI,
        std::shared_ptr<Simulation> simulation) :

        m_Label{label},
        m_SimulatorAPI{simulatorAPI},
        m_Simulation{std::move(simulation)}
    {
    }

    void onDraw()
    {
        if (BeginToolbar(m_Label, Vec2{5.0f, 5.0f}))
        {
            drawContent();
        }
        ImGui::End();
    }

private:
    void drawContent()
    {
        drawScaleFactorGroup();

        ImGui::SameLine();
        ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
        ImGui::SameLine();

        m_Scrubber.onDraw();

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
    }

    void drawSimulationStatusGroup()
    {
        SimulationStatus const status = m_Simulation->getStatus();
        ImGui::TextDisabled("simulator status:");
        ImGui::SameLine();
        osc::PushStyleColor(ImGuiCol_Text, CalcStatusColor(status));
        ImGui::TextUnformatted(GetAllSimulationStatusStrings()[static_cast<size_t>(status)].c_str());
        osc::PopStyleColor();
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

    m_Impl{std::make_unique<Impl>(label, simulatorAPI, std::move(simulation))}
{
}
osc::SimulationToolbar::SimulationToolbar(SimulationToolbar&&) noexcept = default;
osc::SimulationToolbar& osc::SimulationToolbar::operator=(SimulationToolbar&&) noexcept = default;
osc::SimulationToolbar::~SimulationToolbar() noexcept = default;

void osc::SimulationToolbar::onDraw()
{
    m_Impl->onDraw();
}
