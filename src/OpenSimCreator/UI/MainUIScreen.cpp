#include "MainUIScreen.h"

#include <OpenSimCreator/Documents/Model/UndoableModelStatePair.h>
#include <OpenSimCreator/Documents/Simulation/ForwardDynamicSimulatorParams.h>
#include <OpenSimCreator/OutputExtractors/OutputExtractor.h>
#include <OpenSimCreator/UI/IMainUIStateAPI.h>
#include <OpenSimCreator/UI/LoadingTab.h>
#include <OpenSimCreator/UI/SplashTab.h>
#include <OpenSimCreator/UI/MeshImporter/MeshImporterTab.h>
#include <OpenSimCreator/UI/ModelEditor/ModelEditorTab.h>
#include <OpenSimCreator/Utils/ParamBlock.h>

#include <IconsFontAwesome5.h>
#include <oscar/Platform/App.h>
#include <oscar/Platform/AppConfig.h>
#include <oscar/Platform/Log.h>
#include <oscar/Platform/os.h>
#include <oscar/Platform/Screenshot.h>
#include <oscar/UI/ImGuiHelpers.h>
#include <oscar/UI/oscimgui.h>
#include <oscar/UI/oscimgui_internal.h>
#include <oscar/UI/Tabs/ErrorTab.h>
#include <oscar/UI/Tabs/ITab.h>
#include <oscar/UI/Tabs/ScreenshotTab.h>
#include <oscar/UI/Tabs/TabRegistry.h>
#include <oscar/UI/ui_context.h>
#include <oscar/UI/Widgets/SaveChangesPopup.h>
#include <oscar/UI/Widgets/SaveChangesPopupConfig.h>
#include <oscar/Utils/Assertions.h>
#include <oscar/Utils/CStringView.h>
#include <oscar/Utils/ParentPtr.h>
#include <oscar/Utils/Perf.h>
#include <oscar/Utils/UID.h>
#include <SDL_events.h>

#include <algorithm>
#include <functional>
#include <iterator>
#include <limits>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

using namespace osc;

namespace
{
    std::unique_ptr<ITab> LoadConfigurationDefinedTabIfNecessary(
        AppConfig const& config,
        TabRegistry const& tabRegistry,
        ParentPtr<ITabHost> const& api)
    {
        if (std::optional<std::string> maybeRequestedTab = config.getInitialTabOverride())
        {
            if (std::optional<TabRegistryEntry> maybeEntry = tabRegistry.getByName(*maybeRequestedTab))
            {
                return maybeEntry->createTab(api);
            }

            log_warn("%s: cannot find a tab with this name in the tab registry: ignoring", maybeRequestedTab->c_str());
            log_warn("available tabs are:");
            for (size_t i = 0; i < tabRegistry.size(); ++i)
            {
                log_warn("    %s", tabRegistry[i].getName().c_str());
            }
        }

        return nullptr;
    }
}

