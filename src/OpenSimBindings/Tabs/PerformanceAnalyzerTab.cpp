#include "PerformanceAnalyzerTab.hpp"

#include "src/OpenSimBindings/Widgets/ParamBlockEditorPopup.hpp"
#include "src/OpenSimBindings/BasicModelStatePair.hpp"
#include "src/OpenSimBindings/ForwardDynamicSimulation.hpp"
#include "src/OpenSimBindings/ForwardDynamicSimulator.hpp"
#include "src/OpenSimBindings/ForwardDynamicSimulatorParams.hpp"
#include "src/OpenSimBindings/IntegratorMethod.hpp"
#include "src/OpenSimBindings/OutputExtractor.hpp"
#include "src/OpenSimBindings/ParamBlock.hpp"
#include "src/OpenSimBindings/ParamValue.hpp"
#include "src/OpenSimBindings/SimulationStatus.hpp"
#include "src/Platform/os.hpp"
#include "src/Utils/SynchronizedValue.hpp"

#include <SDL_events.h>
#include <IconsFontAwesome5.h>
#include <imgui.h>
#include <nonstd/span.hpp>
#include <OpenSim/Simulation/Model/Model.h>

#include <algorithm>
#include <filesystem>
#include <optional>
#include <ostream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

static osc::OutputExtractor GetSimulatorOutputExtractor(std::string_view name)
{
    for (int i = 0, len = osc::GetNumFdSimulatorOutputExtractors(); i < len; ++i)
    {
        osc::OutputExtractor o = osc::GetFdSimulatorOutputExtractor(i);
        if (o.getName() == name)
        {
            return o;
        }
    }
    throw std::runtime_error{"cannot find output"};
}

class osc::PerformanceAnalyzerTab::Impl final {
public:

    Impl(TabHost* parent, BasicModelStatePair baseModel, osc::ParamBlock const& params) :
        m_Parent{std::move(parent)},
        m_BaseModel{std::move(baseModel)},
        m_BaseParams{params}
    {
    }

    UID getID() const
    {
        return m_ID;
    }

    CStringView getName() const
    {
        return m_Name;
    }

    TabHost* parent()
    {
        return m_Parent;
    }

    void onMount()
    {

    }

    void onUnmount()
    {

    }

    bool onEvent(SDL_Event const&)
    {
        return false;
    }

    void onTick()
    {
        startSimsIfNecessary();
    }

    void onDrawMainMenu()
    {

    }

    void onDraw()
    {
        ImGui::Begin("Inputs");

        ImGui::InputInt("parallelism", &m_Parallelism);
        if (ImGui::Button("edit base params"))
        {
            m_ParamEditor.open();
        }

        if (ImGui::Button("(re)start"))
        {
            populateParamsFromParamBlock();
        }

        ImGui::End();

        ImGui::Begin("Outputs");

        if (!m_Sims.empty() && ImGui::BeginTable("simulations", 4))
        {
            ImGui::TableSetupColumn("Integrator");
            ImGui::TableSetupColumn("Progress");
            ImGui::TableSetupColumn("Wall Time (sec)");
            ImGui::TableSetupColumn("NumStepsTaken");
            ImGui::TableHeadersRow();

            for (osc::ForwardDynamicSimulation const& sim : m_Sims)
            {
                auto reports = sim.getAllSimulationReports();
                if (reports.empty())
                {
                    continue;
                }

                IntegratorMethod m = std::get<IntegratorMethod>(sim.getParams().findValue("Integrator Method").value());
                float t = m_WalltimeExtractor.getValueFloat(*sim.getModel(), reports.back());
                float steps = m_StepsTakenExtractor.getValueFloat(*sim.getModel(), reports.back());

                ImGui::TableNextRow();
                int column = 0;
                ImGui::TableSetColumnIndex(column++);
                ImGui::TextUnformatted(GetIntegratorMethodString(m));
                ImGui::TableSetColumnIndex(column++);
                ImGui::ProgressBar(sim.getProgress());
                ImGui::TableSetColumnIndex(column++);
                ImGui::Text("%f", t);
                ImGui::TableSetColumnIndex(column++);
                ImGui::Text("%i", static_cast<int>(steps));
            }

            ImGui::EndTable();

            if (ImGui::Button(ICON_FA_SAVE " Export to CSV"))
            {
                tryExportOutputs();
            }
        }

        ImGui::End();

        if (m_ParamEditor.beginPopup())
        {
            m_ParamEditor.drawPopupContent();
            m_ParamEditor.endPopup();
        }
    }


private:
    void tryExportOutputs()
    {
        // try prompt user for save location
        std::optional<std::filesystem::path> const maybeCSVPath =
            osc::PromptUserForFileSaveLocationAndAddExtensionIfNecessary("csv");

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

        for (osc::ForwardDynamicSimulation const& sim : m_Sims)
        {
            auto reports = sim.getAllSimulationReports();
            if (reports.empty())
            {
                continue;
            }

            IntegratorMethod m = std::get<IntegratorMethod>(sim.getParams().findValue("Integrator Method").value());
            char const* integratorMethodStr = GetIntegratorMethodString(m);
            float t = m_WalltimeExtractor.getValueFloat(*sim.getModel(), reports.back());
            float steps = m_StepsTakenExtractor.getValueFloat(*sim.getModel(), reports.back());

            fout << integratorMethodStr << ',' << t << ',' << steps << '\n';
        }
    }

