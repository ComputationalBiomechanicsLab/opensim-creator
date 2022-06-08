#include "SimulatorTab.hpp"

#include "src/Bindings/ImGuiHelpers.hpp"
#include "src/Maths/BVH.hpp"
#include "src/MiddlewareAPIs/MainUIStateAPI.hpp"
#include "src/OpenSimBindings/ComponentOutputExtractor.hpp"
#include "src/OpenSimBindings/ParamBlock.hpp"
#include "src/OpenSimBindings/OpenSimHelpers.hpp"
#include "src/OpenSimBindings/ComponentDecoration.hpp"
#include "src/OpenSimBindings/OpenSimHelpers.hpp"
#include "src/OpenSimBindings/OutputExtractor.hpp"
#include "src/OpenSimBindings/Simulation.hpp"
#include "src/OpenSimBindings/SimulationClock.hpp"
#include "src/OpenSimBindings/SimulationModelStatePair.hpp"
#include "src/OpenSimBindings/SimulationReport.hpp"
#include "src/OpenSimBindings/UndoableModelStatePair.hpp"
#include "src/OpenSimBindings/VirtualOutputExtractor.hpp"
#include "src/OpenSimBindings/VirtualSimulation.hpp"
#include "src/Platform/App.hpp"
#include "src/Platform/Config.hpp"
#include "src/Platform/os.hpp"
#include "src/Platform/Styling.hpp"
#include "src/Tabs/ModelEditorTab.hpp"
#include "src/Tabs/TabHost.hpp"
#include "src/Utils/Assertions.hpp"
#include "src/Utils/ScopeGuard.hpp"
#include "src/Utils/Perf.hpp"
#include "src/Widgets/BasicWidgets.hpp"
#include "src/Widgets/LogViewer.hpp"
#include "src/Widgets/MainMenu.hpp"
#include "src/Widgets/ComponentDetails.hpp"
#include "src/Widgets/ModelHierarchyPanel.hpp"
#include "src/Widgets/PerfPanel.hpp"
#include "src/Widgets/UiModelViewer.hpp"

#include <OpenSim/Simulation/Model/Model.h>
#include <OpenSim/Common/ComponentOutput.h>
#include <imgui.h>
#include <implot/implot.h>
#include <IconsFontAwesome5.h>
#include <nonstd/span.hpp>
#include <SDL_events.h>

#include <atomic>
#include <chrono>
#include <fstream>
#include <filesystem>
#include <limits>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <variant>
#include <vector>


static std::atomic<int> g_SimulationNumber = 1;


static std::vector<osc::OutputExtractor> GetAllUserDesiredOutputs(osc::MainUIStateAPI& api)
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

static std::string TryExportNumericOutputToCSV(osc::Simulation& sim,
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

static std::string TryExportOutputsToCSV(osc::Simulation& sim,
    std::vector<osc::OutputExtractor> outputs)
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

    return p.string();
}

static void DrawGenericNumericOutputContextMenuItems(osc::Simulation& sim, osc::VirtualOutputExtractor const& output)
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

static void DrawOutputNameColumn(osc::VirtualOutputExtractor const& output, bool centered = true)
{
    if (centered)
    {
        osc::TextCentered(output.getName());
    }
    else
    {
        ImGui::TextUnformatted(output.getName().c_str());
    }

    if (!output.getDescription().empty())
    {
        ImGui::SameLine();
        osc::DrawHelpMarker(output.getName().c_str(), output.getDescription().c_str());
    }
}

class osc::SimulatorTab::Impl final {
public:
	Impl(MainUIStateAPI* api, std::shared_ptr<Simulation> simulation) :
        m_API{std::move(api)},
        m_Simulation{std::move(simulation)}
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
        return m_API;
	}

	void onMount()
	{
        ImPlot::CreateContext();
        App::upd().makeMainEventLoopWaiting();
	}

	void onUnmount()
	{
        App::upd().makeMainEventLoopPolling();
        ImPlot::DestroyContext();
	}

	bool onEvent(SDL_Event const& e)
	{
        if (e.type == SDL_KEYDOWN)
        {
            return onKeyDown(e.key);
        }
        return false;
	}

	void onTick()
	{
        if (m_IsPlayingBack)
        {
            auto playbackPos = getPlaybackPositionInSimTime(*m_Simulation);
            if (playbackPos < m_Simulation->getEndTime())
            {
                osc::App::upd().requestRedraw();
            }
            else
            {
                m_IsPlayingBack = false;
                return;
            }
        }
	}

	void onDrawMainMenu()
	{
        m_MainMenuFileTab.draw(m_API);
        drawMainMenuWindowTab();
        m_MainMenuAboutTab.draw();
	}

	void onDraw()
	{
        ImGui::DockSpaceOverViewport(ImGui::GetMainViewport(), ImGuiDockNodeFlags_PassthruCentralNode);
        drawContent();
	}

