#include "SimulationOutputPlot.hpp"

#include "OpenSimCreator/MiddlewareAPIs/SimulatorUIAPI.hpp"
#include "OpenSimCreator/SimulationClock.hpp"
#include "OpenSimCreator/SimulationReport.hpp"
#include "OpenSimCreator/OutputExtractor.hpp"
#include "OpenSimCreator/VirtualSimulation.hpp"
#include "OpenSimCreator/VirtualOutputExtractor.hpp"

#include <oscar/Bindings/ImGuiHelpers.hpp>
#include <oscar/Platform/Log.hpp>
#include <oscar/Platform/os.hpp>
#include <oscar/Utils/Assertions.hpp>
#include <oscar/Utils/Perf.hpp>
#include <oscar/Utils/SynchronizedValue.hpp>

#include <glm/vec2.hpp>
#include <imgui.h>
#include <implot.h>
#include <IconsFontAwesome5.h>
#include <nonstd/span.hpp>
#include <OpenSim/Simulation/Model/Model.h>
#include <SimTKcommon.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <chrono>
#include <optional>
#include <ostream>
#include <ratio>
#include <string>
#include <utility>
#include <vector>

namespace
{
    std::vector<osc::OutputExtractor> GetAllUserDesiredOutputs(osc::SimulatorUIAPI& api)
    {
        int nOutputs = api.getNumUserOutputExtractors();

        std::vector<osc::OutputExtractor> rv;
        rv.reserve(nOutputs);
        for (int i = 0; i < nOutputs; ++i)
        {
            rv.push_back(api.getUserOutputExtractor(i));
        }
        return rv;
    }

    // export a timeseries to a CSV file and return the filepath
    std::string ExportTimeseriesToCSV(
        float const* ts,    // times
        float const* vs,    // values @ each time in times
        size_t n,           // number of datapoints
        char const* header)  // name of values (header)
    {
        // try prompt user for save location
        std::optional<std::filesystem::path> const maybeCSVPath =
            osc::PromptUserForFileSaveLocationAndAddExtensionIfNecessary("csv");

        if (!maybeCSVPath)
        {
            return "";  // user probably cancelled out
        }

        std::filesystem::path const& csvPath = *maybeCSVPath;

        std::ofstream fout{csvPath};

        if (!fout)
        {
            osc::log::error("%s: error opening file for writing", csvPath.string().c_str());
            return "";  // error opening output file for writing
        }

        fout << "time," << header << '\n';
        for (size_t i = 0; i < n; ++i)
        {
            fout << ts[i] << ',' << vs[i] << '\n';
        }

        if (!fout)
        {
            osc::log::error("%s: error encountered while writing CSV data to file", csvPath.string().c_str());
            return "";  // error writing
        }

        osc::log::info("%: successfully wrote CSV data to output file", csvPath.string().c_str());

        return csvPath.string();
    }

    std::vector<float> PopulateFirstNNumericOutputValues(
        OpenSim::Model const& model,
        nonstd::span<osc::SimulationReport const> reports,
        osc::VirtualOutputExtractor const& output)
    {
        std::vector<float> rv;
        rv.resize(reports.size());
        output.getValuesFloat(model, reports, rv);
        return rv;
    }

    std::vector<float> PopulateFirstNTimeValues(nonstd::span<osc::SimulationReport const> reports)
    {
        std::vector<float> times;
        times.reserve(reports.size());
        for (osc::SimulationReport const& r : reports)
        {
            times.push_back(static_cast<float>(r.getState().getTime()));
        }
        return times;
    }

    std::string TryExportNumericOutputToCSV(
        osc::VirtualSimulation& sim,
        osc::VirtualOutputExtractor const& output)
    {
        OSC_ASSERT(output.getOutputType() == osc::OutputType::Float);

        std::vector<osc::SimulationReport> reports = sim.getAllSimulationReports();
        std::vector<float> values = PopulateFirstNNumericOutputValues(*sim.getModel(), reports, output);
        std::vector<float> times = PopulateFirstNTimeValues(reports);

        return ExportTimeseriesToCSV(times.data(),
            values.data(),
            times.size(),
            output.getName().c_str());
    }

    void DrawToggleWatchOutputMenuItem(
        osc::SimulatorUIAPI& api,
        osc::OutputExtractor const& output)
    {
        bool isWatching = api.hasUserOutputExtractor(output);

        if (ImGui::MenuItem(ICON_FA_EYE " Watch Output", nullptr, &isWatching))
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
        osc::DrawTooltipIfItemHovered("Watch Output", "Watch the selected output. This makes it appear in the 'Output Watches' window in the editor panel and the 'Output Plots' window during a simulation");
    }

