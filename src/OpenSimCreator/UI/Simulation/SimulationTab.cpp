#include "SimulationTab.h"

#include <OpenSimCreator/Documents/OutputExtractors/ComponentOutputExtractor.h>
#include <OpenSimCreator/Documents/OutputExtractors/OutputExtractor.h>
#include <OpenSimCreator/Documents/Simulation/ISimulation.h>
#include <OpenSimCreator/Documents/Simulation/Simulation.h>
#include <OpenSimCreator/Documents/Simulation/SimulationClock.h>
#include <OpenSimCreator/Documents/Simulation/SimulationModelStatePair.h>
#include <OpenSimCreator/Documents/Simulation/SimulationReport.h>
#include <OpenSimCreator/UI/IMainUIStateAPI.h>
#include <OpenSimCreator/UI/Shared/BasicWidgets.h>
#include <OpenSimCreator/UI/Shared/NavigatorPanel.h>
#include <OpenSimCreator/UI/Simulation/ISimulatorUIAPI.h>
#include <OpenSimCreator/UI/Simulation/ModelStatePairContextMenu.h>
#include <OpenSimCreator/UI/Simulation/OutputPlotsPanel.h>
#include <OpenSimCreator/UI/Simulation/SelectionDetailsPanel.h>
#include <OpenSimCreator/UI/Simulation/SimulationDetailsPanel.h>
#include <OpenSimCreator/UI/Simulation/SimulationTabMainMenu.h>
#include <OpenSimCreator/UI/Simulation/SimulationToolbar.h>
#include <OpenSimCreator/UI/Simulation/SimulationUIPlaybackState.h>
#include <OpenSimCreator/UI/Simulation/SimulationViewerPanel.h>
#include <OpenSimCreator/UI/Simulation/SimulationViewerPanelParameters.h>
#include <OpenSimCreator/UI/Simulation/SimulationViewerRightClickEvent.h>

#include <OpenSim/Common/Component.h>
#include <OpenSim/Simulation/Model/Model.h>
#include <oscar/Platform/App.h>
#include <oscar/Platform/Event.h>
#include <oscar/Platform/IconCodepoints.h>
#include <oscar/Platform/os.h>
#include <oscar/UI/oscimgui.h>
#include <oscar/UI/Panels/LogViewerPanel.h>
#include <oscar/UI/Panels/PanelManager.h>
#include <oscar/UI/Panels/PerfPanel.h>
#include <oscar/UI/Widgets/PopupManager.h>
#include <oscar/Utils/EnumHelpers.h>
#include <oscar/Utils/ParentPtr.h>
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

class osc::SimulationTab::Impl final : public ISimulatorUIAPI {
public:

    Impl(
        const ParentPtr<IMainUIStateAPI>& parent_,
        std::shared_ptr<Simulation> simulation_) :

