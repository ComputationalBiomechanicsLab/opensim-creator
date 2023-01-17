#include "SimulatorTab.hpp"

#include "src/Bindings/ImGuiHelpers.hpp"
#include "src/OpenSimBindings/MiddlewareAPIs/MainUIStateAPI.hpp"
#include "src/OpenSimBindings/MiddlewareAPIs/SimulatorUIAPI.hpp"
#include "src/OpenSimBindings/Widgets/BasicWidgets.hpp"
#include "src/OpenSimBindings/Widgets/MainMenu.hpp"
#include "src/OpenSimBindings/Widgets/NavigatorPanel.hpp"
#include "src/OpenSimBindings/Widgets/ComponentDetails.hpp"
#include "src/OpenSimBindings/Widgets/SimulationDetailsPanel.hpp"
#include "src/OpenSimBindings/Widgets/SimulationOutputPlot.hpp"
#include "src/OpenSimBindings/Widgets/SimulationScrubber.hpp"
#include "src/OpenSimBindings/Widgets/SimulationToolbar.hpp"
#include "src/OpenSimBindings/Widgets/UiModelViewer.hpp"
#include "src/OpenSimBindings/ComponentOutputExtractor.hpp"
#include "src/OpenSimBindings/OutputExtractor.hpp"
#include "src/OpenSimBindings/OpenSimHelpers.hpp"
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
#include "src/Widgets/LogViewerPanel.hpp"
#include "src/Widgets/PerfPanel.hpp"

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

namespace
{
    bool IsAnyOutputExportableToCSV(osc::MainUIStateAPI& api)
    {
        for (int i = 0; i < api.getNumUserOutputExtractors(); ++i)
        {
            if (api.getUserOutputExtractor(i).getOutputType() == osc::OutputType::Float)
            {
                return true;
            }
        }
        return false;
    }
}

class osc::SimulatorTab::Impl final : public SimulatorUIAPI {
public:

