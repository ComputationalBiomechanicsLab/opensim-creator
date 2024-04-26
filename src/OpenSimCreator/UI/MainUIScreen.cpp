#include "MainUIScreen.h"

#include <OpenSimCreator/Documents/Model/UndoableModelStatePair.h>
#include <OpenSimCreator/Documents/OutputExtractors/OutputExtractor.h>
#include <OpenSimCreator/Documents/Simulation/ForwardDynamicSimulatorParams.h>
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
#include <oscar/Shims/Cpp23/ranges.h>
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
#include <oscar/Utils/Algorithms.h>
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
#include <ranges>
#include <sstream>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

using namespace osc;
namespace rgs = std::ranges;

namespace
{
    std::unique_ptr<ITab> LoadConfigurationDefinedTabIfNecessary(
        AppConfig const& config,
        TabRegistry const& tabRegistry,
        ParentPtr<ITabHost> const& api)
    {
        if (std::optional<std::string> maybeRequestedTab = config.initial_tab_override())
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

    void on_mount()
    {
        if (!std::exchange(m_HasBeenMountedBefore, true))
        {
            // on first mount, place the splash tab at the front of the tabs collection
            m_Tabs.insert(m_Tabs.begin(), std::make_unique<SplashTab>(getTabHostAPI()));

            // if the application configuration has requested that a specific tab should be opened,
            // then try looking it up and open it
            if (auto tab = LoadConfigurationDefinedTabIfNecessary(App::config(), *App::singleton<TabRegistry>(), getTabHostAPI()))
            {
                addTab(std::move(tab));
            }

            // focus on the rightmost tab
            if (!m_Tabs.empty())
            {
                m_RequestedTab = m_Tabs.back()->getID();
            }
        }

        ui::context::init();
    }

    void on_unmount()
    {
        // unmount the active tab before unmounting this (host) screen
        if (ITab* active = getActiveTab())
        {
            try
            {
                active->on_unmount();
            }
            catch (std::exception const& ex)
            {
                // - the tab is faulty in some way
                // - soak up the exception to prevent the whole application from terminating
                // - and emit the error to the log, because we have to assume that this
                //   screen is about to die (it's being unmounted)
                log_error("MainUIScreen::on_unmount: unmounting active tab threw an exception: %s", ex.what());
            }

            m_ActiveTabID = UID::empty();
        }

        ui::context::shutdown();
    }

    void onEvent(SDL_Event const& e)
    {
        if (e.type == SDL_KEYUP or
            e.type == SDL_MOUSEBUTTONUP or
            e.type == SDL_MOUSEMOTION) {

            // if the user just potentially changed something via a mouse/keyboard
            // interaction then the screen should be aggressively redrawn to reduce
            // and input delays
            m_ShouldRequestRedraw = true;
        }

        if (e.type == SDL_KEYUP &&
            e.key.keysym.mod & (KMOD_CTRL | KMOD_GUI) &&
            e.key.keysym.scancode == SDL_SCANCODE_P)
        {
            // Ctrl+/Super+P operates as a "take a screenshot" request
            m_MaybeScreenshotRequest = App::upd().request_screenshot();
        }
        if ((e.type == SDL_KEYUP &&
             e.key.keysym.mod & (KMOD_CTRL | KMOD_GUI) &&
             e.key.keysym.scancode == SDL_SCANCODE_PAGEUP) ||
            (e.type == SDL_KEYUP &&
             e.key.keysym.mod & (KMOD_GUI) &&
             e.key.keysym.mod & (KMOD_LALT | KMOD_RALT) &&
             e.key.keysym.scancode == SDL_SCANCODE_LEFT))
        {
            // Ctrl+/Super+PageUp (or Command+Option+Left on MacOS) focuses the tab to the left of the currently active tab
            auto it = findTabByID(m_ActiveTabID);
            if (it != m_Tabs.begin() and it != m_Tabs.end()) {
                --it;  // previous
                selectTab((*it)->getID());
            }
        }
        if ((e.type == SDL_KEYUP &&
             e.key.keysym.mod & (KMOD_CTRL | KMOD_GUI) &&
             e.key.keysym.scancode == SDL_SCANCODE_PAGEDOWN) ||
            (e.type == SDL_KEYUP &&
             e.key.keysym.mod & (KMOD_GUI) &&
             e.key.keysym.mod & (KMOD_LALT | KMOD_RALT) &&
             e.key.keysym.scancode == SDL_SCANCODE_RIGHT))
        {
            // Ctrl+/Super+PageDown focuses the tab to the right of the currently active tab
            auto it = findTabByID(m_ActiveTabID);
            if (it != m_Tabs.end()-1) {
                ++it;  // next
                selectTab((*it)->getID());
            }
        }
        else if (ui::context::on_event(e))
        {
            // event was pumped into the UI context - it shouldn't be pumped into the active tab
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
                    log_error("MainUIScreen::on_event: exception thrown by tab: %s", ex.what());

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

                App::upd().request_quit();
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
                log_error("MainUIScreen::on_event: exception thrown by tab: %s", ex.what());

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

    void on_tick()
    {
        // tick all the tabs, because they may internally be polling something (e.g.
        // updating something as a simulation runs)
        for (size_t i = 0; i < m_Tabs.size(); ++i)
        {
            try
            {
                m_Tabs[i]->on_tick();
            }
            catch (std::exception const& ex)
            {

                log_error("MainUIScreen::on_tick: tab thrown an exception: %s", ex.what());

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
            OSC_PERF("MainUIScreen/clear_screen");
            App::upd().clear_screen({0.0f, 0.0f, 0.0f, 0.0f});
        }

        ui::context::on_start_new_frame();

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

            ui::context::shutdown();
            ui::context::init();
            App::upd().request_redraw();
            m_ImguiWasAggressivelyReset = false;

            return;
        }

        {
            OSC_PERF("MainUIScreen/ui::context::render()");
            ui::context::render();
        }

        if (m_ShouldRequestRedraw)
        {
            App::upd().request_redraw();
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
        App::upd().upd_config().set_panel_enabled("Output Watches", true);
    }

    void implRemoveUserOutputExtractor(int idx) final
    {
        OSC_ASSERT(0 <= idx && idx < static_cast<int>(m_UserOutputExtractors.size()));
        m_UserOutputExtractors.erase(m_UserOutputExtractors.begin() + idx);
    }

    bool implHasUserOutputExtractor(OutputExtractor const& oe) const final
    {
        return cpp23::contains(m_UserOutputExtractors, oe);
    }

    bool implRemoveUserOutputExtractor(OutputExtractor const& oe) final
    {
        return std::erase(m_UserOutputExtractors, oe) > 0;
    }

    bool implOverwriteOrAddNewUserOutputExtractor(OutputExtractor const& old, OutputExtractor const& newer) final
    {
        if (auto it = find_or_nullptr(m_UserOutputExtractors, old)) {
            *it = newer;
            return true;
        }
        else {
            m_UserOutputExtractors.push_back(newer);
            return true;
        }
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

        ui::PushStyleVar(ImGuiStyleVar_FramePadding, ui::GetStyleFramePadding() + 2.0f);
        ui::PushStyleVar(ImGuiStyleVar_ItemInnerSpacing, Vec2{5.0f, 0.0f});
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
                                    activeTab->on_unmount();
                                }
                                m_Tabs[i]->on_mount();
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
                // - and indicate that the UI was aggressively reset, because the drawcall may have thrown midway
                //   through rendering the 2D UI
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

    std::vector<std::unique_ptr<ITab>>::iterator findTabByID(UID id)
    {
        return rgs::find(m_Tabs, id, [](auto const& ptr) { return ptr->getID(); });
    }

    ITab* getTabByID(UID id)
    {
        auto it = findTabByID(id);
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
                App::upd().request_quit();
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
            App::upd().request_quit();
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
            auto it = rgs::find(m_Tabs, id, [](auto const& ptr) { return ptr->getID(); });
            if (it != m_Tabs.end())
            {
                if (id == m_ActiveTabID)
                {
                    (*it)->on_unmount();
                    m_ActiveTabID = UID::empty();
                    lowestDeletedTab = min(lowestDeletedTab, static_cast<int>(std::distance(m_Tabs.begin(), it)));
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

    // cached version of active tab name - used to ensure the UI can re-focus a renamed tab
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

    // true if the UI context was aggressively reset by a tab (and, therefore, this screen should reset the UI)
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

void osc::MainUIScreen::impl_on_mount()
{
    m_Impl->on_mount();
}

void osc::MainUIScreen::impl_on_unmount()
{
    m_Impl->on_unmount();
}

void osc::MainUIScreen::impl_on_event(SDL_Event const& e)
{
    m_Impl->onEvent(e);
}

void osc::MainUIScreen::impl_on_tick()
{
    m_Impl->on_tick();
}

void osc::MainUIScreen::impl_on_draw()
{
    m_Impl->onDraw();
}
