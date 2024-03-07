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
            return Color::muted_blue();
        case SimulationStatus::Completed:
            return Color::dark_green();
        case SimulationStatus::Cancelled:
        case SimulationStatus::Error:
            return Color::red();
        default:
            return Color{Vec4{ui::GetStyle().Colors[ImGuiCol_Text]}};
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
        ui::End();
    }

private:
    void drawContent()
    {
        drawScaleFactorGroup();

        ui::SameLine();
        ui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
        ui::SameLine();

        m_Scrubber.onDraw();

        ui::SameLine();
        ui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
        ui::SameLine();

        drawSimulationStatusGroup();
    }

    void drawScaleFactorGroup()
    {
        ui::PushStyleVar(ImGuiStyleVar_ItemSpacing, {0.0f, 0.0f});
        ui::TextUnformatted(ICON_FA_EXPAND_ALT);
        ui::DrawTooltipIfItemHovered("Scene Scale Factor", "Rescales decorations in the model by this amount. Changing this can be handy when working on extremely small/large models.");
        ui::SameLine();

        {
            float scaleFactor = m_Simulation->getFixupScaleFactor();
            ui::SetNextItemWidth(ui::CalcTextSize("0.00000").x);
            if (ui::InputFloat("##scaleinput", &scaleFactor))
            {
                m_Simulation->setFixupScaleFactor(scaleFactor);
            }
        }
        ui::PopStyleVar();
    }

    void drawSimulationStatusGroup()
    {
        SimulationStatus const status = m_Simulation->getStatus();
        ui::TextDisabled("simulator status:");
        ui::SameLine();
        ui::PushStyleColor(ImGuiCol_Text, CalcStatusColor(status));
        ui::TextUnformatted(GetAllSimulationStatusStrings()[static_cast<size_t>(status)]);
        ui::PopStyleColor();
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
