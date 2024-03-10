#include "SimulationOutputPlot.h"

#include <OpenSimCreator/Documents/Simulation/ISimulation.h>
#include <OpenSimCreator/Documents/Simulation/SimulationClock.h>
#include <OpenSimCreator/Documents/Simulation/SimulationReport.h>
#include <OpenSimCreator/OutputExtractors/IOutputExtractor.h>
#include <OpenSimCreator/OutputExtractors/OutputExtractor.h>
#include <OpenSimCreator/UI/Simulation/ISimulatorUIAPI.h>

#include <IconsFontAwesome5.h>
#include <oscar/UI/oscimgui.h>
#include <OpenSim/Simulation/Model/Model.h>
#include <oscar/Graphics/Color.h>
#include <oscar/Maths/Vec2.h>
#include <oscar/Platform/Log.h>
#include <oscar/Platform/os.h>
#include <oscar/UI/ImGuiHelpers.h>
#include <oscar/Utils/Algorithms.h>
#include <oscar/Utils/Assertions.h>
#include <oscar/Utils/Perf.h>

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <ostream>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

using namespace osc;

namespace
{
    std::vector<OutputExtractor> GetAllUserDesiredOutputs(ISimulatorUIAPI& api)
    {
        int nOutputs = api.getNumUserOutputExtractors();

        std::vector<OutputExtractor> rv;
        rv.reserve(nOutputs);
        for (int i = 0; i < nOutputs; ++i)
        {
            rv.push_back(api.getUserOutputExtractor(i));
        }
        return rv;
    }

    // export a timeseries to a CSV file and return the filepath
    std::string ExportTimeseriesToCSV(
        std::span<float const> times,
        std::span<float const> values,
        std::string_view header)
    {
        // try prompt user for save location
        std::optional<std::filesystem::path> const maybeCSVPath =
            PromptUserForFileSaveLocationAndAddExtensionIfNecessary("csv");

        if (!maybeCSVPath)
        {
            return {};  // user probably cancelled out
        }

        std::filesystem::path const& csvPath = *maybeCSVPath;

        std::ofstream fout{csvPath};

        if (!fout)
        {
            log_error("%s: error opening file for writing", csvPath.string().c_str());
            return {};  // error opening output file for writing
        }

        fout << "time," << header << '\n';
        for (size_t i = 0, len = min(times.size(), values.size()); i < len; ++i)
        {
            fout << times[i] << ',' << values[i] << '\n';
        }

        if (!fout)
        {
            log_error("%s: error encountered while writing CSV data to file", csvPath.string().c_str());
            return {};  // error writing
        }

        log_info("%: successfully wrote CSV data to output file", csvPath.string().c_str());

        return csvPath.string();
    }

    std::vector<float> PopulateFirstNNumericOutputValues(
        OpenSim::Model const& model,
        std::span<SimulationReport const> reports,
        IOutputExtractor const& output)
    {
        std::vector<float> rv;
        rv.resize(reports.size());
        output.getValuesFloat(model, reports, rv);
        return rv;
    }

    std::vector<float> PopulateFirstNTimeValues(std::span<SimulationReport const> reports)
    {
        std::vector<float> times;
        times.reserve(reports.size());
        for (SimulationReport const& r : reports)
        {
            times.push_back(static_cast<float>(r.getState().getTime()));
        }
        return times;
    }

    std::string TryExportNumericOutputToCSV(
        ISimulation& sim,
        IOutputExtractor const& output)
    {
        OSC_ASSERT(output.getOutputType() == OutputType::Float);

        std::vector<SimulationReport> reports = sim.getAllSimulationReports();
        std::vector<float> values = PopulateFirstNNumericOutputValues(*sim.getModel(), reports, output);
        std::vector<float> times = PopulateFirstNTimeValues(reports);

        return ExportTimeseriesToCSV(times, values, output.getName());
    }

    void DrawToggleWatchOutputMenuItem(
        ISimulatorUIAPI& api,
        OutputExtractor const& output)
    {
        bool isWatching = api.hasUserOutputExtractor(output);

        if (ui::MenuItem(ICON_FA_EYE " Watch Output", {}, &isWatching))
        {
            if (isWatching)
            {
                api.addUserOutputExtractor(output);
            }
            else
            {
                api.removeUserOutputExtractor(output);
            }
        }
        ui::DrawTooltipIfItemHovered("Watch Output", "Watch the selected output. This makes it appear in the 'Output Watches' window in the editor panel and the 'Output Plots' window during a simulation");
    }

    void DrawGenericNumericOutputContextMenuItems(
        ISimulatorUIAPI& api,
        ISimulation& sim,
        OutputExtractor const& output)
    {
        OSC_ASSERT(output.getOutputType() == OutputType::Float);

        if (ui::MenuItem(ICON_FA_SAVE "Save as CSV"))
        {
            TryExportNumericOutputToCSV(sim, output);
        }

        if (ui::MenuItem(ICON_FA_SAVE "Save as CSV (and open)"))
        {
            std::string p = TryExportNumericOutputToCSV(sim, output);
            if (!p.empty())
            {
                OpenPathInOSDefaultApplication(p);
            }
        }

        DrawToggleWatchOutputMenuItem(api, output);
    }