private:
    void drawContent()
    {
        OSC_PERF("draw simulation screen");

        {
            OSC_PERF("draw simulator panels");
            drawAll3DViewers();
        }

        // ensure m_ShownModelState is populated, if possible
        {
            std::optional<osc::SimulationReport> maybeReport = TrySelectReportBasedOnScrubbing(*m_Simulation);
            if (maybeReport)
            {
                if (m_ShownModelState)
                {
                    m_ShownModelState->setSimulation(m_Simulation);
                    m_ShownModelState->setSimulationReport(*maybeReport);
                }
                else
                {
                    m_ShownModelState = std::make_unique<osc::SimulationModelStatePair>(m_Simulation, *maybeReport);
                }
            }
        }

        osc::Config const& config = osc::App::get().getConfig();

        // draw hierarchy panel
        {
            if (!m_ShownModelState)
            {
                ImGui::TextDisabled("(no simulation selected)");
                return;
            }

            osc::SimulationModelStatePair& ms = *m_ShownModelState;

            auto resp = m_ModelHierarchyPanel.draw(ms);

            if (resp.type == osc::ModelHierarchyPanel::ResponseType::SelectionChanged)
            {
                ms.setSelected(resp.ptr);
            }
            else if (resp.type == osc::ModelHierarchyPanel::ResponseType::HoverChanged)
            {
                ms.setHovered(resp.ptr);
            }
        }

        // draw selection details panel
        if (bool selectionDetailsOldState = config.getIsPanelEnabled("Selection Details"))
        {
            bool selectionDetailsState = selectionDetailsOldState;
            if (ImGui::Begin("Selection Details", &selectionDetailsState))
            {
                OSC_PERF("draw selection panel");
                drawSelectionTab();
            }
            ImGui::End();

            if (selectionDetailsState != selectionDetailsOldState)
            {
                App::upd().updConfig().setIsPanelEnabled("Selection Details", selectionDetailsState);
            }
        }

        if (bool outputsPanelOldState = config.getIsPanelEnabled("Outputs"))
        {
            bool outputsPanelState = outputsPanelOldState;
            if (ImGui::Begin("Outputs", &outputsPanelState))
            {
                OSC_PERF("draw outputs panel");
                drawOutputsTab();
            }
            ImGui::End();

            if (outputsPanelState != outputsPanelOldState)
            {
                App::upd().updConfig().setIsPanelEnabled("Outputs", outputsPanelState);
            }
        }

        // simulation details
        if (bool simulationStatsOldState = config.getIsPanelEnabled("Simulation Details"))
        {
            bool simulationStatsState = simulationStatsOldState;
            if (ImGui::Begin("Simulation Details", &simulationStatsState))
            {
                OSC_PERF("draw simulation details panel");
                drawSimulationStats();
            }
            ImGui::End();

            if (simulationStatsState != simulationStatsOldState)
            {
                App::upd().updConfig().setIsPanelEnabled("Simulation Details", simulationStatsState);
            }
        }

        if (bool logOldState = config.getIsPanelEnabled("Log"))
        {
            bool logState = logOldState;
            if (ImGui::Begin("Log", &logState, ImGuiWindowFlags_MenuBar))
            {
                OSC_PERF("draw log panel");
                m_LogViewerWidget.draw();
            }
            ImGui::End();

            if (logState != logOldState)
            {
                App::upd().updConfig().setIsPanelEnabled("Log", logState);
            }
        }

        if (bool perfPanelOldState = config.getIsPanelEnabled("Performance"))
        {
            OSC_PERF("draw perf panel");
            m_PerfPanel.open();
            bool state = m_PerfPanel.draw();

            if (state != perfPanelOldState)
            {
                App::upd().updConfig().setIsPanelEnabled("Performance", state);
            }
        }
    }

    void drawOutputsTab()
    {
        int numOutputs = m_API->getNumUserOutputExtractors();

        if (numOutputs <= 0)
        {
            ImGui::TextDisabled("(no outputs requested)");
            return;
        }

        ImGui::Button(ICON_FA_SAVE " Save All " ICON_FA_CARET_DOWN);
        if (ImGui::BeginPopupContextItem("##exportoptions", ImGuiPopupFlags_MouseButtonLeft))
        {
            if (ImGui::MenuItem("as CSV"))
            {
                TryExportOutputsToCSV(*m_Simulation, GetAllUserDesiredOutputs(*m_API));
            }

            if (ImGui::MenuItem("as CSV (and open)"))
            {
                std::string path = TryExportOutputsToCSV(*m_Simulation, GetAllUserDesiredOutputs(*m_API));
                if (!path.empty())
                {
                    osc::OpenPathInOSDefaultApplication(path);
                }
            }

            ImGui::EndPopup();
        }

        ImGui::Separator();
        ImGui::Dummy({0.0f, 5.0f});

        for (int i = 0; i < numOutputs; ++i)
        {
            osc::OutputExtractor const& output = m_API->getUserOutputExtractor(i);

            ImGui::PushID(i);
            drawOutputDataColumn(*m_Simulation, output, 64.0f);
            DrawOutputNameColumn(output, true);
            ImGui::PopID();
        }
    }

    void drawSelectionTab()
    {
        if (!m_ShownModelState)
        {
            ImGui::TextDisabled("(no simulation selected)");
            return;
        }

        osc::SimulationModelStatePair& ms = *m_ShownModelState;

        OpenSim::Component const* selected = ms.getSelected();

        if (!selected)
        {
            ImGui::TextDisabled("(nothing selected)");
            return;
        }

        m_ComponentDetailsWidget.draw(ms.getState(), selected);

        if (ImGui::CollapsingHeader("outputs"))
        {
            int imguiID = 0;
            ImGui::Columns(2);
            for (auto const& [outputName, aoPtr] : selected->getOutputs())
            {
                ImGui::PushID(imguiID++);

                ImGui::Text("%s", outputName.c_str());
                ImGui::NextColumn();
                osc::ComponentOutputExtractor output{*aoPtr};
                drawOutputDataColumn(*ms.updSimulation(), output, ImGui::GetTextLineHeight());
                ImGui::NextColumn();

                ImGui::PopID();
            }
            ImGui::Columns();
        }
    }

    void drawSimulatorTab()
    {
        float progress = m_Simulation->getProgress();
        ImVec4 baseColor = progress >= 1.0f ? ImVec4{0.0f, 0.7f, 0.0f, 0.5f} : ImVec4{0.7f, 0.7f, 0.0f, 0.5f};

        ImGui::SameLine();
        ImGui::PushStyleColor(ImGuiCol_PlotHistogram, baseColor);
        ImGui::ProgressBar(progress);
        ImGui::PopStyleColor();

        if (ImGui::BeginPopupContextItem("simcontextmenu"))
        {
            if (ImGui::MenuItem("edit model"))
            {
                auto uim = std::make_unique<osc::UndoableModelStatePair>(std::make_unique<OpenSim::Model>(*m_Simulation->getModel()));
                UID tabID = m_API->addTab<ModelEditorTab>(m_API, std::move(uim));
                m_API->selectTab(tabID);
            }

            if (ImGui::IsItemHovered())
            {
                ImGui::BeginTooltip();
                ImGui::PushTextWrapPos(ImGui::GetFontSize() + 400.0f);
                ImGui::TextUnformatted("Make the model initially used in this simulation into the model being edited in the editor");
                ImGui::PopTextWrapPos();
                ImGui::EndTooltip();
            }

            ImGui::EndPopup();
        }
    }

    void drawSimulationStats()
    {
        {
            ImGui::Dummy({0.0f, 1.0f});
            ImGui::TextUnformatted("info:");
            ImGui::SameLine();
            osc::DrawHelpMarker("Top-level info about the simulation");
            ImGui::Separator();
            ImGui::Dummy({0.0f, 2.0f});

            ImGui::Columns(2);
            ImGui::Text("num reports");
            ImGui::NextColumn();
            ImGui::Text("%i", m_Simulation->getNumReports());
            ImGui::NextColumn();
            ImGui::Columns();
        }

        {
            OSC_PERF("draw simulation params");
            DrawSimulationParams(m_Simulation->getParams());
        }

        ImGui::Dummy({0.0f, 10.0f});

        {
            OSC_PERF("draw simulation stats");
            drawSimulationStatPlots(*m_Simulation);
        }
    }

    void drawSimulationStatPlots(osc::Simulation& sim)
    {
        auto outputs = sim.getOutputs();

        if (outputs.empty())
        {
            ImGui::TextDisabled("(no simulator output plots available for this simulation)");
            return;
        }

        ImGui::Dummy({0.0f, 1.0f});
        ImGui::Columns(2);
        ImGui::TextUnformatted("plots:");
        ImGui::SameLine();
        osc::DrawHelpMarker("Various statistics collected when the simulation was ran");
        ImGui::NextColumn();
        if (std::any_of(outputs.begin(), outputs.end(), [](osc::OutputExtractor const& o) { return o.getOutputType() == osc::OutputType::Float; }))
        {
            ImGui::Button(ICON_FA_SAVE " Save All " ICON_FA_CARET_DOWN);
            if (ImGui::BeginPopupContextItem("##exportoptions", ImGuiPopupFlags_MouseButtonLeft))
            {
                if (ImGui::MenuItem("as CSV"))
                {
                    TryExportOutputsToCSV(sim, std::vector<osc::OutputExtractor>(outputs.begin(), outputs.end()));
                }

                if (ImGui::MenuItem("as CSV (and open)"))
                {
                    std::string path = TryExportOutputsToCSV(sim, std::vector<osc::OutputExtractor>(outputs.begin(), outputs.end()));
                    if (!path.empty())
                    {
                        osc::OpenPathInOSDefaultApplication(path);
                    }
                }

                ImGui::EndPopup();
            }
        }

        ImGui::NextColumn();
        ImGui::Columns();
        ImGui::Separator();
        ImGui::Dummy({0.0f, 2.0f});

        int imguiID = 0;
        ImGui::Columns(2);
        for (osc::OutputExtractor const& output : sim.getOutputs())
        {
            ImGui::PushID(imguiID++);
            DrawOutputNameColumn(output, false);
            ImGui::NextColumn();
            drawOutputDataColumn(sim, output, 32.0f);
            ImGui::NextColumn();
            ImGui::PopID();
        }
        ImGui::Columns();
    }

    void drawOutputDataColumn(osc::Simulation& sim, osc::VirtualOutputExtractor const& output, float plotHeight)
    {
        int nReports = sim.getNumReports();
        osc::OutputType outputType = output.getOutputType();

        if (nReports <= 0)
        {
            ImGui::Text("no data (yet)");
        }
        else if (outputType == osc::OutputType::Float)
        {
            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvailWidth());
            drawNumericOutputPlot(sim, output, plotHeight);
        }
        else if (outputType == osc::OutputType::String)
        {
            osc::SimulationReport r = TrySelectReportBasedOnScrubbing(sim).value_or(sim.getSimulationReport(nReports-1));
            ImGui::TextUnformatted(output.getValueString(*sim.getModel(), r).c_str());
        }
    }

    void drawNumericOutputPlot(osc::Simulation& sim, osc::VirtualOutputExtractor const& output, float plotHeight)
    {
        OSC_ASSERT(output.getOutputType() == osc::OutputType::Float);

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
            output.getValuesFloat(*sim.getModel(), reports, buf);
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

            if (ImPlot::BeginPlot("##", ImVec2(plotWidth, plotHeight), ImPlotFlags_NoTitle | ImPlotFlags_AntiAliased | ImPlotFlags_NoLegend | ImPlotFlags_NoInputs | ImPlotFlags_NoMenus | ImPlotFlags_NoBoxSelect | ImPlotFlags_NoChild | ImPlotFlags_NoFrame))
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
            DrawGenericNumericOutputContextMenuItems(sim, output);
            ImGui::EndPopup();
        }

        // (the rest): handle scrubber overlay
        OSC_PERF("draw output plot overlay");

        // figure out mapping between screen space and plot space

        osc::SimulationClock::time_point simStartTime = sim.getSimulationReport(0).getTime();
        osc::SimulationClock::time_point simEndTime = sim.getSimulationReport(nReports-1).getTime();
        osc::SimulationClock::duration simTimeStep = (simEndTime-simStartTime)/nReports;
        osc::SimulationClock::time_point simScrubTime = getPlaybackPositionInSimTime(sim);

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
                m_PlaybackStartSimtime = timeLoc;
                m_IsPlayingBack = false;
            }
        }
    }

    // draw timescrubber slider
    void drawSimulationScrubber(osc::Simulation& sim)
    {
        // play/pause buttons
        if (!m_IsPlayingBack)
        {
            if (ImGui::Button(ICON_FA_PLAY))
            {
                m_PlaybackStartWallTime = std::chrono::system_clock::now();
                m_IsPlayingBack = true;
            }
        }
        else
        {
            if (ImGui::Button(ICON_FA_PAUSE))
            {
                m_PlaybackStartSimtime = getPlaybackPositionInSimTime(sim);
                m_IsPlayingBack = false;
            }
        }

        osc::SimulationClock::time_point tStart = sim.getStartTime();
        osc::SimulationClock::time_point tEnd = sim.getEndTime();
        osc::SimulationClock::time_point tCur = getPlaybackPositionInSimTime(sim);

        ImGui::SameLine();
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvailWidth());

        float v = static_cast<float>(tCur.time_since_epoch().count());
        bool userScrubbed = ImGui::SliderFloat("##scrubber",
            &v,
            static_cast<float>(tStart.time_since_epoch().count()),
            static_cast<float>(tEnd.time_since_epoch().count()),
            "%.2f",
            ImGuiSliderFlags_AlwaysClamp);

        if (userScrubbed)
        {
            m_PlaybackStartSimtime = osc::SimulationClock::start() + osc::SimulationClock::duration{static_cast<double>(v)};
            m_PlaybackStartWallTime = std::chrono::system_clock::now();
        }

        if (ImGui::IsItemHovered())
        {
            ImGui::BeginTooltip();
            ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
            ImGui::TextUnformatted("Left-Click: Change simulation time being shown");
            ImGui::TextUnformatted("Ctrl-Click: Type in the simulation time being shown");
            ImGui::PopTextWrapPos();
            ImGui::EndTooltip();
        }
    }

    void drawMainMenuWindowTab()
    {
        static std::vector<std::string> const g_EditorScreenPanels =
        {
            "Hierarchy",
            "Log",
            "Outputs",
            "Selection Details",
            "Simulations",
            "Simulation Details",
            "Performance",
        };

        // draw "window" tab
        if (ImGui::BeginMenu("Window"))
        {
            Config const& cfg = App::get().getConfig();
            for (std::string const& panel : g_EditorScreenPanels)
            {
                bool currentVal = cfg.getIsPanelEnabled(panel);
                if (ImGui::MenuItem(panel.c_str(), nullptr, &currentVal))
                {
                    App::upd().updConfig().setIsPanelEnabled(panel, currentVal);
                }
            }

            ImGui::Separator();

            // active viewers (can be disabled)
            for (int i = 0; i < static_cast<int>(m_ModelViewers.size()); ++i)
            {
                char buf[64];
                std::snprintf(buf, sizeof(buf), "viewer%i", i);

                bool enabled = true;
                if (ImGui::MenuItem(buf, nullptr, &enabled))
                {
                    m_ModelViewers.erase(m_ModelViewers.begin() + i);
                    --i;
                }
            }

            if (ImGui::MenuItem("add viewer"))
            {
                m_ModelViewers.emplace_back();
            }

            ImGui::EndMenu();
        }
    }

    // draw a single 3D model viewer
    bool draw3DViewer(osc::SimulationModelStatePair& ms, osc::UiModelViewer& viewer, char const* name, int i)
    {
        bool isOpen = true;

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {0.0f, 0.0f});
        bool shown = ImGui::Begin(name, &isOpen, ImGuiWindowFlags_MenuBar);
        ImGui::PopStyleVar();

        if (!isOpen)
        {
            ImGui::End();
            return false;  // closed by the user
        }

        if (!shown)
        {
            ImGui::End();
            return true;  // it's open, but not shown
        }

        glm::vec2 pos = ImGui::GetCursorScreenPos();
        glm::vec2 dims = ImGui::GetContentRegionAvail();
        auto resp = viewer.draw(ms);
        ImGui::End();

        // draw scubber overlay
        {
            ImGui::SetNextWindowPos({ pos.x + 100.0f, pos.y + dims.y - 70.0f });
            ImGui::SetNextWindowSize({ dims.x - 110.0f, 100.0f });
            std::string scrubberName = "##scrubber_" + std::to_string(i);
            ImGui::Begin(scrubberName.c_str(), nullptr, osc::GetMinimalWindowFlags() & ~ImGuiWindowFlags_NoInputs);
            drawSimulationScrubber(*ms.updSimulation());
            ImGui::End();
        }

        // upate hover
        if (resp.isMousedOver && resp.hovertestResult != ms.getHovered())
        {
            ms.setHovered(resp.hovertestResult);
            osc::App::upd().requestRedraw();
        }

        // if left-clicked, update selection (can be empty)
        if (resp.isMousedOver && resp.isLeftClicked)
        {
            ms.setSelected(resp.hovertestResult);
            osc::App::upd().requestRedraw();
        }

        // if hovered, draw hover tooltip
        if (resp.isMousedOver && resp.hovertestResult)
        {
            osc::DrawComponentHoverTooltip(*resp.hovertestResult);
        }

        // if right-clicked, draw a context menu
        {
            std::string menuName = std::string{name} + "_contextmenu";

            if (resp.isMousedOver && osc::IsMouseReleasedWithoutDragging(ImGuiMouseButton_Right))
            {
                ms.setSelected(resp.hovertestResult);  // can be empty
                ImGui::OpenPopup(menuName.c_str());
            }

            OpenSim::Component const* selected = ms.getSelected();

            if (selected && ImGui::BeginPopup(menuName.c_str()))
            {
                // draw context menu for whatever's selected
                ImGui::TextUnformatted(selected->getName().c_str());
                ImGui::SameLine();
                ImGui::TextDisabled("%s", selected->getConcreteClassName().c_str());
                ImGui::Separator();
                ImGui::Dummy({0.0f, 3.0f});

                DrawSelectOwnerMenu(ms, *selected);
                DrawRequestOutputsMenu(*m_API, *selected);
                ImGui::EndPopup();
            }
        }

        return true;
    }

    // draw all active 3D viewers
    //
    // the user can (de)activate 3D viewers in the "Window" tab
    void drawAll3DViewers()
    {
        if (!m_ShownModelState)
        {
            if (ImGui::Begin("render"))
            {
                ImGui::TextDisabled("(no simulation data available)");
            }
            ImGui::End();
            return;
        }

        osc::SimulationModelStatePair& ms = *m_ShownModelState;

        for (int i = 0; i < static_cast<int>(m_ModelViewers.size()); ++i)
        {
            osc::UiModelViewer& viewer = m_ModelViewers[i];

            char buf[64];
            std::snprintf(buf, sizeof(buf), "viewer%i", i);

            bool isOpen = draw3DViewer(ms, viewer, buf, i);

            if (!isOpen)
            {
                m_ModelViewers.erase(m_ModelViewers.begin() + i);
                --i;
            }
        }
    }

    // returns the playback position, which changes based on the wall clock (it's a playback)
    // onto the time within a simulation
    osc::SimulationClock::time_point getPlaybackPositionInSimTime(osc::VirtualSimulation const& sim)
    {
        if (!m_IsPlayingBack)
        {
            return m_PlaybackStartSimtime;
        }
        else
        {
            // map wall time onto sim time

            int nReports = sim.getNumReports();
            if (nReports <= 0)
            {
                return osc::SimulationClock::start();
            }
            else
            {
                std::chrono::system_clock::time_point wallNow = std::chrono::system_clock::now();
                auto wallDur = wallNow - m_PlaybackStartWallTime;

                osc::SimulationClock::time_point simNow = m_PlaybackStartSimtime + osc::SimulationClock::duration{wallDur};
                osc::SimulationClock::time_point simLatest = sim.getSimulationReport(nReports-1).getTime();

                return simNow <= simLatest ? simNow : simLatest;
            }
        }
    }

    std::optional<osc::SimulationReport> TryLookupReportBasedOnScrubbing(osc::VirtualSimulation& sim)
    {
        int nReports = sim.getNumReports();

        if (nReports <= 0)
        {
            return std::nullopt;
        }

        osc::SimulationClock::time_point t = getPlaybackPositionInSimTime(sim);

        for (int i = 0; i < nReports; ++i)
        {
            osc::SimulationReport r = sim.getSimulationReport(i);
            if (r.getTime() >= t)
            {
                return r;
            }
        }

        return sim.getSimulationReport(nReports-1);
    }

    std::optional<osc::SimulationReport> TrySelectReportBasedOnScrubbing(osc::VirtualSimulation& sim)
    {
        std::optional<osc::SimulationReport> maybeReport = TryLookupReportBasedOnScrubbing(sim);

        if (!maybeReport)
        {
            return maybeReport;
        }

        osc::SimulationReport& report = *maybeReport;

        // HACK: re-realize state, because of the OpenSim pathwrap bug: https://github.com/ComputationalBiomechanicsLab/opensim-creator/issues/123
        SimTK::State& st = report.updStateHACK();
        st.invalidateAllCacheAtOrAbove(SimTK::Stage::Instance);
        sim.getModel()->realizeReport(st);

        return maybeReport;
    }

    bool onKeyDown(SDL_KeyboardEvent const& e)
    {
        return false;
    }

	UID m_ID;
    std::string m_Name = ICON_FA_PLAY " Simulation_" + std::to_string(g_SimulationNumber++);
    MainUIStateAPI* m_API;
    std::shared_ptr<Simulation> m_Simulation;

    // the modelstate that's being shown in the UI, based on scrubbing etc.
    //
    // if possible (i.e. there's a simulation report available), will be set each frame
    std::unique_ptr<SimulationModelStatePair> m_ShownModelState;

    // UI widgets
    LogViewer m_LogViewerWidget;
    MainMenuFileTab m_MainMenuFileTab;
    MainMenuAboutTab m_MainMenuAboutTab;
    ComponentDetails m_ComponentDetailsWidget;
    PerfPanel m_PerfPanel{"Performance"};
    ModelHierarchyPanel m_ModelHierarchyPanel{"Hierarchy"};

    std::vector<UiModelViewer> m_ModelViewers = std::vector<UiModelViewer>(1);

    // scrubber/playback state
    bool m_IsPlayingBack = true;
    SimulationClock::time_point m_PlaybackStartSimtime = SimulationClock::start();
    std::chrono::system_clock::time_point m_PlaybackStartWallTime = std::chrono::system_clock::now();
};