    Impl(
        MainUIStateAPI* api,
        std::shared_ptr<Simulation> simulation) :

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
        return false;
    }

    void onTick()
    {
        if (m_IsPlayingBack)
        {
            SimulationClock::time_point playbackPos = implGetSimulationScrubTime();
            if (playbackPos < m_Simulation->getEndTime())
            {
                osc::App::upd().requestRedraw();
            }
            else
            {
                m_PlaybackStartSimtime = playbackPos;
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
        ImGui::DockSpaceOverViewport(
            ImGui::GetMainViewport(),
            ImGuiDockNodeFlags_PassthruCentralNode
        );
        drawContent();
    }

private:
    std::optional<SimulationReport> tryFindNthReportAfter(SimulationClock::time_point t, int offset = 0)
    {
        int const numSimulationReports = m_Simulation->getNumReports();

        if (numSimulationReports <= 0)
        {
            return std::nullopt;
        }

        int zeroethReportIndex = numSimulationReports-1;
        for (int i = 0; i < numSimulationReports; ++i)
        {
            SimulationReport r = m_Simulation->getSimulationReport(i);
            if (r.getTime() >= t)
            {
                zeroethReportIndex = i;
                break;
            }
        }

        int const reportIndex = zeroethReportIndex + offset;
        if (0 <= reportIndex && reportIndex < numSimulationReports)
        {
            return m_Simulation->getSimulationReport(reportIndex);
        }
        else
        {
            return std::nullopt;
        }
    }

    VirtualSimulation& implUpdSimulation() final
    {
        return *m_Simulation;
    }

    bool implGetSimulationPlaybackState() final
    {
        return m_IsPlayingBack;
    }

    void implSetSimulationPlaybackState(bool v) final
    {
        if (v)
        {
            m_PlaybackStartWallTime = std::chrono::system_clock::now();
            m_IsPlayingBack = true;
        }
        else
        {
            m_PlaybackStartSimtime = getSimulationScrubTime();
            m_IsPlayingBack = false;
        }
    }

    float implGetSimulationPlaybackSpeed() final
    {
        return m_PlaybackSpeed;
    }

    void implSetSimulationPlaybackSpeed(float v)
    {
        m_PlaybackSpeed = v;
    }

    SimulationClock::time_point implGetSimulationScrubTime() final
    {
        if (!m_IsPlayingBack)
        {
            return m_PlaybackStartSimtime;
        }
        else
        {
            // map wall time onto sim time

            int nReports = m_Simulation->getNumReports();
            if (nReports <= 0)
            {
                return m_Simulation->getStartTime();
            }
            else
            {
                std::chrono::system_clock::time_point wallNow = std::chrono::system_clock::now();
                std::chrono::system_clock::duration wallDur = wallNow - m_PlaybackStartWallTime;

                SimulationClock::duration simDur = m_PlaybackSpeed * osc::SimulationClock::duration{wallDur};
                osc::SimulationClock::time_point simNow = m_PlaybackStartSimtime + simDur;
                osc::SimulationClock::time_point simLatest = m_Simulation->getSimulationReport(nReports-1).getTime();

                return simNow <= simLatest ? simNow : simLatest;
            }
        }
    }

    void implSetSimulationScrubTime(SimulationClock::time_point t) final
    {
        m_PlaybackStartSimtime = t;
        m_PlaybackStartWallTime = std::chrono::system_clock::now();
    }

    void implStepBack() final
    {
        std::optional<SimulationReport> const maybePrev = tryFindNthReportAfter(getSimulationScrubTime(), -1);
        if (maybePrev)
        {
            setSimulationScrubTime(maybePrev->getTime());
        }
    }

    void implStepForward() final
    {
        std::optional<SimulationReport> const maybeNext = tryFindNthReportAfter(getSimulationScrubTime(), 1);
        if (maybeNext)
        {
            setSimulationScrubTime(maybeNext->getTime());
        }
    }

    std::optional<SimulationReport> implTrySelectReportBasedOnScrubbing() final
    {
        return tryFindNthReportAfter(getSimulationScrubTime());
    }

    int implGetNumUserOutputExtractors() const final
    {
        return m_API->getNumUserOutputExtractors();
    }

    OutputExtractor const& implGetUserOutputExtractor(int i) const final
    {
        return m_API->getUserOutputExtractor(i);
    }

    void implAddUserOutputExtractor(OutputExtractor const& outputExtractor) final
    {
        m_API->addUserOutputExtractor(outputExtractor);
    }

    void implRemoveUserOutputExtractor(int i) final
    {
        m_API->removeUserOutputExtractor(i);
    }

    bool implHasUserOutputExtractor(OutputExtractor const& oe) const final
    {
        return m_API->hasUserOutputExtractor(oe);
    }

    bool implRemoveUserOutputExtractor(OutputExtractor const& oe) final
    {
        return m_API->removeUserOutputExtractor(oe);
    }

    void drawContent()
    {
        m_Toolbar.draw();

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

        // draw navigator panel
        {
            if (!m_ShownModelState)
            {
                ImGui::TextDisabled("(no simulation selected)");
                return;
            }

            osc::SimulationModelStatePair& ms = *m_ShownModelState;

            auto resp = m_NavigatorPanel.draw(ms);

            if (resp.type == osc::NavigatorPanel::ResponseType::SelectionChanged)
            {
                ms.setSelected(resp.ptr);
            }
            else if (resp.type == osc::NavigatorPanel::ResponseType::HoverChanged)
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

        if (bool outputsPanelOldState = config.getIsPanelEnabled("Output Plots"))
        {
            bool outputsPanelState = outputsPanelOldState;
            if (ImGui::Begin("Output Plots", &outputsPanelState))
            {
                OSC_PERF("draw output plots panel");
                drawOutputWatchesTab();
            }
            ImGui::End();

            if (outputsPanelState != outputsPanelOldState)
            {
                App::upd().updConfig().setIsPanelEnabled("Output Plots", outputsPanelState);
            }
        }

        m_SimulationDetailsPanel.draw();
        m_LogViewerPanel.draw();
        m_PerfPanel.draw();
    }

    void drawOutputWatchesTab()
    {
        if (m_API->getNumUserOutputExtractors() <= 0)
        {
            ImGui::TextDisabled("(no outputs requested)");
            return;
        }

        if (IsAnyOutputExportableToCSV(*m_API))
        {
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
        }

        ImGui::Separator();
        ImGui::Dummy({0.0f, 5.0f});

        for (int i = 0; i < m_API->getNumUserOutputExtractors(); ++i)
        {
            osc::OutputExtractor output = m_API->getUserOutputExtractor(i);

            ImGui::PushID(i);
            SimulationOutputPlot plot{this, output, 64.0f};
            plot.draw();
            DrawOutputNameColumn(output, true, m_ShownModelState.get());
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

    void drawMainMenuWindowTab()
    {
        static std::vector<std::string> const g_EditorScreenPanels =
        {
            "Navigator",
            "Log",
            "Output Plots",
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
                std::string const name = "viewer" + std::to_string(i);

                bool enabled = true;
                if (ImGui::MenuItem(name.c_str(), nullptr, &enabled))
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
        bool shown = ImGui::Begin(name, &isOpen);
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

        // upate hover
        if (resp.isMousedOver && resp.hovertestResult != ms.getHovered())
        {
            ms.setHovered(resp.hovertestResult);
            osc::App::upd().requestRedraw();
        }

        // if left-clicked, update selection (can be empty)
        if (viewer.isLeftClicked() && resp.isMousedOver)
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

            if (viewer.isRightClicked() && resp.isMousedOver)
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

            std::string const name = "viewer" + std::to_string(i);

            bool isOpen = draw3DViewer(ms, viewer, name.c_str(), i);

            if (!isOpen)
            {
                m_ModelViewers.erase(m_ModelViewers.begin() + i);
                --i;
            }
        }
    }

    std::optional<osc::SimulationReport> TrySelectReportBasedOnScrubbing(osc::VirtualSimulation& sim)
    {
        std::optional<osc::SimulationReport> maybeReport = trySelectReportBasedOnScrubbing();

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

    // tab data
    UID m_ID;
    std::string m_Name = ICON_FA_PLAY " Simulation_" + std::to_string(g_SimulationNumber++);
    MainUIStateAPI* m_API;

    // underlying simulation being shown
    std::shared_ptr<Simulation> m_Simulation;

    // the modelstate that's being shown in the UI, based on scrubbing etc.
    //
    // if possible (i.e. there's a simulation report available), will be set each frame
    std::unique_ptr<SimulationModelStatePair> m_ShownModelState;

    // scrubbing state
    bool m_IsPlayingBack = true;
    float m_PlaybackSpeed = 1.0f;
    SimulationClock::time_point m_PlaybackStartSimtime = m_Simulation->getStartTime();
    std::chrono::system_clock::time_point m_PlaybackStartWallTime = std::chrono::system_clock::now();


    // UI widgets
    SimulationToolbar m_Toolbar{"##SimulationToolbar", this, m_Simulation};
    MainMenuFileTab m_MainMenuFileTab;
    MainMenuAboutTab m_MainMenuAboutTab;
    ComponentDetails m_ComponentDetailsWidget;
    PerfPanel m_PerfPanel{"Performance"};
    NavigatorPanel m_NavigatorPanel{"Navigator"};
    std::vector<UiModelViewer> m_ModelViewers = std::vector<UiModelViewer>(1);
    SimulationDetailsPanel m_SimulationDetailsPanel{"Simulation Details", this, m_Simulation};
    LogViewerPanel m_LogViewerPanel{"Log"};
};


// public API (PIMPL)

osc::SimulatorTab::SimulatorTab(MainUIStateAPI* api, std::shared_ptr<Simulation> simulation) :
    m_Impl{std::make_unique<Impl>(std::move(api), std::move(simulation))}
{
}

osc::SimulatorTab::SimulatorTab(SimulatorTab&&) noexcept = default;
osc::SimulatorTab& osc::SimulatorTab::operator=(SimulatorTab&&) noexcept = default;
osc::SimulatorTab::~SimulatorTab() noexcept = default;

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
