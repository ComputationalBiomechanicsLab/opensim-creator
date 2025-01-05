#include "SimulationTab.h"

#include <OpenSimCreator/Documents/OutputExtractors/ComponentOutputExtractor.h>
#include <OpenSimCreator/Documents/OutputExtractors/OutputExtractor.h>
#include <OpenSimCreator/Documents/Simulation/ISimulation.h>
#include <OpenSimCreator/Documents/Simulation/Simulation.h>
#include <OpenSimCreator/Documents/Simulation/SimulationClock.h>
#include <OpenSimCreator/Documents/Simulation/SimulationModelStatePair.h>
#include <OpenSimCreator/Documents/Simulation/SimulationReport.h>
#include <OpenSimCreator/UI/Events/OpenComponentContextMenuEvent.h>
#include <OpenSimCreator/UI/Shared/BasicWidgets.h>
#include <OpenSimCreator/UI/Shared/ComponentContextMenu.h>
#include <OpenSimCreator/UI/Shared/CoordinateEditorPanel.h>
#include <OpenSimCreator/UI/Shared/ModelStatusBar.h>
#include <OpenSimCreator/UI/Shared/ModelViewerPanel.h>
#include <OpenSimCreator/UI/Shared/ModelViewerPanelParameters.h>
#include <OpenSimCreator/UI/Shared/ModelViewerPanelRightClickEvent.h>
#include <OpenSimCreator/UI/Shared/NavigatorPanel.h>
#include <OpenSimCreator/UI/Shared/PropertiesPanel.h>
#include <OpenSimCreator/UI/Simulation/ISimulatorUIAPI.h>
#include <OpenSimCreator/UI/Simulation/OutputPlotsPanel.h>
#include <OpenSimCreator/UI/Simulation/SimulationDetailsPanel.h>
#include <OpenSimCreator/UI/Simulation/SimulationTabMainMenu.h>
#include <OpenSimCreator/UI/Simulation/SimulationToolbar.h>
#include <OpenSimCreator/UI/Simulation/SimulationUIPlaybackState.h>

#include <OpenSim/Common/Component.h>
#include <OpenSim/Simulation/Model/Model.h>
#include <oscar/Platform/App.h>
#include <oscar/Platform/Event.h>
#include <oscar/Platform/Events.h>
#include <oscar/Platform/IconCodepoints.h>
#include <oscar/Platform/os.h>
#include <oscar/UI/Events.h>
#include <oscar/UI/oscimgui.h>
#include <oscar/UI/Panels/LogViewerPanel.h>
#include <oscar/UI/Panels/PanelManager.h>
#include <oscar/UI/Panels/PerfPanel.h>
#include <oscar/UI/Popups/PopupManager.h>
#include <oscar/UI/Tabs/TabPrivate.h>
#include <oscar/Utils/EnumHelpers.h>
#include <oscar/Utils/LifetimedPtr.h>
#include <oscar/Utils/Perf.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdio>
#include <filesystem>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

using namespace osc;

namespace
{
    int GetNextSimulationNumber()
    {
        static std::atomic<int> s_SimulationNumber = 1;
        return s_SimulationNumber++;
    }
}