    // populate the list of input parameters
    void populateParamsFromParamBlock()
    {
        m_Params.clear();
        m_Sims.clear();

        osc::ForwardDynamicSimulatorParams params = osc::FromParamBlock(m_BaseParams);

        // for now, just permute through integration methods
        for (osc::IntegratorMethod m : osc::GetAllIntegratorMethods())
        {
            params.IntegratorMethodUsed = m;
            m_Params.push_back(params);
        }
    }

    // dequeue any queued sims
    void startSimsIfNecessary()
    {
        int nAvail = static_cast<int>(m_Params.size()) - static_cast<int>(m_Sims.size());
        int nActive = static_cast<int>(std::count_if(m_Sims.begin(), m_Sims.end(), [](auto const& sim) { return sim.getStatus() == SimulationStatus::Running || sim.getStatus() == SimulationStatus::Initializing; }));
        int nToStart = std::min(nAvail, m_Parallelism - nActive);

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

    // tab data
    UID m_ID;
    std::string m_Name = ICON_FA_FAST_FORWARD " PerformanceAnalyzerTab";
    TabHost* m_Parent;

    int m_Parallelism = 1;
    BasicModelStatePair m_BaseModel;
    osc::ParamBlock m_BaseParams;
    std::vector<osc::ForwardDynamicSimulatorParams> m_Params;
    std::vector<osc::ForwardDynamicSimulation> m_Sims;

    osc::OutputExtractor m_WalltimeExtractor = GetSimulatorOutputExtractor("Wall time");
    osc::OutputExtractor m_StepsTakenExtractor = GetSimulatorOutputExtractor("NumStepsTaken");
    osc::ParamBlockEditorPopup m_ParamEditor{"parameditor", &m_BaseParams};
};


// public API (PIMPL)

osc::PerformanceAnalyzerTab::PerformanceAnalyzerTab(TabHost* parent, BasicModelStatePair modelState, ParamBlock const& params) :
    m_Impl{std::make_unique<Impl>(std::move(parent), std::move(modelState), params)}
{
}

osc::PerformanceAnalyzerTab::PerformanceAnalyzerTab(PerformanceAnalyzerTab&&) noexcept = default;
osc::PerformanceAnalyzerTab& osc::PerformanceAnalyzerTab::operator=(PerformanceAnalyzerTab&&) noexcept = default;
osc::PerformanceAnalyzerTab::~PerformanceAnalyzerTab() noexcept = default;

osc::UID osc::PerformanceAnalyzerTab::implGetID() const
{
    return m_Impl->getID();
}

osc::CStringView osc::PerformanceAnalyzerTab::implGetName() const
{
    return m_Impl->getName();
}

osc::TabHost* osc::PerformanceAnalyzerTab::implParent() const
{
    return m_Impl->parent();
}

void osc::PerformanceAnalyzerTab::implOnMount()
{
    m_Impl->onMount();
}

void osc::PerformanceAnalyzerTab::implOnUnmount()
{
    m_Impl->onUnmount();
}

bool osc::PerformanceAnalyzerTab::implOnEvent(SDL_Event const& e)
{
    return m_Impl->onEvent(e);
}

void osc::PerformanceAnalyzerTab::implOnTick()
{
    m_Impl->onTick();
}

void osc::PerformanceAnalyzerTab::implOnDrawMainMenu()
{
    m_Impl->onDrawMainMenu();
}

void osc::PerformanceAnalyzerTab::implOnDraw()
{
    m_Impl->onDraw();
}
