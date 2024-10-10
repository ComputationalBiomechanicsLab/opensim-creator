#include "SimulationToolbar.h"

#include <OpenSimCreator/Documents/Simulation/Simulation.h>
#include <OpenSimCreator/UI/Shared/BasicWidgets.h>
#include <OpenSimCreator/UI/Simulation/SimulationScrubber.h>
#include <OpenSimCreator/Utils/OpenSimHelpers.h>

#include <oscar/Graphics/Color.h>
#include <oscar/Maths/Vec2.h>
#include <oscar/Maths/Vec4.h>
#include <oscar/Platform/IconCodepoints.h>
#include <oscar/UI/oscimgui.h>

#include <memory>
#include <string>
#include <string_view>
#include <utility>

using namespace osc;

namespace
{
    Color CalcStatusColor(SimulationStatus status)
    {
        switch (status) {
        case SimulationStatus::Initializing:
        case SimulationStatus::Running:
            return Color::muted_blue();
        case SimulationStatus::Completed:
            return Color::dark_green();
        case SimulationStatus::Cancelled:
        case SimulationStatus::Error:
            return Color::red();
        default:
            return ui::get_style_color(ui::ColorVar::Text);
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
    {}

    void onDraw()
    {
        if (BeginToolbar(m_Label, Vec2{5.0f, 5.0f})) {
            drawContent();
        }
        ui::end_panel();
    }

private:
    void drawContent()
    {
        drawScaleFactorGroup();

        ui::same_line();
        ui::draw_vertical_separator();
        ui::same_line();

        m_Scrubber.onDraw();

        ui::same_line();
        ui::draw_vertical_separator();
        ui::same_line();

        drawSimulationStatusGroup();
    }

    void drawScaleFactorGroup()
    {
        ui::push_style_var(ui::StyleVar::ItemSpacing, {0.0f, 0.0f});
        ui::draw_text_unformatted(OSC_ICON_EXPAND_ALT);
        ui::draw_tooltip_if_item_hovered("Scene Scale Factor", "Rescales decorations in the model by this amount. Changing this can be handy when working on extremely small/large models.");
        ui::same_line();

        {
            float scaleFactor = m_Simulation->getFixupScaleFactor();
            ui::set_next_item_width(ui::calc_text_size("0.00000").x);
            if (ui::draw_float_input("##scaleinput", &scaleFactor))
            {
                m_Simulation->setFixupScaleFactor(scaleFactor);
            }
        }
        ui::pop_style_var();
    }

    void drawSimulationStatusGroup()
    {
        const SimulationStatus status = m_Simulation->getStatus();
        ui::draw_text_disabled("simulator status:");
        ui::same_line();
        ui::push_style_color(ui::ColorVar::Text, CalcStatusColor(status));
        ui::draw_text_unformatted(GetAllSimulationStatusStrings()[static_cast<size_t>(status)]);
        ui::pop_style_color();
    }

    std::string m_Label;
    ISimulatorUIAPI* m_SimulatorAPI;
    std::shared_ptr<Simulation> m_Simulation;

    SimulationScrubber m_Scrubber{"##SimulationScrubber", m_SimulatorAPI, m_Simulation};
};


osc::SimulationToolbar::SimulationToolbar(
    std::string_view label,
    ISimulatorUIAPI* simulatorAPI,
    std::shared_ptr<Simulation> simulation) :

    m_Impl{std::make_unique<Impl>(label, simulatorAPI, std::move(simulation))}
{}
osc::SimulationToolbar::SimulationToolbar(SimulationToolbar&&) noexcept = default;
osc::SimulationToolbar& osc::SimulationToolbar::operator=(SimulationToolbar&&) noexcept = default;
osc::SimulationToolbar::~SimulationToolbar() noexcept = default;
void osc::SimulationToolbar::onDraw() { m_Impl->onDraw(); }
