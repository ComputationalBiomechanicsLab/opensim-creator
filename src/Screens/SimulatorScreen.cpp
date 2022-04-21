#include "SimulatorScreen.hpp"

#include "src/Bindings/ImGuiHelpers.hpp"
#include "src/Maths/BVH.hpp"
#include "src/OpenSimBindings/ComponentOutputExtractor.hpp"
#include "src/OpenSimBindings/ParamBlock.hpp"
#include "src/OpenSimBindings/OpenSimHelpers.hpp"
#include "src/OpenSimBindings/ComponentDecoration.hpp"
#include "src/OpenSimBindings/OpenSimHelpers.hpp"
#include "src/OpenSimBindings/MainEditorState.hpp"
#include "src/OpenSimBindings/RenderableScene.hpp"
#include "src/OpenSimBindings/Simulation.hpp"
#include "src/OpenSimBindings/SimulationClock.hpp"
#include "src/OpenSimBindings/SimulationReport.hpp"
#include "src/OpenSimBindings/VirtualOutputExtractor.hpp"
#include "src/OpenSimBindings/SimulatorModelStatePair.hpp"
#include "src/OpenSimBindings/VirtualSimulation.hpp"
#include "src/Platform/App.hpp"
#include "src/Platform/os.hpp"
#include "src/Platform/Styling.hpp"
#include "src/Screens/ModelEditorScreen.hpp"
#include "src/Utils/ScopeGuard.hpp"
#include "src/Utils/Perf.hpp"
#include "src/Widgets/LogViewer.hpp"
#include "src/Widgets/MainMenu.hpp"
#include "src/Widgets/ComponentDetails.hpp"
#include "src/Widgets/ComponentHierarchy.hpp"
#include "src/Widgets/PerfPanel.hpp"
#include "src/Widgets/UiModelViewer.hpp"

#include <OpenSim/Simulation/Model/Model.h>
#include <OpenSim/Common/ComponentOutput.h>
#include <imgui.h>
#include <implot/implot.h>
#include <IconsFontAwesome5.h>
#include <nonstd/span.hpp>

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

// simulator screeen (private) state
struct osc::SimulatorScreen::Impl final {

    // top-level state, shared between screens
    std::shared_ptr<MainEditorState> mes;

    // the modelstate that's being shown in the UI, based on scrubbing etc.
    //
    // if possible (i.e. there's a simulation report available), will be set each frame
    std::unique_ptr<SimulatorModelStatePair> m_ShownModelState;

    // UI widgets
    LogViewer logViewerWidget;
    MainMenuFileTab mainMenuFileTab;
    MainMenuWindowTab mainMenuWindowTab;
    MainMenuAboutTab mainMenuAboutTab;
    ComponentDetails componentDetailsWidget;
    PerfPanel perfPanel{"Perf"};

    // scrubber/playback state
    bool m_IsPlayingBack = true;
    SimulationClock::time_point m_PlaybackStartSimtime = SimulationClock::start();
    std::chrono::system_clock::time_point m_PlaybackStartWallTime = std::chrono::system_clock::now();

    Impl(std::shared_ptr<MainEditorState> _mes) : mes{std::move(_mes)}
    {
        // lazily init at least one viewer
        if (mes->getNumViewers() == 0)
        {
            mes->addViewer();
        }
    }
};

// returns the playback position, which changes based on the wall clock (it's a playback)
// onto the time within a simulation
static osc::SimulationClock::time_point GetPlaybackPositionInSimTime(osc::SimulatorScreen::Impl const& impl,
                                                                     osc::VirtualSimulation const& sim)
{
    if (!impl.m_IsPlayingBack)
    {
        return impl.m_PlaybackStartSimtime;
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
            auto wallDur = wallNow - impl.m_PlaybackStartWallTime;

            osc::SimulationClock::time_point simNow = impl.m_PlaybackStartSimtime + osc::SimulationClock::duration{wallDur};
            osc::SimulationClock::time_point simLatest = sim.getSimulationReport(nReports-1).getTime();

            return simNow <= simLatest ? simNow : simLatest;
        }
    }
}


