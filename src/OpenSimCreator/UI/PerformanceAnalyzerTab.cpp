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

#include <IconsFontAwesome5.h>
#include <OpenSim/Simulation/Model/Model.h>
#include <oscar/Platform/os.h>
#include <oscar/UI/oscimgui.h>
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
        for (int i = 0, len = GetNumFdSimulatorOutputExtractors(); i < len; ++i)
        {
            OutputExtractor o = GetFdSimulatorOutputExtractor(i);
            if (o.getName() == name)
            {
                return o;
            }
        }
        throw std::runtime_error{"cannot find output"};
    }
}

class osc::PerformanceAnalyzerTab::Impl final {
public:

    Impl(
        BasicModelStatePair baseModel,
        ParamBlock params) :

        m_BaseModel{std::move(baseModel)},
        m_BaseParams{std::move(params)}
    {
    }

    UID getID() const
    {
        return m_TabID;
    }

    CStringView getName() const
    {
        return ICON_FA_FAST_FORWARD " PerformanceAnalyzerTab";
    }

    void on_tick()
    {
        startSimsIfNecessary();
    }

    void onDraw()
    {
        ui::DockSpaceOverViewport(
            ui::GetMainViewport(),
            ImGuiDockNodeFlags_PassthruCentralNode
        );

        ui::Begin("Inputs");

        ui::InputInt("parallelism", &m_Parallelism);
        if (ui::Button("edit base params"))
        {
            m_ParamEditor.open();
        }

        if (ui::Button("(re)start"))
        {
            populateParamsFromParamBlock();
        }

        ui::End();

        ui::Begin("Outputs");

        if (!m_Sims.empty() && ui::BeginTable("simulations", 4))
        {
            ui::TableSetupColumn("Integrator");
            ui::TableSetupColumn("Progress");
            ui::TableSetupColumn("Wall Time (sec)");
            ui::TableSetupColumn("NumStepsTaken");
            ui::TableHeadersRow();

            for (ForwardDynamicSimulation const& sim : m_Sims)
            {
                auto reports = sim.getAllSimulationReports();
                if (reports.empty())
                {
                    continue;
                }

                IntegratorMethod m = std::get<IntegratorMethod>(sim.getParams().findValue("Integrator Method").value());
                float t = m_WalltimeExtractor.getValueFloat(*sim.getModel(), reports.back());
                float steps = m_StepsTakenExtractor.getValueFloat(*sim.getModel(), reports.back());

                ui::TableNextRow();
                int column = 0;
                ui::TableSetColumnIndex(column++);
                ui::TextUnformatted(m.label());
                ui::TableSetColumnIndex(column++);
                ui::ProgressBar(sim.getProgress());
                ui::TableSetColumnIndex(column++);
                ui::Text("%f", t);
                ui::TableSetColumnIndex(column++);
                ui::Text("%i", static_cast<int>(steps));
            }

            ui::EndTable();

            if (ui::Button(ICON_FA_SAVE " Export to CSV"))
            {
                tryExportOutputs();
            }
        }

        ui::End();

        if (m_ParamEditor.beginPopup())
        {
            m_ParamEditor.onDraw();
            m_ParamEditor.endPopup();
        }
    }


private:
    void tryExportOutputs()
    {
        // try prompt user for save location
        std::optional<std::filesystem::path> const maybeCSVPath =
            PromptUserForFileSaveLocationAndAddExtensionIfNecessary("csv");

        if (!maybeCSVPath)
        {
            return;  // user probably cancelled out
        }

        std::ofstream fout{*maybeCSVPath};

        if (!fout)
        {
            return;  // IO error (can't write to that location?)
        }

        fout << "Integrator,Wall Time (sec),NumStepsTaken\n";

        for (ForwardDynamicSimulation const& sim : m_Sims)
        {
            auto reports = sim.getAllSimulationReports();
            if (reports.empty())
            {
                continue;
            }

            IntegratorMethod m = std::get<IntegratorMethod>(sim.getParams().findValue("Integrator Method").value());
            float t = m_WalltimeExtractor.getValueFloat(*sim.getModel(), reports.back());
            float steps = m_StepsTakenExtractor.getValueFloat(*sim.getModel(), reports.back());

            fout << m.label() << ',' << t << ',' << steps << '\n';
        }
    }

    // populate the list of input parameters
    void populateParamsFromParamBlock()
    {
        m_Params.clear();
        m_Sims.clear();

        ForwardDynamicSimulatorParams params = FromParamBlock(m_BaseParams);

        // for now, just permute through integration methods
        for (IntegratorMethod m : IntegratorMethod::all())
        {
            params.integratorMethodUsed = m;
            m_Params.push_back(params);
        }
    }

    // dequeue any queued sims
    void startSimsIfNecessary()
    {
        int nAvail = static_cast<int>(m_Params.size()) - static_cast<int>(m_Sims.size());
        int nActive = static_cast<int>(rgs::count_if(m_Sims, [](auto const& sim) { return sim.getStatus() == SimulationStatus::Running || sim.getStatus() == SimulationStatus::Initializing; }));
        int nToStart = min(nAvail, m_Parallelism - nActive);

        if (nToStart <= 0)
        {
            return;  // nothing to start
        }

        // load model and enqueue sims
        for (auto it = m_Params.end() - nAvail, end = it + nToStart; it != end; ++it)
        {
            m_Sims.emplace_back(m_BaseModel, *it);
        }
    }

    UID m_TabID;

    int m_Parallelism = 1;
    BasicModelStatePair m_BaseModel;
    ParamBlock m_BaseParams;
    std::vector<ForwardDynamicSimulatorParams> m_Params;
    std::vector<ForwardDynamicSimulation> m_Sims;

    OutputExtractor m_WalltimeExtractor = GetSimulatorOutputExtractor("Wall time");
    OutputExtractor m_StepsTakenExtractor = GetSimulatorOutputExtractor("NumStepsTaken");
    ParamBlockEditorPopup m_ParamEditor{"parameditor", &m_BaseParams};
};


// public API (PIMPL)

osc::PerformanceAnalyzerTab::PerformanceAnalyzerTab(
    ParentPtr<ITabHost> const&,
    BasicModelStatePair modelState,
    ParamBlock const& params) :

    m_Impl{std::make_unique<Impl>(std::move(modelState), params)}
{
}

osc::PerformanceAnalyzerTab::PerformanceAnalyzerTab(PerformanceAnalyzerTab&&) noexcept = default;
osc::PerformanceAnalyzerTab& osc::PerformanceAnalyzerTab::operator=(PerformanceAnalyzerTab&&) noexcept = default;
osc::PerformanceAnalyzerTab::~PerformanceAnalyzerTab() noexcept = default;

UID osc::PerformanceAnalyzerTab::impl_get_id() const
{
    return m_Impl->getID();
}

CStringView osc::PerformanceAnalyzerTab::impl_get_name() const
{
    return m_Impl->getName();
}

void osc::PerformanceAnalyzerTab::impl_on_tick()
{
    m_Impl->on_tick();
}

void osc::PerformanceAnalyzerTab::impl_on_draw()
{
    m_Impl->onDraw();
}