class osc::SimulationTab::Impl final :
    public TabPrivate,
    public ISimulatorUIAPI {
public:

    Impl(
        SimulationTab& owner,
        Widget& parent_,
        std::shared_ptr<Simulation> simulation_) :

        TabPrivate{owner, &parent_, OSC_ICON_PLAY " Simulation_" + std::to_string(GetNextSimulationNumber())},
        m_Simulation{std::move(simulation_)}
    {
        m_PanelManager->register_toggleable_panel(
            "Navigator",
            [this](std::string_view panelName)
            {
                return std::make_shared<NavigatorPanel>(
                    panelName,
                    m_ShownModelState,
                    [this](const OpenSim::ComponentPath& p)
                    {
                        auto popup = std::make_shared<ComponentContextMenu>(
                            "##componentcontextmenu",
                            this->owner(),
                            m_ShownModelState,
                            p.toString(),
                            ComponentContextMenuFlag::NoPlotVsCoordinate  // #922: shouldn't open in simulator screen
                        );
                        popup->open();
                        m_PopupManager.push_back(std::move(popup));
                    }
                );
            }
        );
        m_PanelManager->register_toggleable_panel(
            "Properties",
            [this](std::string_view panelName)
            {
                return std::make_shared<PropertiesPanel>(panelName, this->owner(), m_ShownModelState);
            }
        );
        m_PanelManager->register_toggleable_panel(
            "Log",
            [](std::string_view panelName)
            {
                return std::make_shared<LogViewerPanel>(panelName);
            }
        );
        m_PanelManager->register_toggleable_panel(
            "Coordinates",
            [this](std::string_view panelName)
            {
                return std::make_shared<CoordinateEditorPanel>(panelName, *parent(), m_ShownModelState);
            }
        );
        m_PanelManager->register_toggleable_panel(
            "Performance",
            [](std::string_view panelName)
            {
                return std::make_shared<PerfPanel>(panelName);
            }
        );
        m_PanelManager->register_toggleable_panel(
            "Output Watches",
            [this](std::string_view panelName)
            {
                return std::make_shared<OutputPlotsPanel>(
                    panelName,
                    m_Simulation->tryUpdEnvironment(),
                    this
                );
            }
        );
        m_PanelManager->register_toggleable_panel(
            "Simulation Details",
            [this](std::string_view panelName)
            {
                return std::make_shared<SimulationDetailsPanel>(
                    panelName,
                    this,
                    m_Simulation
                );
            }
        );
        m_PanelManager->register_spawnable_panel(
            "viewer",
            [this](std::string_view panelName)
            {
                ModelViewerPanelParameters params{
                    m_ShownModelState,
                    [this, menuName = std::string{panelName} + "_contextmenu"](const ModelViewerPanelRightClickEvent& e)
                    {
                        auto popup = std::make_shared<ComponentContextMenu>(
                            menuName,
                            this->owner(),
                            m_ShownModelState,
                            OpenSim::ComponentPath{e.componentAbsPathOrEmpty},
                            ComponentContextMenuFlag::NoPlotVsCoordinate  // #922: shouldn't open in simulator screen
                        );
                        popup->open();
                        m_PopupManager.push_back(std::move(popup));
                    },
                };

                return std::make_shared<ModelViewerPanel>(panelName, std::move(params));
            },
            1  // by default, open one viewer
        );
    }

    void on_mount()
    {
        App::upd().make_main_loop_waiting();
        m_PopupManager.on_mount();
        m_PanelManager->on_mount();
    }

    void on_unmount()
    {
        m_PanelManager->on_unmount();
        App::upd().make_main_loop_polling();
    }

    void on_tick()
    {
        if (m_PlaybackState == SimulationUIPlaybackState::Playing) {

            const SimulationClock::time_point playbackPos = implGetSimulationScrubTime();

            if ((m_PlaybackSpeed >= 0.0f and playbackPos < m_Simulation->getEndTime()) ||
                (m_PlaybackSpeed  < 0.0f and playbackPos > m_Simulation->getStartTime())) {

                // if there's still something to playback, ensure the screen is re-rendered to show it
                App::upd().request_redraw();
            }
            else if (m_LoopingState == SimulationUILoopingState::Looping) {
                // there's nothing to playback, but the UI wants to loop the playback, so loop it
                setSimulationScrubTime(m_Simulation->getStartTime());
            }
            else {
                // there's nothing to playback, so put playback into the stopped state
                m_PlaybackStartSimtime = playbackPos;
                m_PlaybackState = SimulationUIPlaybackState::Stopped;
            }
        }

        m_PanelManager->on_tick();
    }

    bool onEvent(Event& e)
    {
        if (auto* openPopupEvent = dynamic_cast<OpenPopupEvent*>(&e)) {
            if (openPopupEvent->has_tab()) {
                auto tab = openPopupEvent->take_tab();
                tab->open();
                m_PopupManager.push_back(std::move(tab));
                return true;
            }
        }
        else if (auto* namedPanel = dynamic_cast<OpenNamedPanelEvent*>(&e)) {
            m_PanelManager->set_toggleable_panel_activated(namedPanel->panel_name(), true);
            return true;
        }
        else if (auto* contextMenuEvent = dynamic_cast<OpenComponentContextMenuEvent*>(&e)) {
            auto popup = std::make_unique<ComponentContextMenu>(
                "##componentcontextmenu",
                this->owner(),
                m_ShownModelState,
                contextMenuEvent->path().toString(),
                ComponentContextMenuFlag::NoPlotVsCoordinate  // #922: shouldn't open in simulator screen
            );
            App::post_event<OpenPopupEvent>(owner(), std::move(popup));
            return true;
        }

        if (e.type() == EventType::KeyDown) {
            if (dynamic_cast<const KeyEvent&>(e).matches(Key::Space)) {
                togglePlaybackMode();
                return true;
            }
        }
        return false;
    }

    void onDrawMainMenu()
    {
        m_MainMenu.onDraw();
    }

    void onDraw()
    {
        ui::enable_dockspace_over_main_viewport();

        drawContent();
    }

private:
    void togglePlaybackMode()
    {
        static_assert(num_options<SimulationUIPlaybackState>() == 2);
        if (m_PlaybackState == SimulationUIPlaybackState::Playing) {
            // pause
            setSimulationPlaybackState(SimulationUIPlaybackState::Stopped);
        }
        else if (getSimulationScrubTime() >= m_Simulation->getEndTime()) {
            // replay
            setSimulationScrubTime(m_Simulation->getStartTime());
            setSimulationPlaybackState(SimulationUIPlaybackState::Playing);
        }
        else {
            // unpause
            setSimulationPlaybackState(SimulationUIPlaybackState::Playing);
        }
    }

    std::optional<SimulationReport> tryFindNthReportAfter(SimulationClock::time_point t, int offset = 0)
    {
        const ptrdiff_t numSimulationReports = m_Simulation->getNumReports();

        if (numSimulationReports == 0) {
            return std::nullopt;
        }

        ptrdiff_t zeroethReportIndex = static_cast<ptrdiff_t>(numSimulationReports) - 1;
        for (ptrdiff_t i = 0; i < numSimulationReports; ++i) {
            const SimulationReport r = m_Simulation->getSimulationReport(i);
            if (r.getTime() >= t) {
                zeroethReportIndex = i;
                break;
            }
        }

        const ptrdiff_t reportIndex = zeroethReportIndex + offset;
        if (0 <= reportIndex and reportIndex < numSimulationReports) {
            return m_Simulation->getSimulationReport(reportIndex);
        }
        else {
            return std::nullopt;
        }
    }

    const ISimulation& implGetSimulation() const final
    {
        return *m_Simulation;
    }

    ISimulation& implUpdSimulation() final
    {
        return *m_Simulation;
    }

    SimulationUIPlaybackState implGetSimulationPlaybackState() final
    {
        return m_PlaybackState;
    }

    void implSetSimulationPlaybackState(SimulationUIPlaybackState newState) final
    {
        if (newState == SimulationUIPlaybackState::Playing) {
            m_PlaybackStartWallTime = std::chrono::system_clock::now();
            m_PlaybackState = newState;
        }
        else {
            m_PlaybackStartSimtime = getSimulationScrubTime();
            m_PlaybackState = newState;
        }
    }

    SimulationUILoopingState implGetSimulationLoopingState() const final
    {
        return m_LoopingState;
    }

    void implSetSimulationLoopingState(SimulationUILoopingState s) final
    {
        m_LoopingState = s;
    }

    float implGetSimulationPlaybackSpeed() final
    {
        return m_PlaybackSpeed;
    }

    void implSetSimulationPlaybackSpeed(float v) final
    {
        m_PlaybackSpeed = v;
    }

    SimulationClock::time_point implGetSimulationScrubTime() final
    {
        if (m_PlaybackState == SimulationUIPlaybackState::Stopped) {
            return m_PlaybackStartSimtime;
        }

        // else: map the computer's wall time onto simulation time

        const ptrdiff_t nReports = m_Simulation->getNumReports();
        if (nReports <= 0) {
            return m_Simulation->getStartTime();
        }
        else {
            std::chrono::system_clock::time_point wallNow = std::chrono::system_clock::now();
            std::chrono::system_clock::duration wallDur = wallNow - m_PlaybackStartWallTime;

            const SimulationClock::duration simDur = m_PlaybackSpeed * SimulationClock::duration{wallDur};
            const SimulationClock::time_point simNow = m_PlaybackStartSimtime + simDur;
            const SimulationClock::time_point simEarliest = m_Simulation->getSimulationReport(0).getTime();
            const SimulationClock::time_point simLatest = m_Simulation->getSimulationReport(nReports - 1).getTime();

            if (simNow < simEarliest) {
                return simEarliest;
            }
            else if (simNow > simLatest) {
                return simLatest;
            }
            else {
                return simNow;
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
        const std::optional<SimulationReport> maybePrev = tryFindNthReportAfter(getSimulationScrubTime(), -1);
        if (maybePrev) {
            setSimulationScrubTime(maybePrev->getTime());
        }
    }

    void implStepForward() final
    {
        const std::optional<SimulationReport> maybeNext = tryFindNthReportAfter(getSimulationScrubTime(), 1);
        if (maybeNext) {
            setSimulationScrubTime(maybeNext->getTime());
        }
    }

    std::optional<SimulationReport> implTrySelectReportBasedOnScrubbing() final
    {
        return tryFindNthReportAfter(getSimulationScrubTime());
    }

    SimulationModelStatePair* implTryGetCurrentSimulationState() final
    {
        return m_ShownModelState.get();
    }

    void drawContent()
    {
        m_Toolbar.onDraw();

        // only draw content if a simulation report is available
        const std::optional<SimulationReport> maybeReport = trySelectReportBasedOnScrubbing();
        if (maybeReport) {
            m_ShownModelState->setSimulation(m_Simulation);
            m_ShownModelState->setSimulationReport(*maybeReport);

            OSC_PERF("draw simulation screen");
            m_PanelManager->on_draw();
            m_StatusBar.onDraw();
            m_PopupManager.on_draw();
        }
        else {
            ui::begin_panel("Waiting for simulation");
            ui::draw_text_disabled("(waiting for first simulation state)");
            ui::end_panel();

            // and show the log, so that the user can see any errors from the integrator (#628)
            //
            // this might be less necessary once the integrator correctly reports errors to
            // this UI panel (#625)
            LogViewerPanel p{"Log"};
            p.on_draw();
        }
    }

    // underlying simulation being shown
    std::shared_ptr<Simulation> m_Simulation;

    // the modelstate that's being shown in the UI, based on scrubbing etc.
    //
    // if possible (i.e. there's a simulation report available), will be set each frame
    std::shared_ptr<SimulationModelStatePair> m_ShownModelState = std::make_shared<SimulationModelStatePair>();

    // scrubbing state
    SimulationUIPlaybackState m_PlaybackState = SimulationUIPlaybackState::Playing;
    SimulationUILoopingState m_LoopingState = SimulationUILoopingState::PlayOnce;
    float m_PlaybackSpeed = 1.0f;
    SimulationClock::time_point m_PlaybackStartSimtime = m_Simulation->getStartTime();
    std::chrono::system_clock::time_point m_PlaybackStartWallTime = std::chrono::system_clock::now();

    // manager for toggleable and spawnable UI panels
    std::shared_ptr<PanelManager> m_PanelManager = std::make_shared<PanelManager>();

    // non-toggleable UI panels/menus/toolbars
    SimulationTabMainMenu m_MainMenu{*parent(), m_Simulation, m_PanelManager};
    SimulationToolbar m_Toolbar{"##SimulationToolbar", this, m_Simulation};
    ModelStatusBar m_StatusBar{*parent(), m_ShownModelState};

    // manager for popups that are open in this tab
    PopupManager m_PopupManager;
};


osc::SimulationTab::SimulationTab(
    Widget& parent_,
    std::shared_ptr<Simulation> simulation_) :

    Tab{std::make_unique<Impl>(*this, parent_, std::move(simulation_))}
{}
void osc::SimulationTab::impl_on_mount() { private_data().on_mount(); }
void osc::SimulationTab::impl_on_unmount() { private_data().on_unmount(); }
bool osc::SimulationTab::impl_on_event(Event& e) { return private_data().onEvent(e); }
void osc::SimulationTab::impl_on_tick() { private_data().on_tick(); }
void osc::SimulationTab::impl_on_draw_main_menu() { private_data().onDrawMainMenu(); }
void osc::SimulationTab::impl_on_draw() { private_data().onDraw(); }