static std::optional<osc::SimulationReport> TryLookupReportBasedOnScrubbing(
        osc::SimulatorScreen::Impl& impl,
        osc::VirtualSimulation& sim)
{
    int nReports = sim.getNumReports();

    if (nReports <= 0)
    {
        return std::nullopt;
    }

    osc::SimulationClock::time_point t = GetPlaybackPositionInSimTime(impl, sim);

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

static std::optional<osc::SimulationReport> TrySelectReportBasedOnScrubbing(
        osc::SimulatorScreen::Impl& impl,
        osc::VirtualSimulation& sim)
{
    std::optional<osc::SimulationReport> maybeReport = TryLookupReportBasedOnScrubbing(impl, sim);

    if (!maybeReport)
    {
        return maybeReport;
    }

    osc::SimulationReport& report = *maybeReport;

    // re-realize state, because of the OpenSim pathwrap bug: https://github.com/ComputationalBiomechanicsLab/opensim-creator/issues/123
    SimTK::State& st = report.updStateHACK();
    st.invalidateAllCacheAtOrAbove(SimTK::Stage::Instance);
    auto guard = sim.getModel();
    guard->realizeReport(st);

    return maybeReport;
}

// draw timescrubber slider
static void DrawSimulationScrubber(osc::SimulatorScreen::Impl& impl, osc::Simulation& sim)
{
    // play/pause buttons
    if (!impl.m_IsPlayingBack)
    {
        if (ImGui::Button(ICON_FA_PLAY))
        {
            impl.m_PlaybackStartWallTime = std::chrono::system_clock::now();
            impl.m_IsPlayingBack = true;
        }
    }
    else
    {
        if (ImGui::Button(ICON_FA_PAUSE))
        {
            impl.m_PlaybackStartSimtime = GetPlaybackPositionInSimTime(impl, sim);
            impl.m_IsPlayingBack = false;
        }
    }

    osc::SimulationClock::time_point tStart = sim.getStartTime();
    osc::SimulationClock::time_point tEnd = sim.getEndTime();
    osc::SimulationClock::time_point tCur = GetPlaybackPositionInSimTime(impl, sim);

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
        impl.m_PlaybackStartSimtime = osc::SimulationClock::start() + osc::SimulationClock::duration{static_cast<double>(v)};
        impl.m_PlaybackStartWallTime = std::chrono::system_clock::now();
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

namespace
{
    class RenderableSim final : public osc::RenderableScene {
    public:
        RenderableSim(OpenSim::Model const& model,
                      osc::SimulationReport const& report,
                      float fixupScaleFactor,
                      OpenSim::Component const* selected,
                      OpenSim::Component const* hovered,
                      OpenSim::Component const* isolated) :
            m_Decorations{},
            m_SceneBVH{},
            m_FixupScaleFactor{std::move(fixupScaleFactor)},
            m_Selected{std::move(selected)},
            m_Hovered{std::move(hovered)},
            m_Isolated{std::move(isolated)}
        {
            GenerateModelDecorations(model,
                                     report.getState(),
                                     m_FixupScaleFactor,
                                     m_Decorations,
                                     m_Selected,
                                     m_Hovered);
            UpdateSceneBVH(m_Decorations, m_SceneBVH);
        }

        nonstd::span<osc::ComponentDecoration const> getSceneDecorations() const override
        {
            return m_Decorations;
        }

        osc::BVH const& getSceneBVH() const override
        {
            return m_SceneBVH;
        }

        float getFixupScaleFactor() const override
        {
            return m_FixupScaleFactor;
        }

        OpenSim::Component const* getSelected() const override
        {
            return m_Selected;
        }

        OpenSim::Component const* getHovered() const override
        {
            return m_Hovered;
        }

        OpenSim::Component const* getIsolated() const override
        {
            return m_Isolated;
        }
    private:
        std::vector<osc::ComponentDecoration> m_Decorations;
        osc::BVH m_SceneBVH;
        float m_FixupScaleFactor;
        OpenSim::Component const* m_Selected;
        OpenSim::Component const* m_Hovered;
        OpenSim::Component const* m_Isolated;
    };
}

// draw a 3D model viewer
static bool SimscreenDraw3DViewer(osc::SimulatorScreen::Impl& impl,
                                  osc::SimulatorModelStatePair& ms,
                                  osc::UiModelViewer& viewer,
                                  char const* name)
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

    RenderableSim rs
    {
        ms.getModel(),
        ms.getSimulationReport(),
        impl.mes->editedModel()->getFixupScaleFactor(),
        ms.getSelected(),
        ms.getHovered(),
        ms.getIsolated(),
    };
    auto resp = viewer.draw(rs);
    ImGui::End();

    if (resp.hovertestResult)
    {
        if (resp.isLeftClicked && resp.hovertestResult != ms.getSelected())
        {
            ms.setSelected(resp.hovertestResult);
            osc::App::cur().requestRedraw();
        }

        if (resp.isMousedOver && resp.hovertestResult != ms.getHovered())
        {
            ms.setHovered(resp.hovertestResult);
            osc::App::cur().requestRedraw();
        }
    }
    else
    {
        if (resp.isLeftClicked)
        {
            ms.setSelected(nullptr);
        }

        ms.setHovered(nullptr);
    }

    return true;
}

// draw all active 3D viewers
//
// the user can (de)activate 3D viewers in the "Window" tab
static void SimscreenDrawAll3DViewers(osc::SimulatorScreen::Impl& impl)
{
    if (!impl.m_ShownModelState)
    {
        if (ImGui::Begin("render"))
        {
            ImGui::TextDisabled("(no simulation data available)");
        }
        ImGui::End();
        return;
    }

    osc::SimulatorModelStatePair& ms = *impl.m_ShownModelState;

    osc::MainEditorState& st = *impl.mes;
    for (int i = 0; i < st.getNumViewers(); ++i)
    {
        osc::UiModelViewer& viewer = st.updViewer(i);

        char buf[64];
        std::snprintf(buf, sizeof(buf), "viewer%i", i);

        bool isOpen = SimscreenDraw3DViewer(impl, ms, viewer, buf);
        if (!isOpen)
        {
            st.removeViewer(i);
            --i;
        }
    }
}

static bool SimscreenDrawMainMenu(osc::SimulatorScreen::Impl& impl)
{
    // draw main menu
    if (ImGui::BeginMainMenuBar())
    {
        impl.mainMenuFileTab.draw(impl.mes);
        impl.mainMenuWindowTab.draw(*impl.mes);
        impl.mainMenuAboutTab.draw();

        ImGui::Dummy({5.0f, 0.0f});

        if (ImGui::Button(ICON_FA_CUBE " Switch to editor (Ctrl+E)"))
        {
            // request the transition then exit this drawcall ASAP
            osc::App::cur().requestTransition<osc::ModelEditorScreen>(std::move(impl.mes));
            ImGui::EndMainMenuBar();
            return true;
        }

        ImGui::EndMainMenuBar();
    }

    return false;
}

static void SimscreenDrawHierarchyTab(osc::SimulatorScreen::Impl& impl)
{
    if (!impl.m_ShownModelState)
    {
        ImGui::TextDisabled("(no simulation selected)");
        return;
    }

    osc::SimulatorModelStatePair& ms = *impl.m_ShownModelState;

    auto resp = osc::ComponentHierarchy{}.draw(&ms.getModel(),
                                               ms.getSelected(),
                                               ms.getHovered());

    if (resp.type == osc::ComponentHierarchy::SelectionChanged)
    {
        ms.setSelected(resp.ptr);
    }
    else if (resp.type == osc::ComponentHierarchy::HoverChanged)
    {
        ms.setHovered(resp.ptr);
    }
}

static void DrawSimulationParamValue(osc::ParamValue const& v)
{
    if (std::holds_alternative<double>(v))
    {
        ImGui::Text("%f", static_cast<float>(std::get<double>(v)));
    }
    else if (std::holds_alternative<osc::IntegratorMethod>(v))
    {
        ImGui::Text("%s", osc::GetIntegratorMethodString(std::get<osc::IntegratorMethod>(v)));
    }
    else if (std::holds_alternative<int>(v))
    {
        ImGui::Text("%i", std::get<int>(v));
    }
    else
    {
        ImGui::Text("(unknown value type)");
    }
}

static void DrawSimulationParams(osc::ParamBlock const& params)
{
    ImGui::Dummy({0.0f, 1.0f});
    ImGui::TextUnformatted("parameters:");
    ImGui::SameLine();
    osc::DrawHelpMarker("The parameters used when this simulation was launched. These must be set *before* running the simulation");
    ImGui::Separator();
    ImGui::Dummy({0.0f, 2.0f});

    ImGui::Columns(2);
    for (int i = 0, len = params.size(); i < len; ++i)
    {
        std::string const& name = params.getName(i);
        std::string const& description = params.getDescription(i);
        osc::ParamValue const& value = params.getValue(i);

        ImGui::TextUnformatted(name.c_str());
        ImGui::SameLine();
        osc::DrawHelpMarker(name.c_str(), description.c_str());
        ImGui::NextColumn();

        DrawSimulationParamValue(value);
        ImGui::NextColumn();
    }
    ImGui::Columns();
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

static std::vector<float> PopulateFirstNNumericOutputValues(OpenSim::Model const& model,
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
    OpenSim::Model const& m = *guard;
    for (size_t i = 0; i < reports.size(); ++i)
    {
        fout << times.at(i);  // time column

        osc::SimulationReport r = reports[i];
        for (osc::OutputExtractor const& o : outputs)
        {
            fout << ',' << o.getValueFloat(m, r);
        }

        fout << '\n';
    }

    if (!fout)
    {
        osc::log::warn("%s: encountered error while writing output data: some of the data may have been written, but maybe not all of it", p.string().c_str());
    }

    return p.string();
}

static void DrawGenericNumericOutputContextMenuItems(osc::Simulation& sim,
                                                     osc::VirtualOutputExtractor const& output)
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

static void DrawNumericOutputPlot(osc::SimulatorScreen::Impl& impl,
                                  osc::Simulation& sim,
                                  osc::VirtualOutputExtractor const& output,
                                  float plotHeight)
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
        auto guard = sim.getModel();
        OpenSim::Model const& model = *guard;
        OSC_PERF("collect output data");
        std::vector<osc::SimulationReport> reports = sim.getAllSimulationReports();
        buf.resize(reports.size());
        output.getValuesFloat(model, reports, buf);
    }

    // draw plot
    float const plotWidth = ImGui::GetContentRegionAvailWidth();
    glm::vec2 plotTopLeft{};
    glm::vec2 plotBottomRight{};

    {
        OSC_PERF("draw output plot");
;
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
    osc::SimulationClock::time_point simScrubTime = GetPlaybackPositionInSimTime(impl, sim);

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
            impl.m_PlaybackStartSimtime = timeLoc;
            impl.m_IsPlayingBack = false;
        }
    }
}