        m_Parent{parent_},
        m_Simulation{std::move(simulation_)}
    {
        // register panels

        m_PanelManager->register_toggleable_panel(
            "Performance",
            [](std::string_view panelName)
            {
                return std::make_shared<PerfPanel>(panelName);
            }
        );
        m_PanelManager->register_toggleable_panel(
            "Navigator",
            [this](std::string_view panelName)
            {
                return std::make_shared<NavigatorPanel>(
                    panelName,
                    m_ShownModelState,
                    [this](const OpenSim::ComponentPath& p)
                    {
                        auto popup = std::make_shared<ModelStatePairContextMenu>(
                            "##componentcontextmenu",
                            m_ShownModelState,
                            m_Parent,
                            p.toString()
                        );
                        popup->open();
                        m_PopupManager.push_back(std::move(popup));
                    }
                );
            }
        );
        m_PanelManager->register_toggleable_panel(
            "Selection Details",
            [this](std::string_view panelName)
            {
                return std::make_shared<SelectionDetailsPanel>(
                    panelName,
                    this
                );
            }
        );
        m_PanelManager->register_toggleable_panel(
            "Output Plots",
            [this](std::string_view panelName)
            {
                return std::make_shared<OutputPlotsPanel>(
                    panelName,
                    m_Parent,
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
        m_PanelManager->register_toggleable_panel(
            "Log",
            [](std::string_view panelName)
            {
                return std::make_shared<LogViewerPanel>(panelName);
            }
        );
        m_PanelManager->register_spawnable_panel(
            "viewer",
            [this](std::string_view panelName)
            {
                SimulationViewerPanelParameters params
                {
                    m_ShownModelState,
                    [this, menuName = std::string{panelName} + "_contextmenu"](const SimulationViewerRightClickEvent& e)
                    {
                        auto popup = std::make_shared<ModelStatePairContextMenu>(
                            menuName,
                            m_ShownModelState,
                            m_Parent,
                            e.maybeComponentAbsPath
                        );
                        popup->open();
                        m_PopupManager.push_back(std::move(popup));
                    },
                };

                return std::make_shared<SimulationViewerPanel>(panelName, std::move(params));
            },
            1  // by default, open one viewer
        );
    }

    UID getID() const
    {
        return m_ID;
    }

    CStringView getName() const
    {
        return m_Name;
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

    bool onEvent(const Event& e)
    {
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

        if (numSimulationReports == 0)
        {
            return std::nullopt;
        }

        ptrdiff_t zeroethReportIndex = static_cast<ptrdiff_t>(numSimulationReports) - 1;
        for (ptrdiff_t i = 0; i < numSimulationReports; ++i)
        {
            SimulationReport r = m_Simulation->getSimulationReport(i);
            if (r.getTime() >= t)
            {
                zeroethReportIndex = i;
                break;
            }
        }

        const ptrdiff_t reportIndex = zeroethReportIndex + offset;
        if (0 <= reportIndex && reportIndex < numSimulationReports) {
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

    int implGetNumUserOutputExtractors() const final
    {
        return m_Parent->getNumUserOutputExtractors();
    }

    const OutputExtractor& implGetUserOutputExtractor(int i) const final
    {
        return m_Parent->getUserOutputExtractor(i);
    }

    void implAddUserOutputExtractor(const OutputExtractor& outputExtractor) final
    {
        m_Parent->addUserOutputExtractor(outputExtractor);
    }

    void implRemoveUserOutputExtractor(int i) final
    {
        m_Parent->removeUserOutputExtractor(i);
    }

    bool implHasUserOutputExtractor(const OutputExtractor& oe) const final
    {
        return m_Parent->hasUserOutputExtractor(oe);
    }

    bool implRemoveUserOutputExtractor(const OutputExtractor& oe) final
    {
        return m_Parent->removeUserOutputExtractor(oe);
    }

    bool implOverwriteOrAddNewUserOutputExtractor(const OutputExtractor& old, const OutputExtractor& newer) final
    {
        return m_Parent->overwriteOrAddNewUserOutputExtractor(old, newer);
    }

    SimulationModelStatePair* implTryGetCurrentSimulationState() final
    {
        return m_ShownModelState.get();
    }

    void drawContent()
    {
        m_Toolbar.onDraw();

        // only draw content if a simulation report is available
        std::optional<SimulationReport> maybeReport = trySelectReportBasedOnScrubbing();
        if (maybeReport)
        {
            m_ShownModelState->setSimulation(m_Simulation);
            m_ShownModelState->setSimulationReport(*maybeReport);

            OSC_PERF("draw simulation screen");
            m_PanelManager->on_draw();
            m_PopupManager.on_draw();
        }
        else
        {
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

    // tab data
    UID m_ID;
    ParentPtr<IMainUIStateAPI> m_Parent;
    std::string m_Name = OSC_ICON_PLAY " Simulation_" + std::to_string(GetNextSimulationNumber());

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
    SimulationTabMainMenu m_MainMenu{m_Parent, m_Simulation, m_PanelManager};
    SimulationToolbar m_Toolbar{"##SimulationToolbar", this, m_Simulation};

    // manager for popups that are open in this tab
    PopupManager m_PopupManager;
};


osc::SimulationTab::SimulationTab(
    const ParentPtr<IMainUIStateAPI>& parent_,
    std::shared_ptr<Simulation> simulation_) :

    m_Impl{std::make_unique<Impl>(parent_, std::move(simulation_))}
{}
osc::SimulationTab::SimulationTab(SimulationTab&&) noexcept = default;
osc::SimulationTab& osc::SimulationTab::operator=(SimulationTab&&) noexcept = default;
osc::SimulationTab::~SimulationTab() noexcept = default;

UID osc::SimulationTab::impl_get_id() const
{
    return m_Impl->getID();
}

CStringView osc::SimulationTab::impl_get_name() const
{
    return m_Impl->getName();
}

void osc::SimulationTab::impl_on_mount()
{
    m_Impl->on_mount();
}

void osc::SimulationTab::impl_on_unmount()
{
    m_Impl->on_unmount();
}

bool osc::SimulationTab::impl_on_event(const Event& e)
{
    return m_Impl->onEvent(e);
}

void osc::SimulationTab::impl_on_tick()
{
    m_Impl->on_tick();
}

void osc::SimulationTab::impl_on_draw_main_menu()
{
    m_Impl->onDrawMainMenu();
}

void osc::SimulationTab::impl_on_draw()
{
    m_Impl->onDraw();
}
