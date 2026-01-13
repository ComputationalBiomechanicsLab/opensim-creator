#include "PerformanceAnalyzerTab.h"

#include <libopensimcreator/Documents/Model/BasicModelStatePair.h>
#include <libopensimcreator/Documents/OutputExtractors/OutputExtractor.h>
#include <libopensimcreator/Documents/Simulation/ForwardDynamicSimulation.h>
#include <libopensimcreator/Documents/Simulation/ForwardDynamicSimulator.h>
#include <libopensimcreator/Documents/Simulation/ForwardDynamicSimulatorParams.h>
#include <libopensimcreator/Documents/Simulation/IntegratorMethod.h>
#include <libopensimcreator/Documents/Simulation/SimulationStatus.h>
#include <libopensimcreator/Platform/msmicons.h>
#include <libopensimcreator/UI/Shared/ParamBlockEditorPopup.h>
#include <libopensimcreator/Utils/ParamBlock.h>
#include <libopensimcreator/Utils/ParamValue.h>

#include <liboscar/platform/App.h>
#include <liboscar/platform/Widget.h>
#include <liboscar/ui/oscimgui.h>
#include <liboscar/ui/tabs/TabPrivate.h>
#include <liboscar/utils/Algorithms.h>
#include <OpenSim/Simulation/Model/Model.h>

#include <algorithm>
#include <filesystem>
#include <optional>
#include <ostream>
#include <string_view>
#include <utility>
#include <vector>

using namespace osc;
namespace rgs = std::ranges;

namespace
{
    OutputExtractor GetSimulatorOutputExtractor(std::string_view name)
    {
        for (int i = 0, len = GetNumFdSimulatorOutputExtractors(); i < len; ++i) {
            OutputExtractor o = GetFdSimulatorOutputExtractor(i);
            if (o.getName() == name) {
                return o;
            }
        }
        throw std::runtime_error{"cannot find output"};
    }
}

class osc::PerformanceAnalyzerTab::Impl final : public TabPrivate {
public:

    explicit Impl(
        PerformanceAnalyzerTab& owner,
        Widget* parent,
        BasicModelStatePair baseModel,
        ParamBlock params) :

        TabPrivate{owner, parent, MSMICONS_FAST_FORWARD " PerformanceAnalyzerTab"},
        m_BaseModel{std::move(baseModel)},
        m_BaseParams{std::move(params)}
    {}

    void on_tick()
    {
        startSimsIfNecessary();
    }

    void onDraw()
    {
        ui::enable_dockspace_over_main_window();

        ui::begin_panel("Inputs");

        ui::draw_int_input("parallelism", &m_Parallelism);
        if (ui::draw_button("edit base params")) {
            m_ParamEditor.open();
        }

        if (ui::draw_button("(re)start")) {
            populateParamsFromParamBlock();
        }

        ui::end_panel();

        ui::begin_panel("Outputs");

        if (not m_Simulations.empty() and ui::begin_table("simulations", 4)) {
            ui::table_setup_column("Integrator");
            ui::table_setup_column("Progress");
            ui::table_setup_column("Wall Time (sec)");
            ui::table_setup_column("NumStepsTaken");
            ui::table_headers_row();

            for (const ForwardDynamicSimulation& sim : m_Simulations) {
                const auto reports = sim.getAllSimulationReports();
                if (reports.empty()) {
                    continue;
                }

                const IntegratorMethod m = std::get<IntegratorMethod>(sim.getParams().findValue("Integrator Method").value());
                const float t = m_WalltimeExtractor.getValueFloat(*sim.getModel(), reports.back());
                const float steps = m_StepsTakenExtractor.getValueFloat(*sim.getModel(), reports.back());

                ui::table_next_row();
                int column = 0;
                ui::table_set_column_index(column++);
                ui::draw_text(m.label());
                ui::table_set_column_index(column++);
                ui::draw_progress_bar(sim.getProgress());
                ui::table_set_column_index(column++);
                ui::draw_text("%f", t);
                ui::table_set_column_index(column++);
                ui::draw_text("%i", static_cast<int>(steps));
            }

            ui::end_table();

            if (ui::draw_button(MSMICONS_SAVE " Export to CSV")) {
                promptUserToExportOutputs();
            }
        }

        ui::end_panel();

        if (m_ParamEditor.begin_popup()) {
            m_ParamEditor.on_draw();
            m_ParamEditor.end_popup();
        }
    }


private:
    void writeOutputsAsCSV(std::ostream& out)
    {
        out << "Integrator,Wall Time (sec),NumStepsTaken\n";
        for (const ForwardDynamicSimulation& simulation : m_Simulations) {

            const auto reports = simulation.getAllSimulationReports();
            if (reports.empty()) {
                continue;
            }

            const IntegratorMethod m = std::get<IntegratorMethod>(simulation.getParams().findValue("Integrator Method").value());
            const float t = m_WalltimeExtractor.getValueFloat(*simulation.getModel(), reports.back());
            const float steps = m_StepsTakenExtractor.getValueFloat(*simulation.getModel(), reports.back());

            out << m.label() << ',' << t << ',' << steps << '\n';
        }
    }