static void TextCentered(std::string const& s)
{
    auto windowWidth = ImGui::GetWindowSize().x;
    auto textWidth   = ImGui::CalcTextSize(s.c_str()).x;

    ImGui::SetCursorPosX((windowWidth - textWidth) * 0.5f);
    ImGui::TextUnformatted(s.c_str());
}

static void DrawOutputNameColumn(osc::VirtualOutputExtractor const& output, bool centered = true)
{
    if (centered)
    {
        TextCentered(output.getName());
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

static void DrawOutputDataColumn(osc::SimulatorScreen::Impl& impl,
                                 osc::Simulation& sim,
                                 osc::VirtualOutputExtractor const& output,
                                 float plotHeight)
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
        DrawNumericOutputPlot(impl, sim, output, plotHeight);
    }
    else if (outputType == osc::OutputType::String)
    {
        osc::SimulationReport r = TrySelectReportBasedOnScrubbing(impl, sim).value_or(sim.getSimulationReport(nReports-1));
        auto guard = sim.getModel();
        ImGui::TextUnformatted(output.getValueString(*guard, r).c_str());
    }
}

static void DrawSimulationStatPlots(osc::SimulatorScreen::Impl& impl,
                                    osc::Simulation& sim)
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
        DrawOutputDataColumn(impl, sim, output, 32.0f);
        ImGui::NextColumn();
        ImGui::PopID();
    }
    ImGui::Columns();
}