    void DrawGenericNumericOutputContextMenuItems(
        osc::SimulatorUIAPI& api,
        osc::VirtualSimulation& sim,
        osc::OutputExtractor const& output)
    {
        OSC_ASSERT(output.getOutputType() == osc::OutputType::Float);

        if (ImGui::MenuItem(ICON_FA_SAVE "Save as CSV"))
        {
            TryExportNumericOutputToCSV(sim, output);
        }

        if (ImGui::MenuItem(ICON_FA_SAVE "Save as CSV (and open)"))
        {
            std::string p = TryExportNumericOutputToCSV(sim, output);
            if (!p.empty())
            {
                osc::OpenPathInOSDefaultApplication(p);
            }
        }

        DrawToggleWatchOutputMenuItem(api, output);
    }

    std::filesystem::path TryExportOutputsToCSV(osc::VirtualSimulation& sim, nonstd::span<osc::OutputExtractor const> outputs)
    {
        std::vector<osc::SimulationReport> reports = sim.getAllSimulationReports();
        std::vector<float> times = PopulateFirstNTimeValues(reports);

        // try prompt user for save location
        std::optional<std::filesystem::path> const maybeCSVPath =
            osc::PromptUserForFileSaveLocationAndAddExtensionIfNecessary("csv");

        if (!maybeCSVPath)
        {
            // user probably cancelled out
            return "";
        }
        std::filesystem::path const& csvPath = *maybeCSVPath;

        std::ofstream fout{csvPath};

        if (!fout)
        {
            osc::log::error("%s: error opening file for writing", csvPath.string().c_str());
            return "";  // error opening output file for writing
        }

        // header line
        fout << "time";
        for (osc::OutputExtractor const& o : outputs)
        {
            fout << ',' << o.getName();
        }
        fout << '\n';


        // data lines
        auto guard = sim.getModel();
        for (size_t i = 0; i < reports.size(); ++i)
        {
            fout << times.at(i);  // time column

            osc::SimulationReport r = reports[i];
            for (osc::OutputExtractor const& o : outputs)
            {
                fout << ',' << o.getValueFloat(*guard, r);
            }

            fout << '\n';
        }

        if (!fout)
        {
            osc::log::warn("%s: encountered error while writing output data: some of the data may have been written, but maybe not all of it", csvPath.string().c_str());
        }

        return csvPath;
    }
}

class osc::SimulationOutputPlot::Impl final {
public:

    Impl(SimulatorUIAPI* api, OutputExtractor outputExtractor, float height) :
        m_API{std::move(api)},
        m_OutputExtractor{std::move(outputExtractor)},
        m_Height{std::move(height)}
    {
    }

    void draw()
    {
        VirtualSimulation& sim = m_API->updSimulation();

        ptrdiff_t const nReports = sim.getNumReports();
        osc::OutputType outputType = m_OutputExtractor.getOutputType();

        if (nReports <= 0)
        {
            ImGui::Text("no data (yet)");
        }
        else if (outputType == osc::OutputType::Float)
        {
            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
            drawFloatOutputPlot(sim);
        }
        else if (outputType == osc::OutputType::String)
        {
            osc::SimulationReport r = m_API->trySelectReportBasedOnScrubbing().value_or(sim.getSimulationReport(nReports - 1));
            ImGui::TextUnformatted(m_OutputExtractor.getValueString(*sim.getModel(), r).c_str());

            // draw context menu (if user right clicks)
            if (ImGui::BeginPopupContextItem("plotcontextmenu"))
            {
                DrawToggleWatchOutputMenuItem(*m_API, m_OutputExtractor);
                ImGui::EndPopup();
            }
        }
        else
        {
            ImGui::Text("unknown output type");
        }
    }

private:
    void drawFloatOutputPlot(VirtualSimulation& sim)
    {
        OSC_ASSERT(m_OutputExtractor.getOutputType() == osc::OutputType::Float);

        ImU32 const currentTimeLineColor = ImGui::ColorConvertFloat4ToU32({1.0f, 1.0f, 0.0f, 0.6f});
        ImU32 const hoverTimeLineColor = ImGui::ColorConvertFloat4ToU32({1.0f, 1.0f, 0.0f, 0.3f});

        // collect data
        ptrdiff_t const nReports = sim.getNumReports();
        if (nReports <= 0)
        {
            ImGui::Text("no data (yet)");
            return;
        }

        std::vector<float> buf;
        {
            OSC_PERF("collect output data");
            std::vector<osc::SimulationReport> reports = sim.getAllSimulationReports();
            buf.resize(reports.size());
            m_OutputExtractor.getValuesFloat(*sim.getModel(), reports, buf);
        }

        // draw plot
        float const plotWidth = ImGui::GetContentRegionAvail().x;
        glm::vec2 plotTopLeft{};
        glm::vec2 plotBottomRight{};

        {
            OSC_PERF("draw output plot");

            ImPlot::PushStyleVar(ImPlotStyleVar_PlotPadding, ImVec2(0,0));
            ImPlot::PushStyleVar(ImPlotStyleVar_PlotBorderSize, 0.0f);
            ImPlot::PushStyleVar(ImPlotStyleVar_FitPadding, ImVec2(0,1));

            if (ImPlot::BeginPlot("##", ImVec2(plotWidth, m_Height), ImPlotFlags_NoTitle | ImPlotFlags_NoLegend | ImPlotFlags_NoInputs | ImPlotFlags_NoMenus | ImPlotFlags_NoBoxSelect | ImPlotFlags_NoChild | ImPlotFlags_NoFrame))
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
                plotBottomRight = plotTopLeft + glm::vec2{ImPlot::GetPlotSize()};

                ImPlot::EndPlot();
            }
            ImPlot::PopStyleVar();
            ImPlot::PopStyleVar();
            ImPlot::PopStyleVar();
        }


