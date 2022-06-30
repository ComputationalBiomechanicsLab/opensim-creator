#include "SimulatorTab.hpp"

#include "src/Bindings/ImGuiHelpers.hpp"
#include "src/MiddlewareAPIs/MainUIStateAPI.hpp"
#include "src/MiddlewareAPIs/SimulatorUIAPI.hpp"
#include "src/OpenSimBindings/ComponentOutputExtractor.hpp"
#include "src/OpenSimBindings/OutputExtractor.hpp"
#include "src/OpenSimBindings/Simulation.hpp"
#include "src/OpenSimBindings/SimulationClock.hpp"
#include "src/OpenSimBindings/SimulationModelStatePair.hpp"
#include "src/OpenSimBindings/SimulationReport.hpp"
#include "src/OpenSimBindings/VirtualOutputExtractor.hpp"
#include "src/OpenSimBindings/VirtualSimulation.hpp"
#include "src/Platform/App.hpp"
#include "src/Platform/Config.hpp"
#include "src/Platform/os.hpp"
#include "src/Tabs/TabHost.hpp"
#include "src/Utils/SynchronizedValue.hpp"
#include "src/Utils/Perf.hpp"
#include "src/Widgets/BasicWidgets.hpp"
#include "src/Widgets/LogViewer.hpp"
#include "src/Widgets/MainMenu.hpp"
#include "src/Widgets/ComponentDetails.hpp"
#include "src/Widgets/ModelHierarchyPanel.hpp"
#include "src/Widgets/PerfPanel.hpp"
#include "src/Widgets/SimulationOutputPlot.hpp"
#include "src/Widgets/UiModelViewer.hpp"

#include <glm/vec2.hpp>
#include <imgui.h>
#include <implot/implot.h>
#include <IconsFontAwesome5.h>
#include <nonstd/span.hpp>
#include <OpenSim/Common/Component.h>
#include <OpenSim/Common/ComponentOutput.h>
#include <OpenSim/Simulation/Model/Model.h>
#include <SDL_events.h>
#include <SimTKcommon/basics.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdio>
#include <fstream>
#include <filesystem>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

static std::atomic<int> g_SimulationNumber = 1;

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

class osc::SimulatorTab::Impl final : public SimulatorUIAPI {
public:
	Impl(MainUIStateAPI* api, std::shared_ptr<Simulation> simulation) :
        m_API{std::move(api)},
        m_Simulation{std::move(simulation)}
	{
	}

    // tab API

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

    // simulator UI API

    VirtualSimulation& updSimulation() override
    {
        return *m_Simulation;
    }

    SimulationClock::time_point getSimulationScrubTime() override
    {
        return getPlaybackPositionInSimTime(*m_Simulation);
    }

    void setSimulationScrubTime(SimulationClock::time_point t) override
    {
        m_PlaybackStartSimtime = t;
        m_IsPlayingBack = false;
    }

    std::optional<SimulationReport> trySelectReportBasedOnScrubbing() override
    {
        return TrySelectReportBasedOnScrubbing(*m_Simulation);
    }

    int getNumUserOutputExtractors() const override
    {
        return m_API->getNumUserOutputExtractors();
    }

    OutputExtractor const& getUserOutputExtractor(int i) const override
    {
        return m_API->getUserOutputExtractor(i);
    }

    void addUserOutputExtractor(OutputExtractor const& outputExtractor) override
    {
        m_API->addUserOutputExtractor(outputExtractor);
    }

    void removeUserOutputExtractor(int i) override
    {
        m_API->removeUserOutputExtractor(i);
    }

    bool hasUserOutputExtractor(OutputExtractor const& oe) const override
    {
        return m_API->hasUserOutputExtractor(oe);
    }

    bool removeUserOutputExtractor(OutputExtractor const& oe) override
    {
        return m_API->removeUserOutputExtractor(oe);
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

        if (bool outputsPanelOldState = config.getIsPanelEnabled("Output Watches"))
        {
            bool outputsPanelState = outputsPanelOldState;
            if (ImGui::Begin("Output Watches", &outputsPanelState))
            {
                OSC_PERF("draw output watches panel");
                drawOutputWatchesTab();
            }
            ImGui::End();

            if (outputsPanelState != outputsPanelOldState)
            {
                App::upd().updConfig().setIsPanelEnabled("Output Watches", outputsPanelState);
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

    void drawOutputWatchesTab()
    {
        if (m_API->getNumUserOutputExtractors() <= 0)
        {
            ImGui::TextDisabled("(no outputs requested)");
            return;
        }

        ImGui::Button(ICON_FA_SAVE " Save All " ICON_FA_CARET_DOWN);
        if (ImGui::BeginPopupContextItem("##exportoptions", ImGuiPopupFlags_MouseButtonLeft))
        {
            if (ImGui::MenuItem("as CSV"))
            {
                osc::TryPromptAndSaveAllUserDesiredOutputsAsCSV(*this);
            }

            if (ImGui::MenuItem("as CSV (and open)"))
            {
                auto p = osc::TryPromptAndSaveAllUserDesiredOutputsAsCSV(*this);
                if (!p.empty())
                {
                    osc::OpenPathInOSDefaultApplication(p);
                }
            }

            ImGui::EndPopup();
        }

        ImGui::Separator();
        ImGui::Dummy({0.0f, 5.0f});

        for (int i = 0; i < m_API->getNumUserOutputExtractors(); ++i)
        {
            osc::OutputExtractor output = m_API->getUserOutputExtractor(i);

            ImGui::PushID(i);
            SimulationOutputPlot plot{this, output, 64.0f};
            plot.draw();
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
                SimulationOutputPlot plot{this, osc::OutputExtractor{osc::ComponentOutputExtractor{*aoPtr}}, ImGui::GetTextLineHeight()};
                plot.draw();
                ImGui::NextColumn();

                ImGui::PopID();
            }
            ImGui::Columns();
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
                    osc::TryPromptAndSaveOutputsAsCSV(*this, outputs);
                }

                if (ImGui::MenuItem("as CSV (and open)"))
                {
                    std::filesystem::path p = osc::TryPromptAndSaveOutputsAsCSV(*this, outputs);
                    if (!p.empty())
                    {
                        osc::OpenPathInOSDefaultApplication(p);
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
            SimulationOutputPlot plot{this, output, 32.0f};
            plot.draw();
            ImGui::NextColumn();
            ImGui::PopID();
        }
        ImGui::Columns();
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
            "Output Watches",
            "Selection Details",
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
            float leftPadding = 100.0f;
            float bottomPadding = 20.0f;
            float panelHeight = 50.0f;
            ImGui::SetNextWindowPos({ pos.x + leftPadding, pos.y + dims.y - panelHeight - bottomPadding });
            ImGui::SetNextWindowSize({ dims.x - 1.1f*leftPadding, panelHeight });
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
                DrawWatchOutputMenu(*m_API, *selected);
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

    bool onKeyDown(SDL_KeyboardEvent const&)
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
