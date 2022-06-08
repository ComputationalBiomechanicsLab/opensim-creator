#include "SimulationOutputPlot.hpp"

#include "src/MiddlewareAPIs/SimulatorUIAPI.hpp"
#include "src/OpenSimBindings/Simulation.hpp"
#include "src/OpenSimBindings/OutputExtractor.hpp"
#include "src/OpenSimBindings/VirtualSimulation.hpp"
#include "src/Platform/App.hpp"
#include "src/Platform/os.hpp"
#include "src/Utils/Assertions.hpp"
#include "src/Utils/Perf.hpp"

#include <imgui.h>
#include <implot.h>
#include <IconsFontAwesome5.h>
#include <nonstd/span.hpp>
#include <OpenSim/Simulation/Model/Model.h>

#include <memory>
#include <utility>

static std::vector<osc::OutputExtractor> GetAllUserDesiredOutputs(osc::SimulatorUIAPI& api)
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
static std::string ExportTimeseriesToCSV(
    float const* ts,    // times
    float const* vs,    // values @ each time in times
    size_t n,           // number of datapoints
    char const* header)  // name of values (header)
{
    // try prompt user for save location
    std::filesystem::path p =
        osc::PromptUserForFileSaveLocationAndAddExtensionIfNecessary("csv");

    if (p.empty())
    {
        // user probably cancelled out
        return "";
    }

    std::ofstream fout{p};

    if (!fout)
    {
        osc::log::error("%s: error opening file for writing", p.string().c_str());
        return "";  // error opening output file for writing
    }

    fout << "time," << header << '\n';
    for (size_t i = 0; i < n; ++i)
    {
        fout << ts[i] << ',' << vs[i] << '\n';
    }

    if (!fout)
    {
        osc::log::error("%s: error encountered while writing CSV data to file", p.string().c_str());
        return "";  // error writing
    }

    osc::log::info("%: successfully wrote CSV data to output file", p.string().c_str());

    return p.string();
}

static std::vector<float> PopulateFirstNNumericOutputValues(
    OpenSim::Model const& model,
    nonstd::span<osc::SimulationReport const> reports,
    osc::VirtualOutputExtractor const& output)
{
    std::vector<float> rv;
    rv.resize(reports.size());
    output.getValuesFloat(model, reports, rv);
    return rv;
}

static std::vector<float> PopulateFirstNTimeValues(nonstd::span<osc::SimulationReport const> reports)
{
    std::vector<float> times;
    times.reserve(reports.size());
    for (osc::SimulationReport const& r : reports)
    {
        times.push_back(static_cast<float>(r.getState().getTime()));
    }
    return times;
}

static std::string TryExportNumericOutputToCSV(osc::VirtualSimulation& sim, osc::VirtualOutputExtractor const& output)
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

static void DrawGenericNumericOutputContextMenuItems(osc::VirtualSimulation& sim, osc::VirtualOutputExtractor const& output)
{
    OSC_ASSERT(output.getOutputType() == osc::OutputType::Float);

    if (ImGui::MenuItem(ICON_FA_SAVE "Save as CSV"))
    {
        TryExportNumericOutputToCSV(sim, output);
    }
    else if (ImGui::MenuItem(ICON_FA_SAVE "Save as CSV (and open)"))
    {
        std::string p = TryExportNumericOutputToCSV(sim, output);
        if (!p.empty())
        {
            osc::OpenPathInOSDefaultApplication(p);
        }
    }
}

static std::filesystem::path TryExportOutputsToCSV(osc::VirtualSimulation& sim, nonstd::span<osc::OutputExtractor> outputs)
{
    std::vector<osc::SimulationReport> reports = sim.getAllSimulationReports();
    std::vector<float> times = PopulateFirstNTimeValues(reports);

    // try prompt user for save location
    std::filesystem::path p =
        osc::PromptUserForFileSaveLocationAndAddExtensionIfNecessary("csv");

    if (p.empty())
    {
        // user probably cancelled out
        return "";
    }

    std::ofstream fout{p};

    if (!fout)
    {
        osc::log::error("%s: error opening file for writing", p.string().c_str());
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
        osc::log::warn("%s: encountered error while writing output data: some of the data may have been written, but maybe not all of it", p.string().c_str());
    }

    return p;
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
		m_FrameCountOnLastDrawcall = osc::App::get().getFrameCount();

		VirtualSimulation& sim = m_API->updSimulation();

		int nReports = sim.getNumReports();
		osc::OutputType outputType = m_OutputExtractor.getOutputType();

		if (nReports <= 0)
		{
			ImGui::Text("no data (yet)");
		}
		else if (outputType == osc::OutputType::Float)
		{
			ImGui::SetNextItemWidth(ImGui::GetContentRegionAvailWidth());
			drawFloatOutputPlot(sim);
		}
		else if (outputType == osc::OutputType::String)
		{
			osc::SimulationReport r = m_API->trySelectReportBasedOnScrubbing().value_or(sim.getSimulationReport(nReports - 1));
			ImGui::TextUnformatted(m_OutputExtractor.getValueString(*sim.getModel(), r).c_str());
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
        int nReports = sim.getNumReports();
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
        float const plotWidth = ImGui::GetContentRegionAvailWidth();
        glm::vec2 plotTopLeft{};
        glm::vec2 plotBottomRight{};

        {
            OSC_PERF("draw output plot");

            ImPlot::PushStyleVar(ImPlotStyleVar_PlotPadding, ImVec2(0,0));
            ImPlot::PushStyleVar(ImPlotStyleVar_PlotBorderSize, 0.0f);
            ImPlot::PushStyleVar(ImPlotStyleVar_FitPadding, ImVec2(0,1));

            if (ImPlot::BeginPlot("##", ImVec2(plotWidth, m_Height), ImPlotFlags_NoTitle | ImPlotFlags_AntiAliased | ImPlotFlags_NoLegend | ImPlotFlags_NoInputs | ImPlotFlags_NoMenus | ImPlotFlags_NoBoxSelect | ImPlotFlags_NoChild | ImPlotFlags_NoFrame))
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
            DrawGenericNumericOutputContextMenuItems(sim, m_OutputExtractor);
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
	std::uint64_t m_FrameCountOnLastDrawcall = 0;
};


// public API

osc::SimulationOutputPlot::SimulationOutputPlot(SimulatorUIAPI* api, OutputExtractor outputExtractor, float height) :
	m_Impl{new Impl{std::move(api), std::move(outputExtractor), std::move(height)}}
{
}

osc::SimulationOutputPlot::SimulationOutputPlot(SimulationOutputPlot&& tmp) noexcept :
	m_Impl{std::exchange(tmp.m_Impl, nullptr)}
{
}

osc::SimulationOutputPlot& osc::SimulationOutputPlot::operator=(SimulationOutputPlot && tmp) noexcept
{
	std::swap(m_Impl, tmp.m_Impl);
	return *this;
}

osc::SimulationOutputPlot::~SimulationOutputPlot() noexcept
{
	delete m_Impl;
}

void osc::SimulationOutputPlot::draw()
{
	m_Impl->draw();
}

std::filesystem::path osc::TryPromptAndSaveAllUserDesiredOutputsAsCSV(SimulatorUIAPI& api)
{
    auto outputs = GetAllUserDesiredOutputs(api);
    return TryExportOutputsToCSV(api.updSimulation(), outputs);
}