static void SimscreenDrawSimulationStats(osc::SimulatorScreen::Impl& impl)
{
    osc::MainEditorState& st = *impl.mes;
    std::shared_ptr<osc::Simulation> maybeSim = st.updFocusedSimulation();

    if (!maybeSim)
    {
        ImGui::TextDisabled("(no simulation selected)");
        return;
    }

    osc::Simulation& sim = *maybeSim;

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
        ImGui::Text("%i", sim.getNumReports());
        ImGui::NextColumn();
        ImGui::Columns();
    }

    {
        OSC_PERF("draw simulation params");
        DrawSimulationParams(sim.getParams());
    }

    ImGui::Dummy({0.0f, 10.0f});

    {
        OSC_PERF("draw simulation stats");
        DrawSimulationStatPlots(impl, sim);
    }
}

static void DrawSimulationProgressBarEtc(osc::SimulatorScreen::Impl& impl, int simulationIdx)
{
    osc::MainEditorState& st = *impl.mes;
    std::shared_ptr<osc::Simulation> sim = st.updSimulation(simulationIdx);

    bool isFocused = simulationIdx == st.getFocusedSimulationIndex();
    float progress = sim->getProgress();
    ImVec4 baseColor = progress >= 1.0f ? ImVec4{0.0f, 0.7f, 0.0f, 0.5f} : ImVec4{0.7f, 0.7f, 0.0f, 0.5f};

    if (isFocused)
    {
        baseColor.w = 1.0f;
    }

    bool shouldErase = false;
    if (ImGui::Button(ICON_FA_TRASH))
    {
        shouldErase = true;
    }

    ImGui::SameLine();
    ImGui::PushStyleColor(ImGuiCol_PlotHistogram, baseColor);
    ImGui::ProgressBar(progress);
    ImGui::PopStyleColor();

    if (ImGui::IsItemHovered())
    {
        if (ImGui::IsKeyPressed(SDL_SCANCODE_DELETE))
        {
            shouldErase = true;
        }

        ImGui::BeginTooltip();
        ImGui::PushTextWrapPos(ImGui::GetFontSize() + 400.0f);
        ImGui::TextUnformatted(sim->getModel()->getName().c_str());
        ImGui::Dummy({0.0f, 1.0f});
        ImGui::PushStyleColor(ImGuiCol_Text, OSC_SLIGHTLY_GREYED_RGBA);
        ImGui::Text("Sim time (sec): %.1f", static_cast<float>((sim->getCurTime() - sim->getStartTime()).count()));
        ImGui::Text("Sim final time (sec): %.1f", static_cast<float>(sim->getEndTime().time_since_epoch().count()));
        ImGui::Dummy({0.0f, 1.0f});
        ImGui::TextUnformatted("Left-click: Select this simulation");
        ImGui::TextUnformatted("Delete: cancel this simulation");
        ImGui::PopStyleColor();
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }

    if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
    {
        st.setFocusedSimulation(simulationIdx);
    }

    if (ImGui::BeginPopupContextItem("simcontextmenu"))
    {
        st.setFocusedSimulation(simulationIdx);

        if (ImGui::MenuItem("edit model"))
        {
            std::shared_ptr<osc::UndoableUiModel> editedModel = st.editedModel();
            editedModel->setModel(std::make_unique<OpenSim::Model>(*sim->getModel()));
            editedModel->commit("loaded model from simulator window");
            osc::App::cur().requestTransition<osc::ModelEditorScreen>(impl.mes);
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

    if (shouldErase)
    {
        st.removeSimulation(simulationIdx);
    }
}

static void SimscreenDrawSimulatorTab(osc::SimulatorScreen::Impl& impl)
{
    osc::MainEditorState& st = *impl.mes;

    if (st.getNumSimulations() <= 0)
    {
        ImGui::TextDisabled("(no simulations available)");
        return;
    }

    std::shared_ptr<osc::Simulation> maybeSim = st.updFocusedSimulation();

    if (maybeSim)
    {
        DrawSimulationScrubber(impl, *maybeSim);
    }
    else
    {
        ImGui::TextDisabled("(no simulation selected)");
    }

    // draw simulations list
    ImGui::Dummy({0.0f, 1.0f});
    ImGui::TextUnformatted("Simulations:");
    ImGui::Separator();
    ImGui::Dummy({0.0f, 0.3f});


    for (int i = 0; i < st.getNumSimulations(); ++i)
    {
        ImGui::PushID(i);
        DrawSimulationProgressBarEtc(impl, i);
        ImGui::PopID();
    }
}

static void SimscreenDrawSelectionTab(osc::SimulatorScreen::Impl& impl)
{
    if (!impl.m_ShownModelState)
    {
        ImGui::TextDisabled("(no simulation selected)");
        return;
    }

    osc::SimulatorModelStatePair& ms = *impl.m_ShownModelState;

    OpenSim::Component const* selected = ms.getSelected();

    if (!selected)
    {
        ImGui::TextDisabled("(nothing selected)");
        return;
    }

    impl.componentDetailsWidget.draw(ms.getState(), selected);

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
            DrawOutputDataColumn(impl, *ms.updSimulation(), output, ImGui::GetTextLineHeight());
            ImGui::NextColumn();

            ImGui::PopID();
        }
        ImGui::Columns();
    }
}