        // draw context menu (if user right clicks)
        if (ImGui::BeginPopupContextItem("plotcontextmenu"))
        {
            DrawGenericNumericOutputContextMenuItems(*m_API, sim, m_OutputExtractor);
            ImGui::EndPopup();
        }

        // (the rest): handle scrubber overlay
        OSC_PERF("draw output plot overlay");

        // figure out mapping between screen space and plot space

        osc::SimulationClock::time_point simStartTime = sim.getSimulationReport(0).getTime();
        osc::SimulationClock::time_point simEndTime = sim.getSimulationReport(nReports-1).getTime();
        osc::SimulationClock::duration simTimeStep = (simEndTime-simStartTime)/nReports;
        osc::SimulationClock::time_point simScrubTime = m_API->getSimulationScrubTime();

        float simScrubPct = static_cast<float>(static_cast<double>((simScrubTime - simStartTime)/(simEndTime - simStartTime)));

        ImDrawList* drawlist = ImGui::GetWindowDrawList();

        // draw a vertical Y line showing the current scrub time over the plots
        {
            float plotScrubLineX = plotTopLeft.x + simScrubPct*(plotBottomRight.x - plotTopLeft.x);
            glm::vec2 p1 = {plotScrubLineX, plotBottomRight.y};
            glm::vec2 p2 = {plotScrubLineX, plotTopLeft.y};
            drawlist->AddLine(p1, p2, currentTimeLineColor);
        }

        if (ImGui::IsItemHovered())
        {
            glm::vec2 mp = ImGui::GetMousePos();
            glm::vec2 plotLoc = mp - plotTopLeft;
            float relLoc = plotLoc.x / (plotBottomRight.x - plotTopLeft.x);
            osc::SimulationClock::time_point timeLoc = simStartTime + relLoc*(simEndTime - simStartTime);

            // draw vertical line to show current X of their hover
            {
                glm::vec2 p1 = {mp.x, plotBottomRight.y};
                glm::vec2 p2 = {mp.x, plotTopLeft.y};
                drawlist->AddLine(p1, p2, hoverTimeLineColor);
            }

            // show a tooltip of X and Y
            {
                int step = static_cast<int>((timeLoc - simStartTime) / simTimeStep);
                if (0 <= step && static_cast<size_t>(step) < buf.size())
                {
                    float y = buf[static_cast<size_t>(step)];
                    ImGui::SetTooltip("(%.2fs, %.4f)", static_cast<float>(timeLoc.time_since_epoch().count()), y);
                }
            }

            // if the user presses their left mouse while hovering over the plot,
            // change the current sim scrub time to match their press location
            if (ImGui::IsMouseDown(ImGuiMouseButton_Left))
            {
                m_API->setSimulationScrubTime(timeLoc);
            }
        }
    }

    SimulatorUIAPI* m_API;
    OutputExtractor m_OutputExtractor;
    float m_Height;
};


// public API

osc::SimulationOutputPlot::SimulationOutputPlot(SimulatorUIAPI* api, OutputExtractor outputExtractor, float height) :
    m_Impl{std::make_unique<Impl>(std::move(api), std::move(outputExtractor), std::move(height))}
{
}

osc::SimulationOutputPlot::SimulationOutputPlot(SimulationOutputPlot&&) noexcept = default;
osc::SimulationOutputPlot& osc::SimulationOutputPlot::operator=(SimulationOutputPlot&&) noexcept = default;
osc::SimulationOutputPlot::~SimulationOutputPlot() noexcept = default;

void osc::SimulationOutputPlot::draw()
{
    m_Impl->draw();
}

std::filesystem::path osc::TryPromptAndSaveOutputsAsCSV(SimulatorUIAPI& api, nonstd::span<OutputExtractor const> outputs)
{
    return TryExportOutputsToCSV(api.updSimulation(), outputs);
}

std::filesystem::path osc::TryPromptAndSaveAllUserDesiredOutputsAsCSV(SimulatorUIAPI& api)
{
    auto outputs = GetAllUserDesiredOutputs(api);
    return TryExportOutputsToCSV(api.updSimulation(), outputs);
}
