#include "SimulationToolbar.h"

#include <OpenSimCreator/Documents/Simulation/Simulation.h>
#include <OpenSimCreator/UI/Shared/BasicWidgets.h>
#include <OpenSimCreator/UI/Simulation/SimulationScrubber.h>
#include <OpenSimCreator/Utils/OpenSimHelpers.h>

#include <IconsFontAwesome5.h>
#include <oscar/Graphics/Color.h>
#include <oscar/Maths/Vec2.h>
#include <oscar/Maths/Vec4.h>
#include <oscar/UI/ImGuiHelpers.h>
#include <oscar/UI/oscimgui.h>
#include <oscar/UI/oscimgui_internal.h>

#include <memory>
#include <string>
#include <string_view>
#include <utility>

using namespace osc;

namespace
{
    Color CalcStatusColor(SimulationStatus status)
    {
        switch (status)
        {
        case SimulationStatus::Initializing:
        case SimulationStatus::Running:
            return Color::mutedBlue();
        case SimulationStatus::Completed:
            return Color::darkGreen();
        case SimulationStatus::Cancelled:
        case SimulationStatus::Error:
            return Color::red();
        default:
            return Color{Vec4{ImGui::GetStyle().Colors[ImGuiCol_Text]}};
        }
    }
}


class osc::SimulationToolbar::Impl final {
public:
    Impl(
        std::string_view label,
        ISimulatorUIAPI* simulatorAPI,
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
        ui::TextUnformatted(ICON_FA_EXPAND_ALT);
        DrawTooltipIfItemHovered("Scene Scale Factor", "Rescales decorations in the model by this amount. Changing this can be handy when working on extremely small/large models.");
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
        ui::TextDisabled("simulator status:");
        ImGui::SameLine();
        PushStyleColor(ImGuiCol_Text, CalcStatusColor(status));
        ui::TextUnformatted(GetAllSimulationStatusStrings()[static_cast<size_t>(status)]);
        PopStyleColor();
    }

    std::string m_Label;
    ISimulatorUIAPI* m_SimulatorAPI;
    std::shared_ptr<Simulation> m_Simulation;

    SimulationScrubber m_Scrubber{"##SimulationScrubber", m_SimulatorAPI, m_Simulation};
};


// public API (PIMPL)

osc::SimulationToolbar::SimulationToolbar(
    std::string_view label,
    ISimulatorUIAPI* simulatorAPI,
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