static void SimscreenDrawOutputsTab(osc::SimulatorScreen::Impl& impl)
{
    osc::MainEditorState& st = *impl.mes;
    std::shared_ptr<osc::Simulation> maybeSim = st.updFocusedSimulation();

    if (!maybeSim)
    {
        ImGui::TextDisabled("(no simulation selected)");
        return;
    }

    osc::Simulation& sim = *maybeSim;

    int numOutputs = st.getNumUserOutputExtractors();

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
            TryExportOutputsToCSV(sim, osc::GetAllUserDesiredOutputs(st));
        }

        if (ImGui::MenuItem("as CSV (and open)"))
        {
            std::string path = TryExportOutputsToCSV(sim, osc::GetAllUserDesiredOutputs(st));
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
        osc::OutputExtractor const& output = st.getUserOutputExtractor(i);

        ImGui::PushID(i);
        DrawOutputDataColumn(impl, sim, output, 64.0f);
        DrawOutputNameColumn(output, true);
        ImGui::PopID();
    }
}

// draw the simulator screen
static void SimscreenDraw(osc::SimulatorScreen::Impl& impl)
{
    OSC_PERF("draw simulation screen");

    if (SimscreenDrawMainMenu(impl))
    {
        return;
    }

    osc::MainEditorState& st = *impl.mes;

    // edge-case: there are no simulations available, so
    // show a "you need to run something, fool" dialog
    if (st.getNumSimulations() <= 0)
    {
        if (ImGui::Begin("Warning"))
        {
            ImGui::TextUnformatted("No simulations are currently running");
            if (ImGui::Button("Run new simulation"))
            {
                StartSimulatingEditedModel(st);
                impl.m_IsPlayingBack = true;
                impl.m_PlaybackStartSimtime = osc::SimulationClock::start();
                impl.m_PlaybackStartWallTime = std::chrono::system_clock::now();
            }
        }
        ImGui::End();
        return;
    }

    // ensure m_ShownModelState is populated, if possible
    {
        std::shared_ptr<osc::Simulation> maybeSim = st.updFocusedSimulation();
        if (maybeSim)
        {
            std::optional<osc::SimulationReport> maybeReport = TrySelectReportBasedOnScrubbing(impl, *maybeSim);
            if (maybeReport)
            {
                float sf = impl.mes->editedModel()->getFixupScaleFactor();
                if (impl.m_ShownModelState)
                {
                    impl.m_ShownModelState->setSimulation(maybeSim);
                    impl.m_ShownModelState->setSimulationReport(*maybeReport);
                    impl.m_ShownModelState->setFixupScaleFactor(sf);
                }
                else
                {
                    impl.m_ShownModelState = std::make_unique<osc::SimulatorModelStatePair>(
                        maybeSim,
                        *maybeReport,
                        sf);
                }
            }
        }
    }

    // draw simulations tab
    if (st.getUserPanelPrefs().simulations)
    {
        if (ImGui::Begin("Simulations", &st.updUserPanelPrefs().simulations))
        {
            OSC_PERF("draw simulations panel");
            SimscreenDrawSimulatorTab(impl);
        }
        ImGui::End();
    }

    // draw 3D viewers
    {
        OSC_PERF("draw simulator panels");
        SimscreenDrawAll3DViewers(impl);
    }


    // draw hierarchy tab
    if (st.getUserPanelPrefs().hierarchy)
    {
        if (ImGui::Begin("Hierarchy", &st.updUserPanelPrefs().hierarchy))
        {
            OSC_PERF("draw hierarchy panel");
            SimscreenDrawHierarchyTab(impl);
        }
        ImGui::End();
    }

    // draw selection tab
    if (st.getUserPanelPrefs().selectionDetails)
    {
        if (ImGui::Begin("Selection", &st.updUserPanelPrefs().selectionDetails))
        {
            OSC_PERF("draw selection panel");
            SimscreenDrawSelectionTab(impl);
        }
        ImGui::End();
    }

    // outputs
    if (st.getUserPanelPrefs().outputs)
    {
        if (ImGui::Begin("Outputs", &st.updUserPanelPrefs().outputs))
        {
            OSC_PERF("draw outputs panel");
            SimscreenDrawOutputsTab(impl);
        }
        ImGui::End();
    }

    // simulation stats
    if (st.getUserPanelPrefs().simulationStats)
    {
        if (ImGui::Begin("Simulation Details", &st.updUserPanelPrefs().simulationStats))
        {
            OSC_PERF("draw simulation details panel");
            SimscreenDrawSimulationStats(impl);
        }
        ImGui::End();
    }

    if (st.getUserPanelPrefs().log)
    {
        if (ImGui::Begin("Log", &st.updUserPanelPrefs().log, ImGuiWindowFlags_MenuBar))
        {
            OSC_PERF("draw log panel");
            impl.logViewerWidget.draw();
        }
        ImGui::End();
    }

    if (st.getUserPanelPrefs().perfPanel)
    {
        OSC_PERF("draw perf panel");
        impl.perfPanel.open();
        st.updUserPanelPrefs().perfPanel = impl.perfPanel.draw();
    }
}