    void promptUserToExportOutputs()
    {
        // Pre-write the CSV content in-memory so that the asynchronous user prompt isn't
        // dependent on a bunch of state.
        std::stringstream ss;
        writeOutputsAsCSV(ss);

        App::upd().prompt_user_to_save_file_with_extension_async([content = std::move(ss).str()](std::optional<std::filesystem::path> p)
        {
            if (not p) {
                return;  // user cancelled out of the prompt
            }

            std::ofstream ofs{*p};
            if (not ofs) {
                return;  // IO error (can't write to that location?)
            }
            ofs << content;
        }, "csv");
    }

    // populate the list of input parameters
    void populateParamsFromParamBlock()
    {
        m_Params.clear();
        m_Simulations.clear();

        ForwardDynamicSimulatorParams params = FromParamBlock(m_BaseParams);

        // for now, just permute through integration methods
        for (IntegratorMethod m : IntegratorMethod::all()) {
            params.integratorMethodUsed = m;
            m_Params.push_back(params);
        }
    }

    // dequeue any queued sims
    void startSimsIfNecessary()
    {
        const int nAvail = static_cast<int>(m_Params.size()) - static_cast<int>(m_Simulations.size());
        const int nActive = static_cast<int>(rgs::count_if(m_Simulations, [](const auto& sim) { return sim.getStatus() == SimulationStatus::Running || sim.getStatus() == SimulationStatus::Initializing; }));
        const int nToStart = min(nAvail, m_Parallelism - nActive);

        if (nToStart <= 0) {
            return;  // nothing to start
        }

        // load model and enqueue sims
        for (auto it = m_Params.end() - nAvail, end = it + nToStart; it != end; ++it) {
            m_Simulations.emplace_back(m_BaseModel, *it);
        }
    }

    int m_Parallelism = 1;
    BasicModelStatePair m_BaseModel;
    ParamBlock m_BaseParams;
    std::vector<ForwardDynamicSimulatorParams> m_Params;
    std::vector<ForwardDynamicSimulation> m_Simulations;

    OutputExtractor m_WalltimeExtractor = GetSimulatorOutputExtractor("Wall time");
    OutputExtractor m_StepsTakenExtractor = GetSimulatorOutputExtractor("NumStepsTaken");
    ParamBlockEditorPopup m_ParamEditor{&owner(), "parameditor", &m_BaseParams};
};


osc::PerformanceAnalyzerTab::PerformanceAnalyzerTab(
    Widget* parent,
    BasicModelStatePair modelState,
    const ParamBlock& params) :

    Tab{std::make_unique<Impl>(*this, parent, std::move(modelState), params)}
{}
void osc::PerformanceAnalyzerTab::impl_on_tick() { private_data().on_tick(); }
void osc::PerformanceAnalyzerTab::impl_on_draw() { private_data().onDraw(); }
