#include "SimulatorTab.hpp"

#include "src/Bindings/ImGuiHelpers.hpp"
#include "src/OpenSimBindings/MiddlewareAPIs/MainUIStateAPI.hpp"
#include "src/OpenSimBindings/MiddlewareAPIs/SimulatorUIAPI.hpp"
#include "src/OpenSimBindings/Widgets/BasicWidgets.hpp"
#include "src/OpenSimBindings/Widgets/MainMenu.hpp"
#include "src/OpenSimBindings/Widgets/NavigatorPanel.hpp"
#include "src/OpenSimBindings/Widgets/OutputPlotsPanel.hpp"
#include "src/OpenSimBindings/Widgets/SelectionDetailsPanel.hpp"
#include "src/OpenSimBindings/Widgets/SimulationDetailsPanel.hpp"
#include "src/OpenSimBindings/Widgets/SimulationOutputPlot.hpp"
#include "src/OpenSimBindings/Widgets/SimulationScrubber.hpp"
#include "src/OpenSimBindings/Widgets/SimulationToolbar.hpp"
#include "src/OpenSimBindings/Widgets/SimulationViewerPanel.hpp"
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
#include "src/Widgets/PanelManager.hpp"
#include "src/Widgets/PerfPanel.hpp"
#include "src/Widgets/WindowMenu.hpp"

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

class osc::SimulatorTab::Impl final : public SimulatorUIAPI {
public:

    Impl(
        MainUIStateAPI* api,
        std::shared_ptr<Simulation> simulation) :

        m_API{std::move(api)},
        m_Simulation{std::move(simulation)}
    {
        // register panels

        m_PanelManager->registerToggleablePanel(
            "Performance",
            [](std::string_view panelName)
            {
                return std::make_shared<PerfPanel>(panelName);
            }
        );
        m_PanelManager->registerToggleablePanel(
            "Navigator",
            [this](std::string_view panelName)
            {
                return std::make_shared<NavigatorPanel>(panelName, m_ShownModelState);
            }
        );
        m_PanelManager->registerToggleablePanel(
            "Selection Details",
            [this](std::string_view panelName)
            {
                return std::make_shared<SelectionDetailsPanel>(panelName, this);
            }
        );
        m_PanelManager->registerToggleablePanel(
            "Output Plots",
            [this](std::string_view panelName)
            {
                return std::make_shared<OutputPlotsPanel>(panelName, m_API, this);
            }
        );
        m_PanelManager->registerToggleablePanel(
            "Simulation Details",
            [this](std::string_view panelName)
            {
                return std::make_shared<SimulationDetailsPanel>(panelName, this, m_Simulation);
            }
        );
        m_PanelManager->registerToggleablePanel(
            "Log",
            [](std::string_view panelName)
            {
                return std::make_shared<LogViewerPanel>(panelName);
            }
        );
        m_PanelManager->registerSpawnablePanel(
            "viewer",
            [this](std::string_view panelName)
            {
                return std::make_shared<SimulationViewerPanel>(panelName, m_ShownModelState, m_API);
            }
        );

        // by default, open one viewer
        m_PanelManager->pushDynamicPanel(
            "viewer",
            std::make_shared<SimulationViewerPanel>(
                m_PanelManager->computeSuggestedDynamicPanelName("viewer"),
                m_ShownModelState,
                m_API
            )
        );

        m_PanelManager->activateAllDefaultOpenPanels();
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
        m_MainMenuWindowTab.draw();
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

    SimulationModelStatePair* implTryGetCurrentSimulationState() final
    {
        return m_ShownModelState.get();
    }

    void drawContent()
    {
        // ensure m_ShownModelState is populated, if possible
        {
            OSC_PERF("process simulation report");

            std::optional<osc::SimulationReport> maybeReport = TrySelectReportBasedOnScrubbing(*m_Simulation);
            if (maybeReport)
            {
                m_ShownModelState->setSimulation(m_Simulation);
                m_ShownModelState->setSimulationReport(*maybeReport);
            }
        }

        OSC_PERF("draw simulation screen");

        m_Toolbar.draw();
        m_PanelManager->drawAllActivatedPanels();
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
    std::shared_ptr<SimulationModelStatePair> m_ShownModelState = std::make_shared<osc::SimulationModelStatePair>();

    // scrubbing state
    bool m_IsPlayingBack = true;
    float m_PlaybackSpeed = 1.0f;
    SimulationClock::time_point m_PlaybackStartSimtime = m_Simulation->getStartTime();
    std::chrono::system_clock::time_point m_PlaybackStartWallTime = std::chrono::system_clock::now();

    // manager for toggleable and spawnable UI panels
    std::shared_ptr<PanelManager> m_PanelManager = std::make_shared<PanelManager>();

    // static (non-toggleable) UI panels/menus/toolbars
    MainMenuFileTab m_MainMenuFileTab;
    MainMenuAboutTab m_MainMenuAboutTab;
    WindowMenu m_MainMenuWindowTab{m_PanelManager};
    SimulationToolbar m_Toolbar{"##SimulationToolbar", this, m_Simulation};
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
