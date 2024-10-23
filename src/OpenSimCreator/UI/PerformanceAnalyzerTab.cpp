#include "PerformanceAnalyzerTab.h"

#include <OpenSimCreator/Documents/Model/BasicModelStatePair.h>
#include <OpenSimCreator/Documents/OutputExtractors/OutputExtractor.h>
#include <OpenSimCreator/Documents/Simulation/ForwardDynamicSimulation.h>
#include <OpenSimCreator/Documents/Simulation/ForwardDynamicSimulator.h>
#include <OpenSimCreator/Documents/Simulation/ForwardDynamicSimulatorParams.h>
#include <OpenSimCreator/Documents/Simulation/IntegratorMethod.h>
#include <OpenSimCreator/Documents/Simulation/SimulationStatus.h>
#include <OpenSimCreator/UI/Shared/ParamBlockEditorPopup.h>
#include <OpenSimCreator/Utils/ParamBlock.h>
#include <OpenSimCreator/Utils/ParamValue.h>

#include <OpenSim/Simulation/Model/Model.h>
#include <oscar/Platform/IconCodepoints.h>
#include <oscar/Platform/os.h>
#include <oscar/Platform/Widget.h>
#include <oscar/UI/oscimgui.h>
#include <oscar/UI/Tabs/TabPrivate.h>
#include <oscar/Utils/Algorithms.h>

#include <algorithm>
#include <filesystem>
#include <optional>
#include <ostream>
#include <ranges>
#include <span>
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

    Impl(
        PerformanceAnalyzerTab& owner,
        Widget& parent,
        BasicModelStatePair baseModel,
        ParamBlock params) :

        TabPrivate{owner, &parent, OSC_ICON_FAST_FORWARD " PerformanceAnalyzerTab"},
        m_BaseModel{std::move(baseModel)},
        m_BaseParams{std::move(params)}
    {}

    void on_tick()
    {
        startSimsIfNecessary();
    }

    void onDraw()
    {
        ui::enable_dockspace_over_main_viewport();

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
                ui::draw_text_unformatted(m.label());
                ui::table_set_column_index(column++);
                ui::draw_progress_bar(sim.getProgress());
                ui::table_set_column_index(column++);
                ui::draw_text("%f", t);
                ui::table_set_column_index(column++);
                ui::draw_text("%i", static_cast<int>(steps));
            }

            ui::end_table();

            if (ui::draw_button(OSC_ICON_SAVE " Export to CSV")) {
                tryExportOutputs();
            }
        }

        ui::end_panel();

        if (m_ParamEditor.begin_popup()) {
            m_ParamEditor.on_draw();
            m_ParamEditor.end_popup();
        }
    }


private:
    void tryExportOutputs()
    {
        // try prompt user for save location
        const std::optional<std::filesystem::path> maybeCSVPath =
            prompt_user_for_file_save_location_add_extension_if_necessary("csv");

        if (!maybeCSVPath) {
            return;  // user probably cancelled out
        }

        std::ofstream fout{*maybeCSVPath};

        if (not fout) {
            return;  // IO error (can't write to that location?)
        }

        fout << "Integrator,Wall Time (sec),NumStepsTaken\n";

        for (const ForwardDynamicSimulation& simulation : m_Simulations) {

            const auto reports = simulation.getAllSimulationReports();
            if (reports.empty()) {
                continue;
            }

            const IntegratorMethod m = std::get<IntegratorMethod>(simulation.getParams().findValue("Integrator Method").value());
            const float t = m_WalltimeExtractor.getValueFloat(*simulation.getModel(), reports.back());
            const float steps = m_StepsTakenExtractor.getValueFloat(*simulation.getModel(), reports.back());

            fout << m.label() << ',' << t << ',' << steps << '\n';
        }
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
    ParamBlockEditorPopup m_ParamEditor{"parameditor", &m_BaseParams};
};


osc::PerformanceAnalyzerTab::PerformanceAnalyzerTab(
    Widget& parent,
    BasicModelStatePair modelState,
    const ParamBlock& params) :

    Tab{std::make_unique<Impl>(*this, parent, std::move(modelState), params)}
{}
void osc::PerformanceAnalyzerTab::impl_on_tick() { private_data().on_tick(); }
void osc::PerformanceAnalyzerTab::impl_on_draw() { private_data().onDraw(); }
