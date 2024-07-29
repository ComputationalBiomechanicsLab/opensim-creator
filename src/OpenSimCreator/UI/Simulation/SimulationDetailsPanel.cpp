#include "SimulationDetailsPanel.h"

#include <OpenSimCreator/Documents/Simulation/Simulation.h>
#include <OpenSimCreator/Documents/OutputExtractors/OutputExtractorDataTypeHelpers.h>
#include <OpenSimCreator/UI/Shared/BasicWidgets.h>
#include <OpenSimCreator/UI/Simulation/ISimulatorUIAPI.h>
#include <OpenSimCreator/UI/Simulation/SimulationOutputPlot.h>

#include <IconsFontAwesome5.h>
#include <oscar/Platform/os.h>
#include <oscar/UI/oscimgui.h>
#include <oscar/UI/Panels/StandardPanelImpl.h>
#include <oscar/Utils/Algorithms.h>
#include <oscar/Utils/Perf.h>

#include <filesystem>
#include <memory>
#include <ranges>
#include <string_view>
#include <utility>

using namespace osc;
namespace rgs = std::ranges;

class osc::SimulationDetailsPanel::Impl final : public StandardPanelImpl {
public:
    Impl(
        std::string_view panelName,
        ISimulatorUIAPI* simulatorUIAPI,
        std::shared_ptr<const Simulation> simulation) :

        StandardPanelImpl{panelName},
        m_SimulatorUIAPI{simulatorUIAPI},
        m_Simulation{std::move(simulation)}
    {}

private:
    void impl_draw_content() final
    {
        {
            ui::draw_dummy({0.0f, 1.0f});
            ui::draw_text_unformatted("info:");
            ui::same_line();
            ui::draw_help_marker("Top-level information about the simulation");
            ui::draw_separator();
            ui::draw_dummy({0.0f, 2.0f});

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

        ui::draw_dummy({0.0f, 10.0f});

        {
            OSC_PERF("draw simulation stats");
            drawSimulationStatPlots(*m_Simulation);
        }
    }

    void drawSimulationStatPlots(const Simulation& sim)
    {
        auto outputs = sim.getOutputs();

        if (outputs.empty())
        {
            ui::draw_text_disabled_and_centered("(no simulator output plots)");
            return;
        }

        ui::draw_dummy({0.0f, 1.0f});
        ui::set_num_columns(2);
        ui::draw_text_unformatted("plots:");
        ui::same_line();
        ui::draw_help_marker("Various statistics collected when the simulation was ran");
        ui::next_column();
        if (rgs::any_of(outputs, is_numeric, &OutputExtractor::getOutputType))
        {
            ui::draw_button(ICON_FA_SAVE " Save All " ICON_FA_CARET_DOWN);
            if (ui::begin_popup_context_menu("##exportoptions", ImGuiPopupFlags_MouseButtonLeft))
            {
                if (ui::draw_menu_item("as CSV"))
                {
                    m_SimulatorUIAPI->tryPromptToSaveOutputsAsCSV(outputs);
                }

                if (ui::draw_menu_item("as CSV (and open)"))
                {
                    if (const auto path = m_SimulatorUIAPI->tryPromptToSaveOutputsAsCSV(outputs)) {
                        open_file_in_os_default_application(*path);
                    }
                }

                ui::end_popup();
            }
        }

        ui::next_column();
        ui::set_num_columns();
        ui::draw_separator();
        ui::draw_dummy({0.0f, 2.0f});

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
    std::string_view panelName,
    ISimulatorUIAPI* simulatorUIAPI,
    std::shared_ptr<const Simulation> simulation) :

    m_Impl{std::make_unique<Impl>(panelName, simulatorUIAPI, std::move(simulation))}
{}
osc::SimulationDetailsPanel::SimulationDetailsPanel(SimulationDetailsPanel&&) noexcept = default;
osc::SimulationDetailsPanel& osc::SimulationDetailsPanel::operator=(SimulationDetailsPanel&&) noexcept = default;
osc::SimulationDetailsPanel::~SimulationDetailsPanel() noexcept = default;

CStringView osc::SimulationDetailsPanel::impl_get_name() const
{
    return m_Impl->name();
}

bool osc::SimulationDetailsPanel::impl_is_open() const
{
    return m_Impl->is_open();
}

void osc::SimulationDetailsPanel::impl_open()
{
    m_Impl->open();
}

void osc::SimulationDetailsPanel::impl_close()
{
    m_Impl->close();
}

void osc::SimulationDetailsPanel::impl_on_draw()
{
    m_Impl->on_draw();
}
