#include "SimulationDetailsPanel.h"

#include <libopensimcreator/Documents/OutputExtractors/OutputExtractorDataTypeHelpers.h>
#include <libopensimcreator/Documents/Simulation/Simulation.h>
#include <libopensimcreator/Platform/msmicons.h>
#include <libopensimcreator/UI/Shared/BasicWidgets.h>
#include <libopensimcreator/UI/Simulation/ISimulatorUIAPI.h>
#include <libopensimcreator/UI/Simulation/SimulationOutputPlot.h>

#include <liboscar/platform/os.h>
#include <liboscar/ui/oscimgui.h>
#include <liboscar/ui/panels/PanelPrivate.h>
#include <liboscar/utils/Algorithms.h>
#include <liboscar/utils/Perf.h>

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
        std::string_view panelName,
        ISimulatorUIAPI* simulatorUIAPI,
        std::shared_ptr<const Simulation> simulation) :

        PanelPrivate{owner, parent, panelName},
        m_SimulatorUIAPI{simulatorUIAPI},
        m_Simulation{std::move(simulation)}
    {}

    void draw_content()
    {
        {
            ui::draw_vertical_spacer(1.0f/15.0f);
            ui::draw_text("info:");
            ui::same_line();
            ui::draw_help_marker("Top-level information about the simulation");
            ui::draw_separator();
            ui::draw_vertical_spacer(2.0f/15.0f);

            ui::set_num_columns(2);
            ui::draw_text("num reports");
            ui::next_column();
            ui::draw_text("%zu", m_Simulation->getNumReports());
            ui::next_column();
            ui::set_num_columns();
        }

        {
            OSC_PERF("draw simulation params");
            DrawSimulationParams(m_Simulation->getParams());
        }

        ui::draw_vertical_spacer(10.0f/15.0f);

        {
            OSC_PERF("draw simulation stats");
            drawSimulationStatPlots(*m_Simulation);
        }
    }

private:
    void drawSimulationStatPlots(const Simulation& sim)
    {
        auto outputs = sim.getOutputs();

        if (outputs.empty()) {
            ui::draw_text_disabled_and_centered("(no simulator output plots)");
            return;
        }

        ui::draw_vertical_spacer(1.0f/15.0f);
        ui::set_num_columns(2);
        ui::draw_text("plots:");
        ui::same_line();
        ui::draw_help_marker("Various statistics collected when the simulation was ran");
        ui::next_column();
        if (rgs::any_of(outputs, is_numeric, &OutputExtractor::getOutputType)) {

            ui::draw_button(MSMICONS_SAVE " Save All " MSMICONS_CARET_DOWN);
            if (ui::begin_popup_context_menu("##exportoptions", ui::PopupFlag::MouseButtonLeft)) {
                if (ui::draw_menu_item("as CSV")) {
                    m_SimulatorUIAPI->tryPromptToSaveOutputsAsCSV(outputs, false);
                }

                if (ui::draw_menu_item("as CSV (and open)")) {
                    m_SimulatorUIAPI->tryPromptToSaveOutputsAsCSV(outputs, true);
                }

                ui::end_popup();
            }
        }

        ui::next_column();
        ui::set_num_columns();
        ui::draw_separator();
        ui::draw_vertical_spacer(2.0f/15.0f);

        int imguiID = 0;
        ui::set_num_columns(2);
        for (const OutputExtractor& output : sim.getOutputs()) {
            ui::push_id(imguiID++);
            DrawOutputNameColumn(output, false);
            ui::next_column();
            SimulationOutputPlot plot{m_SimulatorUIAPI, output, 32.0f};
            plot.onDraw();
            ui::next_column();
            ui::pop_id();
        }
        ui::set_num_columns();
    }

    ISimulatorUIAPI* m_SimulatorUIAPI;
    std::shared_ptr<const Simulation> m_Simulation;
};

osc::SimulationDetailsPanel::SimulationDetailsPanel(
    Widget* parent,
    std::string_view panelName,
    ISimulatorUIAPI* simulatorUIAPI,
    std::shared_ptr<const Simulation> simulation) :

    Panel{std::make_unique<Impl>(*this, parent, panelName, simulatorUIAPI, std::move(simulation))}
{}
void osc::SimulationDetailsPanel::impl_draw_content() { private_data().draw_content(); }
