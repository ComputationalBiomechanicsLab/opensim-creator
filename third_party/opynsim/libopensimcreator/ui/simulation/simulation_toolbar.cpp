#include "simulation_toolbar.h"

#include <libopensimcreator/documents/simulation/simulation.h>
#include <libopensimcreator/platform/msmicons.h>
#include <libopensimcreator/ui/shared/basic_widgets.h>
#include <libopensimcreator/ui/simulation/simulation_scrubber.h>

#include <libopynsim/utilities/open_sim_helpers.h>
#include <liboscar/graphics/color.h>
#include <liboscar/maths/vector.h>
#include <liboscar/ui/oscimgui.h>

#include <memory>
#include <string>
#include <string_view>
#include <utility>

using namespace osc;

namespace
{
    Color calc_status_color(SimulationStatus status)
    {
        switch (status) {
        case SimulationStatus::Initializing: [[fallthrough]];
        case SimulationStatus::Running: return Color::muted_blue();
        case SimulationStatus::Completed: return Color::dark_green();
        case SimulationStatus::Cancelled: [[fallthrough]];
        case SimulationStatus::Error: return Color::red();
        default: return ui::get_style_color(ui::ColorVar::Text);
        }
    }
}

class osc::SimulationToolbar::Impl final {
public:
    Impl(
        std::string_view label,
        SimulatorUIAPI* simulator_api,
        std::shared_ptr<Simulation> simulation) :

        label_{label},
        simulator_api_{simulator_api},
        simulation_{std::move(simulation)}
    {}

    void on_draw()
    {
        if (begin_toolbar(label_, Vector2{5.0f, 5.0f})) {
            draw_content();
        }
        ui::end_panel();
    }

private:
    void draw_content()
    {
        draw_scale_factor_group();

        ui::same_line();
        ui::draw_vertical_separator();
        ui::same_line();

        scrubber_.on_draw();

        ui::same_line();
        ui::draw_vertical_separator();
        ui::same_line();

        draw_simulation_status_group();
    }

    void draw_scale_factor_group()
    {
        ui::push_style_var(ui::StyleVar::ItemSpacing, {0.0f, 0.0f});
        ui::draw_text(MSMICONS_EXPAND_ALT);
        ui::draw_tooltip_if_item_hovered(
            "Scene Scale Factor",
            "Rescales decorations in the model by this amount. Changing this can be handy when working on extremely small/large models.");
        ui::same_line();

        {
            float scale_factor = simulation_->get_fixup_scale_factor();
            ui::set_next_item_width(ui::calc_text_size("0.00000").x());
            if (ui::draw_float_input("##scaleinput", &scale_factor)) {
                simulation_->set_fixup_scale_factor(scale_factor);
            }
        }
        ui::pop_style_var();
    }

    void draw_simulation_status_group()
    {
        const SimulationStatus status = simulation_->get_status();
        ui::draw_text_disabled("simulator status:");
        ui::same_line();
        ui::push_style_color(ui::ColorVar::Text, calc_status_color(status));
        ui::draw_text(get_all_simulation_status_strings()[static_cast<size_t>(status)]);
        ui::pop_style_color();
    }

    std::string label_;
    SimulatorUIAPI* simulator_api_;
    std::shared_ptr<Simulation> simulation_;

    SimulationScrubber scrubber_{"##SimulationScrubber", simulator_api_, simulation_};
};

osc::SimulationToolbar::SimulationToolbar(
    std::string_view label, SimulatorUIAPI* simulator_api, std::shared_ptr<Simulation> simulation) :

    impl_{std::make_unique<Impl>(label, simulator_api, std::move(simulation))}
{}
osc::SimulationToolbar::SimulationToolbar(SimulationToolbar&&) noexcept = default;
osc::SimulationToolbar& osc::SimulationToolbar::operator=(SimulationToolbar&&) noexcept = default;
osc::SimulationToolbar::~SimulationToolbar() noexcept = default;
void osc::SimulationToolbar::on_draw()
{
    impl_->on_draw();
}