class osc::MainUIScreen::Impl final :
    public IMainUIStateAPI,
    public std::enable_shared_from_this<Impl> {
public:

    UID addTab(std::unique_ptr<ITab> tab)
    {
        return implAddTab(std::move(tab));
    }

    void open(std::filesystem::path const& p)
    {
        addTab(std::make_unique<LoadingTab>(getTabHostAPI(), p));
    }

    void onMount()
    {
        if (!std::exchange(m_HasBeenMountedBefore, true))
        {
            // on first mount, place the splash tab at the front of the tabs collection
            m_Tabs.insert(m_Tabs.begin(), std::make_unique<SplashTab>(getTabHostAPI()));

            // if the application configuration has requested that a specific tab should be opened,
            // then try looking it up and open it
            if (auto tab = LoadConfigurationDefinedTabIfNecessary(App::get().getConfig(), *App::singleton<TabRegistry>(), getTabHostAPI()))
            {
                addTab(std::move(tab));
            }

            // focus on the rightmost tab
            if (!m_Tabs.empty())
            {
                m_RequestedTab = m_Tabs.back()->getID();
            }
        }

        ui::context::Init();
    }

    void onUnmount()
    {
        // unmount the active tab before unmounting this (host) screen
        if (ITab* active = getActiveTab())
        {
            try
            {
                active->onUnmount();
            }
            catch (std::exception const& ex)
            {
                // - the tab is faulty in some way
                // - soak up the exception to prevent the whole application from terminating
                // - and emit the error to the log, because we have to assume that this
                //   screen is about to die (it's being unmounted)
                log_error("MainUIScreen::onUnmount: unmounting active tab threw an exception: %s", ex.what());
            }

            m_ActiveTabID = UID::empty();
        }

        ui::context::Shutdown();
    }

    void onEvent(SDL_Event const& e)
    {
        if (e.type == SDL_KEYUP &&
            e.key.keysym.mod & (KMOD_CTRL | KMOD_GUI) &&
            e.key.keysym.scancode == SDL_SCANCODE_P)
        {
            // Ctrl+/Super+P operates as a "take a screenshot" request
            m_MaybeScreenshotRequest = App::upd().requestScreenshot();
        }
        else if (ui::context::OnEvent(e))
        {
            // event was pumped into ImGui - it shouldn't be pumped into the active tab
            m_ShouldRequestRedraw = true;
        }
        else if (e.type == SDL_QUIT)
        {
            // it's a quit *request* event, which must be pumped into all tabs
            //
            // note: some tabs may block the quit event, e.g. because they need to
            //       ask the user whether they want to save changes or not

            bool quitHandled = false;
            for (int i = 0; i < static_cast<int>(m_Tabs.size()); ++i)
            {
                try
                {
                    quitHandled = m_Tabs[i]->onEvent(e) || quitHandled;
                }
                catch (std::exception const& ex)
                {
                    log_error("MainUIScreen::onEvent: exception thrown by tab: %s", ex.what());

                    // - the tab is faulty in some way
                    // - soak up the exception to prevent the whole application from terminating
                    // - then create a new tab containing the error message, so the user can see the error
                    UID id = addTab(std::make_unique<ErrorTab>(getTabHostAPI(), ex));
                    selectTab(id);
                    implCloseTab(m_Tabs[i]->getID());
                }
            }

            if (!quitHandled)
            {
                // if no tab handled the quit event, treat it as-if the user
                // has tried to close all tabs

                for (auto const& tab : m_Tabs)
                {
                    implCloseTab(tab->getID());
                }
                m_QuitRequested = true;
            }

            // handle any deletion-related side-effects (e.g. showing save prompt)
            handleDeletedTabs();

            if (!quitHandled && (!m_MaybeSaveChangesPopup || !m_MaybeSaveChangesPopup->isOpen()))
            {
                // - if no tab handled a quit event
                // - and the UI isn't currently showing a save prompt
                // - then it's safe to outright quit the application from this screen

                App::upd().requestQuit();
            }
        }
        else if (ITab* active = getActiveTab())
        {
            // all other event types are only pumped into the active tab

            bool handled = false;
            try
            {
                handled = active->onEvent(e);
            }
            catch (std::exception const& ex)
            {
                log_error("MainUIScreen::onEvent: exception thrown by tab: %s", ex.what());

                // - the tab is faulty in some way
                // - soak up the exception to prevent the whole application from terminating
                // - then create a new tab containing the error message, so the user can see the error
                UID id = addTab(std::make_unique<ErrorTab>(getTabHostAPI(), ex));
                selectTab(id);
                implCloseTab(m_ActiveTabID);
            }

            // the event may have triggered tab deletions
            handleDeletedTabs();

            if (handled)
            {
                m_ShouldRequestRedraw = true;
                return;
            }
        }
    }

    void onTick()
    {
        // tick all the tabs, because they may internally be polling something (e.g.
        // updating something as a simulation runs)
        for (size_t i = 0; i < m_Tabs.size(); ++i)
        {
            try
            {
                m_Tabs[i]->onTick();
            }
            catch (std::exception const& ex)
            {

                log_error("MainUIScreen::onTick: tab thrown an exception: %s", ex.what());

                // - the tab is faulty in some way
                // - soak up the exception to prevent the whole application from terminating
                // - then create a new tab containing the error message, so the user can see the error
                UID id = addTab(std::make_unique<ErrorTab>(getTabHostAPI(), ex));
                selectTab(id);
                implCloseTab(m_Tabs[i]->getID());
            }
        }

        // clear the flagged-to-be-deleted tabs
        handleDeletedTabs();

        // handle any currently-active user screenshot requests
        tryHandleScreenshotRequest();
    }

    void onDraw()
    {
        OSC_PERF("MainUIScreen/draw");

        {
            OSC_PERF("MainUIScreen/clearScreen");
            App::upd().clearScreen({0.0f, 0.0f, 0.0f, 0.0f});
        }

        ui::context::NewFrame();

        {
            OSC_PERF("MainUIScreen/drawUIContent");
            drawUIContent();
        }

        if (m_ImguiWasAggressivelyReset)
        {
            if (m_RequestedTab == UID::empty())
            {
                m_RequestedTab = m_ActiveTabID;
            }
            m_ActiveTabID = UID::empty();

            ui::context::Shutdown();
            ui::context::Init();
            App::upd().requestRedraw();
            m_ImguiWasAggressivelyReset = false;

            return;
        }

        {
            OSC_PERF("MainUIScreen/ui::context::Render()");
            ui::context::Render();
        }

        if (m_ShouldRequestRedraw)
        {
            App::upd().requestRedraw();
            m_ShouldRequestRedraw = false;
        }
    }

    ParamBlock const& implGetSimulationParams() const final
    {
        return m_SimulationParams;
    }

    ParamBlock& implUpdSimulationParams() final
    {
        return m_SimulationParams;
    }

    int implGetNumUserOutputExtractors() const final
    {
        return static_cast<int>(m_UserOutputExtractors.size());
    }

    OutputExtractor const& implGetUserOutputExtractor(int idx) const final
    {
        return m_UserOutputExtractors.at(idx);
    }

    void implAddUserOutputExtractor(OutputExtractor const& output) final
    {
        m_UserOutputExtractors.push_back(output);
        App::upd().updConfig().setIsPanelEnabled("Output Watches", true);
    }

    void implRemoveUserOutputExtractor(int idx) final
    {
        OSC_ASSERT(0 <= idx && idx < static_cast<int>(m_UserOutputExtractors.size()));
        m_UserOutputExtractors.erase(m_UserOutputExtractors.begin() + idx);
    }

    bool implHasUserOutputExtractor(OutputExtractor const& oe) const final
    {
        return std::find(m_UserOutputExtractors.begin(), m_UserOutputExtractors.end(), oe) != m_UserOutputExtractors.end();
    }

    bool implRemoveUserOutputExtractor(OutputExtractor const& oe) final
    {
        return std::erase(m_UserOutputExtractors, oe) > 0;
    }

private:

    ParentPtr<IMainUIStateAPI> getTabHostAPI()
    {
        return ParentPtr<IMainUIStateAPI>{shared_from_this()};
    }
    void drawTabSpecificMenu()
    {
        OSC_PERF("MainUIScreen/drawTabSpecificMenu");

        if (ui::BeginMainViewportTopBar("##TabSpecificMenuBar"))
        {
            if (ui::BeginMenuBar())
            {
                if (ITab* active = getActiveTab())
                {
                    try
                    {
                        active->onDrawMainMenu();
                    }
                    catch (std::exception const& ex)
                    {
                        log_error("MainUIScreen::drawTabSpecificMenu: tab thrown an exception: %s", ex.what());

                        // - the tab is faulty in some way
                        // - soak up the exception to prevent the whole application from terminating
                        // - then create a new tab containing the error message, so the user can see the error
                        UID id = addTab(std::make_unique<ErrorTab>(getTabHostAPI(), ex));
                        selectTab(id);
                        implCloseTab(m_ActiveTabID);
                    }

                    if (m_ImguiWasAggressivelyReset)
                    {
                        return;  // must return here to prevent the ImGui End calls from erroring
                    }
                }
                ui::EndMenuBar();
            }
            ui::End();
            handleDeletedTabs();
        }
    }

    void drawTabBar()
    {
        OSC_PERF("MainUIScreen/drawTabBar");

        ui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{ ui::GetStyle().FramePadding.x + 2.0f, ui::GetStyle().FramePadding.y + 2.0f });
        ui::PushStyleVar(ImGuiStyleVar_ItemInnerSpacing, ImVec2{ 5.0f, 0.0f });
        ui::PushStyleVar(ImGuiStyleVar_TabRounding, 10.0f);
        ui::PushStyleVar(ImGuiStyleVar_FrameRounding, 10.0f);
        if (ui::BeginMainViewportTopBar("##TabBarViewport"))
        {
            if (ui::BeginMenuBar())
            {
                if (ui::BeginTabBar("##TabBar"))
                {
                    for (size_t i = 0; i < m_Tabs.size(); ++i)
                    {
                        ImGuiTabItemFlags flags = ImGuiTabItemFlags_NoReorder;

                        if (i == 0)
                        {
                            flags |= ImGuiTabItemFlags_NoCloseButton;  // splash screen
                        }

                        if (m_Tabs[i]->isUnsaved())
                        {
                            flags |= ImGuiTabItemFlags_UnsavedDocument;
                        }

                        if (m_Tabs[i]->getID() == m_RequestedTab)
                        {
                            flags |= ImGuiTabItemFlags_SetSelected;
                        }

                        if (m_Tabs[i]->getID() == m_ActiveTabID && m_Tabs[i]->getName() != m_ActiveTabNameLastFrame)
                        {
                            flags |= ImGuiTabItemFlags_SetSelected;
                            m_ActiveTabNameLastFrame = m_Tabs[i]->getName();
                        }

                        ui::PushID(m_Tabs[i].get());
                        bool active = true;

                        if (ui::BeginTabItem(m_Tabs[i]->getName(), &active, flags))
                        {
                            if (m_Tabs[i]->getID() != m_ActiveTabID)
                            {
                                if (ITab* activeTab = getActiveTab())
                                {
                                    activeTab->onUnmount();
                                }
                                m_Tabs[i]->onMount();
                            }

                            m_ActiveTabID = m_Tabs[i]->getID();
                            m_ActiveTabNameLastFrame = m_Tabs[i]->getName();

                            if (m_RequestedTab == m_ActiveTabID)
                            {
                                m_RequestedTab = UID::empty();
                            }

                            if (m_ImguiWasAggressivelyReset)
                            {
                                return;
                            }

                            ui::EndTabItem();
                        }

                        ui::PopID();
                        if (!active && i != 0)  // can't close the splash tab
                        {
                            implCloseTab(m_Tabs[i]->getID());
                        }
                    }

                    // adding buttons to tab bars: https://github.com/ocornut/imgui/issues/3291
                    ui::TabItemButton(ICON_FA_PLUS);

                    if (ui::BeginPopupContextItem("popup", ImGuiPopupFlags_MouseButtonLeft))
                    {
                        drawAddNewTabMenu();
                        ui::EndPopup();
                    }

                    ui::EndTabBar();
                }
                ui::EndMenuBar();
            }

            ui::End();
            handleDeletedTabs();
        }
        ui::PopStyleVar(4);
    }

    void drawUIContent()
    {
        drawTabSpecificMenu();

        if (m_ImguiWasAggressivelyReset)
        {
            return;
        }

        drawTabBar();

        if (m_ImguiWasAggressivelyReset)
        {
            return;
        }

        // draw the active tab (if any)
        if (ITab* active = getActiveTab())
        {
            try
            {
                OSC_PERF("MainUIScreen/drawActiveTab");
                active->onDraw();
            }
            catch (std::exception const& ex)
            {
                log_error("MainUIScreen::drawUIConent: tab thrown an exception: %s", ex.what());

                // - the tab is faulty in some way
                // - soak up the exception to prevent the whole application from terminating
                // - then create a new tab containing the error message, so the user can see the error
                // - and indicate that ImGui was aggressively reset, because the drawcall may have thrown midway
                //   through doing stuff in ImGui
                UID id = addTab(std::make_unique<ErrorTab>(getTabHostAPI(), ex));
                selectTab(id);
                implCloseTab(m_ActiveTabID);
                resetImgui();
            }

            handleDeletedTabs();
        }

        if (m_ImguiWasAggressivelyReset)
        {
            return;
        }

        if (m_MaybeSaveChangesPopup)
        {
            m_MaybeSaveChangesPopup->onDraw();
        }
    }

    void drawAddNewTabMenu()
    {
        if (ui::MenuItem(ICON_FA_EDIT " Editor"))
        {
            selectTab(addTab(std::make_unique<ModelEditorTab>(getTabHostAPI(), std::make_unique<UndoableModelStatePair>())));
        }

        if (ui::MenuItem(ICON_FA_CUBE " Mesh Importer"))
        {
            selectTab(addTab(std::make_unique<mi::MeshImporterTab>(getTabHostAPI())));
        }

        std::shared_ptr<TabRegistry const> const tabs = App::singleton<TabRegistry>();
        if (tabs->size() > 0)
        {
            if (ui::BeginMenu("Experimental Tabs"))
            {
                for (size_t i = 0; i < tabs->size(); ++i)
                {
                    TabRegistryEntry e = (*tabs)[i];
                    if (ui::MenuItem(e.getName()))
                    {
                        selectTab(addTab(e.createTab(ParentPtr<ITabHost>{getTabHostAPI()})));
                    }
                }
                ui::EndMenu();
            }
        }
    }

    ITab* getTabByID(UID id)
    {
        auto it = std::find_if(m_Tabs.begin(), m_Tabs.end(), [id](auto const& p)
        {
            return p->getID() == id;
        });

        return it != m_Tabs.end() ? it->get() : nullptr;
    }

    ITab* getActiveTab()
    {
        return getTabByID(m_ActiveTabID);
    }

    ITab* getRequestedTab()
    {
        return getTabByID(m_RequestedTab);
    }

    UID implAddTab(std::unique_ptr<ITab> tab) final
    {
        return m_Tabs.emplace_back(std::move(tab))->getID();
    }

    void implSelectTab(UID id) final
    {
        m_RequestedTab = id;
    }

    void implCloseTab(UID id) final
    {
        m_DeletedTabs.insert(id);
    }

    // called by the "save changes?" popup when user opts to save changes
    bool onUserSelectedSaveChangesInSavePrompt()
    {
        bool savingFailedSomewhere = false;
        for (UID id : m_DeletedTabs)
        {
            if (ITab* tab = getTabByID(id); tab && tab->isUnsaved())
            {
                savingFailedSomewhere = !tab->trySave() || savingFailedSomewhere;
            }
        }

        if (!savingFailedSomewhere)
        {
            nukeDeletedTabs();
            if (m_QuitRequested)
            {
                App::upd().requestQuit();
            }
            return true;
        }
        else
        {
            return false;
        }
    }

    // called by the "save changes?" popup when user opts to not save changes
    bool onUserSelectedDoNotSaveChangesInSavePrompt()
    {
        nukeDeletedTabs();
        if (m_QuitRequested)
        {
            App::upd().requestQuit();
        }
        return true;
    }

    // called by the "save changes?" popup when user clicks "cancel"
    bool onUserCancelledOutOfSavePrompt()
    {
        m_DeletedTabs.clear();
        m_QuitRequested = false;
        return true;
    }

    void nukeDeletedTabs()
    {
        int lowestDeletedTab = std::numeric_limits<int>::max();
        for (UID id : m_DeletedTabs)
        {
            auto it = std::find_if(m_Tabs.begin(), m_Tabs.end(), [id](auto const& o) { return o->getID() == id; });

            if (it != m_Tabs.end())
            {
                if (id == m_ActiveTabID)
                {
                    (*it)->onUnmount();
                    m_ActiveTabID = UID::empty();
                    lowestDeletedTab = std::min(lowestDeletedTab, static_cast<int>(std::distance(m_Tabs.begin(), it)));
                }
                m_Tabs.erase(it);
            }
        }
        m_DeletedTabs.clear();

        // coerce active tab, if it has become stale due to a deletion
        if (!getRequestedTab() && !getActiveTab() && !m_Tabs.empty())
        {
            // focus the tab just to the left of the closed one
            if (1 <= lowestDeletedTab && lowestDeletedTab <= static_cast<int>(m_Tabs.size()))
            {
                m_RequestedTab = m_Tabs[lowestDeletedTab - 1]->getID();
            }
            else
            {
                m_RequestedTab = m_Tabs.front()->getID();
            }
        }
    }

    void handleDeletedTabs()
    {
        // tabs aren't immediately deleted, because they may hold onto unsaved changes
        //
        // this top-level screen has to handle the unsaved changes. This is because it would be
        // annoying, from a UX PoV, to have each tab individually prompt the user. It is preferable
        // to have all the "do you want to save changes?" things in one prompt


        // if any of the to-be-deleted tabs have unsaved changes, then open a save changes dialog
        // that prompts the user to decide on how to handle it
        //
        // don't delete the tabs yet, because the user can always cancel out of the operation

        std::vector<ITab*> tabsWithUnsavedChanges;

        for (UID id : m_DeletedTabs)
        {
            if (ITab* t = getTabByID(id))
            {
                if (t->isUnsaved())
                {
                    tabsWithUnsavedChanges.push_back(t);
                }
            }
        }

        if (!tabsWithUnsavedChanges.empty())
        {
            std::stringstream ss;
            if (tabsWithUnsavedChanges.size() > 1)
            {
                ss << tabsWithUnsavedChanges.size() << " tabs have unsaved changes:\n";
            }
            else
            {
                ss << "A tab has unsaved changes:\n";
            }

            for (ITab* t : tabsWithUnsavedChanges)
            {
                ss << "\n  - " << t->getName();
            }
            ss << "\n\n";

            // open the popup
            SaveChangesPopupConfig cfg
            {
                "Save Changes?",
                [this]() { return onUserSelectedSaveChangesInSavePrompt(); },
                [this]() { return onUserSelectedDoNotSaveChangesInSavePrompt(); },
                [this]() { return onUserCancelledOutOfSavePrompt(); },
                ss.str(),
            };
            m_MaybeSaveChangesPopup.emplace(cfg);
            m_MaybeSaveChangesPopup->open();
        }
        else
        {
            // just nuke all the tabs
            nukeDeletedTabs();
        }
    }

    void implResetImgui() final
    {
        m_ImguiWasAggressivelyReset = true;
    }

    void tryHandleScreenshotRequest()
    {
        if (!m_MaybeScreenshotRequest.valid())
        {
            return;  // probably empty/errored
        }

        if (m_MaybeScreenshotRequest.valid() && m_MaybeScreenshotRequest.wait_for(std::chrono::seconds{0}) == std::future_status::ready)
        {
            UID tabID = addTab(std::make_unique<ScreenshotTab>(updThisAsParent(), m_MaybeScreenshotRequest.get()));
            selectTab(tabID);
        }
    }

    ParentPtr<IMainUIStateAPI> updThisAsParent()
    {
        return ParentPtr<IMainUIStateAPI>{shared_from_this()};
    }

    // set the first time `onMount` is called
    bool m_HasBeenMountedBefore = false;

    // global simulation params: dictates how the next simulation shall be ran
    ParamBlock m_SimulationParams = ToParamBlock(ForwardDynamicSimulatorParams{});

    // user-initiated output extractors
    //
    // simulators should try to hook into these, if the component exists
    std::vector<OutputExtractor> m_UserOutputExtractors;

    // user-visible UI tabs
    std::vector<std::unique_ptr<ITab>> m_Tabs;

    // set of tabs that should be deleted once control returns to this screen
    std::unordered_set<UID> m_DeletedTabs;

    // currently-active UI tab
    UID m_ActiveTabID = UID::empty();

    // cached version of active tab name - used to ensure ImGui can re-focus a renamed tab
    std::string m_ActiveTabNameLastFrame;

    // a tab that should become active next frame
    UID m_RequestedTab = UID::empty();

    // a popup that is shown when a tab, or the whole screen, is requested to close
    //
    // effectively, shows the "do you want to save changes?" popup
    std::optional<SaveChangesPopup> m_MaybeSaveChangesPopup;

    // true if the screen is midway through trying to quit
    bool m_QuitRequested = false;

    // true if the screen should request a redraw from the application
    bool m_ShouldRequestRedraw = false;

    // true if ImGui was aggressively reset by a tab (and, therefore, this screen should reset ImGui)
    bool m_ImguiWasAggressivelyReset = false;

    // `valid` if the user has requested a screenshot (that hasn't yet been handled)
    std::future<Screenshot> m_MaybeScreenshotRequest;
};


// public API

osc::MainUIScreen::MainUIScreen() :
    m_Impl{std::make_shared<Impl>()}
{
}
osc::MainUIScreen::MainUIScreen(MainUIScreen&&) noexcept = default;
osc::MainUIScreen& osc::MainUIScreen::operator=(MainUIScreen&&) noexcept = default;
osc::MainUIScreen::~MainUIScreen() noexcept = default;

UID osc::MainUIScreen::addTab(std::unique_ptr<ITab> tab)
{
    return m_Impl->addTab(std::move(tab));
}

void osc::MainUIScreen::open(std::filesystem::path const& path)
{
    m_Impl->open(path);
}

void osc::MainUIScreen::implOnMount()
{
    m_Impl->onMount();
}

void osc::MainUIScreen::implOnUnmount()
{
    m_Impl->onUnmount();
}

void osc::MainUIScreen::implOnEvent(SDL_Event const& e)
{
    m_Impl->onEvent(e);
}

void osc::MainUIScreen::implOnTick()
{
    m_Impl->onTick();
}

void osc::MainUIScreen::implOnDraw()
{
    m_Impl->onDraw();
}