    std::filesystem::path TryExportOutputsToCSV(ISimulation& sim, std::span<OutputExtractor const> outputs)
    {
        std::vector<SimulationReport> reports = sim.getAllSimulationReports();
        std::vector<float> times = PopulateFirstNTimeValues(reports);

        // try prompt user for save location
        std::optional<std::filesystem::path> const maybeCSVPath =
            PromptUserForFileSaveLocationAndAddExtensionIfNecessary("csv");

        if (!maybeCSVPath)
        {
            // user probably cancelled out
            return {};
        }
        std::filesystem::path const& csvPath = *maybeCSVPath;

        std::ofstream fout{csvPath};

        if (!fout)
        {
            log_error("%s: error opening file for writing", csvPath.string().c_str());
            return {};  // error opening output file for writing
        }

        // header line
        fout << "time";
        for (OutputExtractor const& o : outputs)
        {
            fout << ',' << o.getName();
        }
        fout << '\n';


        // data lines
        auto guard = sim.getModel();
        for (size_t i = 0; i < reports.size(); ++i)
        {
            fout << times.at(i);  // time column

            SimulationReport r = reports[i];
            for (OutputExtractor const& o : outputs)
            {
                fout << ',' << o.getValueFloat(*guard, r);
            }

            fout << '\n';
        }

        if (!fout)
        {
            log_warn("%s: encountered error while writing output data: some of the data may have been written, but maybe not all of it", csvPath.string().c_str());
        }

        return csvPath;
    }
}

class osc::SimulationOutputPlot::Impl final {
public:

    Impl(ISimulatorUIAPI* api, OutputExtractor outputExtractor, float height) :
        m_API{api},
        m_OutputExtractor{std::move(outputExtractor)},
        m_Height{height}
    {
    }