// action to take when user presses a key
static bool SimscreenOnKeydown(osc::SimulatorScreen::Impl& impl,
                               SDL_KeyboardEvent const& e)
{
    if (e.keysym.mod & KMOD_CTRL)
    {
        // Ctrl
        switch (e.keysym.sym) {
        case SDLK_e:
            // Ctrl + e
            osc::App::cur().requestTransition<osc::ModelEditorScreen>(std::move(impl.mes));
            return true;
        }
    }
    return false;
}

// action to take when a generic event occurs
static bool SimscreenOnEvent(osc::SimulatorScreen::Impl& impl,
                             SDL_Event const& e)
{
    if (e.type == SDL_KEYDOWN)
    {
        if (SimscreenOnKeydown(impl, e.key))
        {
            return true;
        }
    }
    return false;
}

// Simulator_screen: public impl.

osc::SimulatorScreen::SimulatorScreen(std::shared_ptr<MainEditorState> mes) :
    m_Impl{new Impl{std::move(mes)}}
{
}

osc::SimulatorScreen::~SimulatorScreen() noexcept = default;

void osc::SimulatorScreen::onMount()
{
    osc::ImGuiInit();
    ImPlot::CreateContext();
    App::cur().makeMainEventLoopWaiting();
}

void osc::SimulatorScreen::onUnmount()
{
    osc::ImGuiShutdown();
    ImPlot::DestroyContext();
    App::cur().makeMainEventLoopPolling();
}