// public API

osc::SimulatorTab::SimulatorTab(MainUIStateAPI* api, std::shared_ptr<Simulation> simulation) :
	m_Impl{new Impl{std::move(api), std::move(simulation)}}
{
}

osc::SimulatorTab::SimulatorTab(SimulatorTab&& tmp) noexcept :
	m_Impl{std::exchange(tmp.m_Impl, nullptr)}
{
}

osc::SimulatorTab& osc::SimulatorTab::operator=(SimulatorTab&& tmp) noexcept
{
	std::swap(m_Impl, tmp.m_Impl);
	return *this;
}

osc::SimulatorTab::~SimulatorTab() noexcept
{
	delete m_Impl;
}

osc::UID osc::SimulatorTab::implGetID() const
{
	return m_Impl->getID();
}

osc::CStringView osc::SimulatorTab::implGetName() const
{
	return m_Impl->getName();
}

osc::TabHost* osc::SimulatorTab::implParent() const
{
	return m_Impl->parent();
}

void osc::SimulatorTab::implOnMount()
{
	m_Impl->onMount();
}

void osc::SimulatorTab::implOnUnmount()
{
	m_Impl->onUnmount();
}

bool osc::SimulatorTab::implOnEvent(SDL_Event const& e)
{
	return m_Impl->onEvent(e);
}

void osc::SimulatorTab::implOnTick()
{
	m_Impl->onTick();
}

void osc::SimulatorTab::implOnDrawMainMenu()
{
	m_Impl->onDrawMainMenu();
}

void osc::SimulatorTab::implOnDraw()
{
	m_Impl->onDraw();
}