    void onDraw()
    {
        ISimulation& sim = m_API->updSimulation();

        ptrdiff_t const nReports = sim.getNumReports();
        OutputType outputType = m_OutputExtractor.getOutputType();

        if (nReports <= 0)
        {
            ui::Text("no data (yet)");
        }
        else if (outputType == OutputType::Float)
        {
            ui::SetNextItemWidth(ui::GetContentRegionAvail().x);
            drawFloatOutputPlot(sim);
        }
        else if (outputType == OutputType::String)
        {
            SimulationReport r = m_API->trySelectReportBasedOnScrubbing().value_or(sim.getSimulationReport(nReports - 1));
            ui::TextUnformatted(m_OutputExtractor.getValueString(*sim.getModel(), r));

            // draw context menu (if user right clicks)
            if (ui::BeginPopupContextItem("plotcontextmenu"))
            {
                DrawToggleWatchOutputMenuItem(*m_API, m_OutputExtractor);
                ui::EndPopup();
            }
        }
        else
        {
            ui::Text("unknown output type");
        }
    }

private:
    void drawFloatOutputPlot(ISimulation& sim)
    {
        OSC_ASSERT(m_OutputExtractor.getOutputType() == OutputType::Float);

        ImU32 const currentTimeLineColor = ui::ToImU32(Color::yellow().with_alpha(0.6f));
        ImU32 const hoverTimeLineColor = ui::ToImU32(Color::yellow().with_alpha(0.3f));

        // collect data
        ptrdiff_t const nReports = sim.getNumReports();
        if (nReports <= 0)
        {
            ui::Text("no data (yet)");
            return;
        }

        std::vector<float> buf;
        {
            OSC_PERF("collect output data");
            std::vector<SimulationReport> reports = sim.getAllSimulationReports();
            buf.resize(reports.size());
            m_OutputExtractor.getValuesFloat(*sim.getModel(), reports, buf);
        }

        // draw plot
        float const plotWidth = ui::GetContentRegionAvail().x;
        Vec2 plotTopLeft{};
        Vec2 plotBottomRight{};

        {
            OSC_PERF("draw output plot");

            ImPlot::PushStyleVar(ImPlotStyleVar_PlotPadding, {0.0f, 0.0f});
            ImPlot::PushStyleVar(ImPlotStyleVar_PlotBorderSize, 0.0f);
            ImPlot::PushStyleVar(ImPlotStyleVar_FitPadding, {0.0f, 1.0f});

            if (ImPlot::BeginPlot("##", ImVec2(plotWidth, m_Height), ImPlotFlags_NoTitle | ImPlotFlags_NoLegend | ImPlotFlags_NoInputs | ImPlotFlags_NoMenus | ImPlotFlags_NoBoxSelect | ImPlotFlags_NoFrame))
            {
                ImPlot::SetupAxis(ImAxis_X1, nullptr, ImPlotAxisFlags_NoDecorations | ImPlotAxisFlags_NoMenus | ImPlotAxisFlags_AutoFit);
                ImPlot::SetupAxis(ImAxis_Y1, nullptr, ImPlotAxisFlags_NoDecorations | ImPlotAxisFlags_NoMenus | ImPlotAxisFlags_AutoFit);
                ImPlot::PushStyleColor(ImPlotCol_Line, ImVec4{1.0f, 1.0f, 1.0f, 0.7f});
                ImPlot::PushStyleColor(ImPlotCol_PlotBg, ImVec4{0.0f, 0.0f, 0.0f, 0.0f});
                ImPlot::PlotLine("##",
                    buf.data(),
                    static_cast<int>(buf.size()));
                ImPlot::PopStyleColor();
                ImPlot::PopStyleColor();
                plotTopLeft = ImPlot::GetPlotPos();
                plotBottomRight = plotTopLeft + Vec2{ImPlot::GetPlotSize()};

                ImPlot::EndPlot();
            }
            ImPlot::PopStyleVar();
            ImPlot::PopStyleVar();
            ImPlot::PopStyleVar();
        }


        // draw context menu (if user right clicks)
        if (ui::BeginPopupContextItem("plotcontextmenu"))
        {
            DrawGenericNumericOutputContextMenuItems(*m_API, sim, m_OutputExtractor);
            ui::EndPopup();
        }

        // (the rest): handle scrubber overlay
        OSC_PERF("draw output plot overlay");

        // figure out mapping between screen space and plot space

        SimulationClock::time_point simStartTime = sim.getSimulationReport(0).getTime();
        SimulationClock::time_point simEndTime = sim.getSimulationReport(nReports-1).getTime();
        SimulationClock::duration simTimeStep = (simEndTime-simStartTime)/nReports;
        SimulationClock::time_point simScrubTime = m_API->getSimulationScrubTime();

        float simScrubPct = static_cast<float>(static_cast<double>((simScrubTime - simStartTime)/(simEndTime - simStartTime)));

        ImDrawList* drawlist = ui::GetWindowDrawList();

        // draw a vertical Y line showing the current scrub time over the plots
        {
            float plotScrubLineX = plotTopLeft.x + simScrubPct*(plotBottomRight.x - plotTopLeft.x);
            Vec2 p1 = {plotScrubLineX, plotBottomRight.y};
            Vec2 p2 = {plotScrubLineX, plotTopLeft.y};
            drawlist->AddLine(p1, p2, currentTimeLineColor);
        }

        if (ui::IsItemHovered())
        {
            Vec2 mp = ui::GetMousePos();
            Vec2 plotLoc = mp - plotTopLeft;
            float relLoc = plotLoc.x / (plotBottomRight.x - plotTopLeft.x);
            SimulationClock::time_point timeLoc = simStartTime + relLoc*(simEndTime - simStartTime);

            // draw vertical line to show current X of their hover
            {
                Vec2 p1 = {mp.x, plotBottomRight.y};
                Vec2 p2 = {mp.x, plotTopLeft.y};
                drawlist->AddLine(p1, p2, hoverTimeLineColor);
            }

            // show a tooltip of X and Y
            {
                int step = static_cast<int>((timeLoc - simStartTime) / simTimeStep);
                if (0 <= step && static_cast<size_t>(step) < buf.size())
                {
                    float y = buf[static_cast<size_t>(step)];
                    ui::SetTooltip("(%.2fs, %.4f)", static_cast<float>(timeLoc.time_since_epoch().count()), y);
                }
            }

            // if the user presses their left mouse while hovering over the plot,
            // change the current sim scrub time to match their press location
            if (ui::IsMouseDown(ImGuiMouseButton_Left))
            {
                m_API->setSimulationScrubTime(timeLoc);
            }
        }
    }

    ISimulatorUIAPI* m_API;
    OutputExtractor m_OutputExtractor;
    float m_Height;
};


// public API

osc::SimulationOutputPlot::SimulationOutputPlot(ISimulatorUIAPI* api, OutputExtractor outputExtractor, float height) :
    m_Impl{std::make_unique<Impl>(api, std::move(outputExtractor), height)}
{
}

osc::SimulationOutputPlot::SimulationOutputPlot(SimulationOutputPlot&&) noexcept = default;
osc::SimulationOutputPlot& osc::SimulationOutputPlot::operator=(SimulationOutputPlot&&) noexcept = default;
osc::SimulationOutputPlot::~SimulationOutputPlot() noexcept = default;

void osc::SimulationOutputPlot::onDraw()
{
    m_Impl->onDraw();
}

std::filesystem::path osc::TryPromptAndSaveOutputsAsCSV(ISimulatorUIAPI& api, std::span<OutputExtractor const> outputs)
{
    return TryExportOutputsToCSV(api.updSimulation(), outputs);
}

std::filesystem::path osc::TryPromptAndSaveAllUserDesiredOutputsAsCSV(ISimulatorUIAPI& api)
{
    auto outputs = GetAllUserDesiredOutputs(api);
    return TryExportOutputsToCSV(api.updSimulation(), outputs);
}