void osc::SimulatorScreen::onEvent(SDL_Event const& e)
{
    if (e.type == SDL_QUIT)
    {
        App::cur().requestQuit();
        return;
    }
    else if (osc::ImGuiOnEvent(e))
    {
        return;
    }
    else
    {
        SimscreenOnEvent(*m_Impl, e);
    }
}

void osc::SimulatorScreen::tick(float)
{
    if (m_Impl->m_IsPlayingBack)
    {
        osc::MainEditorState& st = *m_Impl->mes;
        std::shared_ptr<osc::Simulation> maybeSim = st.updFocusedSimulation();

        if (maybeSim)
        {
            osc::Simulation& sim = *maybeSim;
            auto playbackPos = GetPlaybackPositionInSimTime(*m_Impl, sim);
            if (playbackPos < sim.getEndTime())
            {
                osc::App::cur().requestRedraw();
            }
            else
            {
                m_Impl->m_IsPlayingBack = false;
                return;
            }
        }
    }
}

void osc::SimulatorScreen::draw()
{
    App::cur().clearScreen({0.0f, 0.0f, 0.0f, 0.0f});
    osc::ImGuiNewFrame();
    ImGui::DockSpaceOverViewport(ImGui::GetMainViewport(), ImGuiDockNodeFlags_PassthruCentralNode);
    SimscreenDraw(*m_Impl);
    osc::ImGuiRender();
}
