#include "simulation_details_panel.h"

#include <libopensimcreator/documents/simulation/simulation.h>
#include <libopensimcreator/platform/msmicons.h>
#include <libopensimcreator/ui/shared/basic_widgets.h>
#include <libopensimcreator/ui/simulation/simulator_ui_api.h>
#include <libopensimcreator/ui/simulation/simulation_output_plot.h>

#include <libopynsim/documents/output_extractors/output_extractor_data_type_helpers.h>
#include <liboscar/platform/os.h>
#include <liboscar/ui/oscimgui.h>
#include <liboscar/ui/panels/panel_private.h>
#include <liboscar/utilities/algorithms.h>
#include <liboscar/utilities/perf.h>

#include <algorithm>
#include <filesystem>
#include <memory>
#include <string_view>
#include <utility>

using namespace osc;
namespace rgs = std::ranges;

class osc::SimulationDetailsPanel::Impl final : public PanelPrivate {
public:
    explicit Impl(
        SimulationDetailsPanel& owner,
        Widget* parent,
        std::string_view panel_name,
        SimulatorUIAPI* simulator_uiapi,
        std::shared_ptr<const Simulation> simulation) :

        PanelPrivate{owner, parent, panel_name},
        simulator_uiapi_{simulator_uiapi},
        simulation_{std::move(simulation)}
    {}

    void draw_content()
    {
        {
            ui::draw_vertical_spacer(1.0f / 15.0f);
            ui::draw_text("info:");
            ui::same_line();
            ui::draw_help_marker("Top-level information about the simulation");
            ui::draw_separator();
            ui::draw_vertical_spacer(2.0f / 15.0f);

            ui::set_num_columns(2);
            ui::draw_text("num reports");
            ui::next_column();
            ui::draw_text("{}", simulation_->get_num_reports());
            ui::next_column();
            ui::set_num_columns();
        }

        {
            OSC_PERF("draw simulation params");
            draw_simulation_params(simulation_->get_params());
        }

        ui::draw_vertical_spacer(10.0f / 15.0f);

        {
            OSC_PERF("draw simulation stats");
            draw_simulation_stat_plots(*simulation_);
        }
    }

private:
    void draw_simulation_stat_plots(const Simulation& sim)
    {
        auto outputs = sim.get_outputs();

        if (outputs.empty()) {
            ui::draw_text_disabled_and_centered("(no simulator output plots)");
            return;
        }

        ui::draw_vertical_spacer(1.0f / 15.0f);
        ui::set_num_columns(2);
        ui::draw_text("plots:");
        ui::same_line();
        ui::draw_help_marker("Various statistics collected when the simulation was ran");
        ui::next_column();
        if (rgs::any_of(outputs, opyn::is_numeric, &opyn::SharedOutputExtractor::output_type)) {

            ui::draw_button(MSMICONS_SAVE " Save All " MSMICONS_CARET_DOWN);
            if (ui::begin_popup_context_menu("##exportoptions", ui::PopupFlag::MouseButtonLeft)) {
                if (ui::draw_menu_item("as CSV")) {
                    simulator_uiapi_->try_prompt_to_save_outputs_as_csv(outputs, false);
                }

                if (ui::draw_menu_item("as CSV (and open)")) {
                    simulator_uiapi_->try_prompt_to_save_outputs_as_csv(outputs, true);
                }

                ui::end_popup();
            }
        }

        ui::next_column();
        ui::set_num_columns();
        ui::draw_separator();
        ui::draw_vertical_spacer(2.0f / 15.0f);

        int imgui_id = 0;
        ui::set_num_columns(2);
        for (const opyn::SharedOutputExtractor& output : sim.get_outputs()) {
            ui::push_id(imgui_id++);
            draw_output_name_column(output, false);
            ui::next_column();
            SimulationOutputPlot plot{simulator_uiapi_, output, 32.0f};
            plot.on_draw();
            ui::next_column();
            ui::pop_id();
        }
        ui::set_num_columns();
    }

    SimulatorUIAPI* simulator_uiapi_;
    std::shared_ptr<const Simulation> simulation_;
};

osc::SimulationDetailsPanel::SimulationDetailsPanel(
    Widget* parent,
    std::string_view panel_name,
    SimulatorUIAPI* simulator_uiapi,
    std::shared_ptr<const Simulation> simulation) :

    Panel{std::make_unique<Impl>(*this, parent, panel_name, simulator_uiapi, std::move(simulation))}
{}
void osc::SimulationDetailsPanel::impl_draw_content()
{
    private_data().draw_content();
}
