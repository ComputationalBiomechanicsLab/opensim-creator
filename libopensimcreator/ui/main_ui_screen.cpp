#include "main_ui_screen.h"

#include <libopensimcreator/platform/msmicons.h>
#include <libopensimcreator/ui/events/open_file_event.h>
#include <libopensimcreator/ui/loading_tab.h>
#include <libopensimcreator/ui/mesh_importer/mesh_importer_tab.h>
#include <libopensimcreator/ui/model_editor/model_editor_tab.h>
#include <libopensimcreator/ui/splash_tab.h>

#include <liboscar/platform/app.h>
#include <liboscar/platform/app_settings.h>
#include <liboscar/platform/events/drop_file_event.h>
#include <liboscar/platform/events/event.h>
#include <liboscar/platform/events/key_event.h>
#include <liboscar/platform/log.h>
#include <liboscar/platform/os.h>
#include <liboscar/platform/screenshot.h>
#include <liboscar/platform/widget_private.h>
#include <liboscar/ui/events/close_tab_event.h>
#include <liboscar/ui/events/open_tab_event.h>
#include <liboscar/ui/events/reset_ui_context_event.h>
#include <liboscar/ui/oscimgui.h>
#include <liboscar/ui/popups/save_changes_popup.h>
#include <liboscar/ui/popups/save_changes_popup_config.h>
#include <liboscar/ui/tabs/error_tab.h>
#include <liboscar/ui/tabs/screenshot_tab.h>
#include <liboscar/ui/tabs/tab.h>
#include <liboscar/ui/tabs/tab_registry.h>
#include <liboscar/utilities/algorithms.h>
#include <liboscar/utilities/conversion.h>
#include <liboscar/utilities/c_string_view.h>
#include <liboscar/utilities/perf.h>
#include <liboscar/utilities/uid.h>

#include <algorithm>
#include <chrono>
#include <functional>
#include <iterator>
#include <limits>
#include <memory>
#include <optional>
#include <ranges>
#include <set>
#include <span>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

using namespace osc;
using namespace std::literals::chrono_literals;
namespace rgs = std::ranges;

namespace
{
    std::unique_ptr<Tab> LoadConfigurationDefinedTabIfNecessary(
        const AppSettings& settings,
        const TabRegistry& tabRegistry,
        Widget& parent)
    {
        if (auto maybeRequestedTab = settings.find_value("initial_tab")) {
            if (std::optional<TabRegistryEntry> maybeEntry = tabRegistry.find_by_name(to<std::string>(*maybeRequestedTab))) {
                return maybeEntry->construct_tab(&parent);
            }

            log_warn("%s: cannot find a tab with this name in the tab registry: ignoring", to<std::string>(*maybeRequestedTab).c_str());
            log_warn("available tabsWithUnsavedChanges are:");
            for (auto&& tabRegistryEntry : tabRegistry) {
                log_warn("    %s", tabRegistryEntry.name().c_str());
            }
        }

        return nullptr;
    }

    std::string MakeSavePromptString(std::span<Tab*> tabsWithUnsavedChanges)
    {
        std::stringstream ss;
        if (tabsWithUnsavedChanges.size() > 1) {
            ss << tabsWithUnsavedChanges.size() << " tabsWithUnsavedChanges have unsaved changes:\n";
        }
        else {
            ss << "A tab has unsaved changes:\n";
        }

        for (Tab* t : tabsWithUnsavedChanges) {
            ss << "\n  - " << t->name();
        }
        ss << "\n\n";

        return std::move(ss).str();
    }

    std::vector<UID> ExtractToReversedVectorOfUIDs(std::span<Tab*> tabs)
    {
        std::vector<UID> rv;
        rv.reserve(tabs.size());  // upper limit
        for (auto it = tabs.rbegin(); it != tabs.rend(); ++it) {
            rv.push_back((*it)->id());
        }
        return rv;
    }
}

class osc::MainUIScreen::Impl final : public WidgetPrivate {
public:

    explicit Impl(MainUIScreen& owner) :
        WidgetPrivate{owner, nullptr}
    {
        set_name("MainUIScreen");
    }

    bool onUnhandledKeyUp(const KeyEvent& e)
    {
        if (e.combination() == (KeyModifier::Ctrl | Key::PageUp) or
            e.combination() == (KeyModifier::Ctrl | KeyModifier::Alt | Key::LeftArrow)) {
            // `Ctrl+PageUp` or `Ctrl+Alt+Left`: focus the tab to the left of the currently-active tab
            auto it = findTabByID(m_ActiveTabID);
            if (it != m_Tabs.begin() and it != m_Tabs.end()) {
                --it;  // previous
                impl_select_tab((*it)->id());
            }
            return true;
        }
        if (e.combination() == (KeyModifier::Ctrl | Key::PageDown) or
            e.combination() == (KeyModifier::Ctrl | KeyModifier::Alt | Key::RightArrow)) {
            // `Ctrl+PageDown` or `Ctrl+Alt+Right`: focus the tab to the right of the currently-active tab
            auto it = findTabByID(m_ActiveTabID);
            if (it != m_Tabs.end()-1) {
                ++it;  // next
                impl_select_tab((*it)->id());
            }
            return true;
        }
        if (e.combination() == (KeyModifier::Ctrl | Key::W) and m_Tabs.size() > 1 and m_ActiveTabID != m_Tabs.front()->id()) {
            // `Ctrl+W`: close the current tab - unless it's the splash tab
            impl_close_tab(m_ActiveTabID);
            return true;
        }
        return false;
    }

    // called when an event is pumped into this screen but isn't handled by
    // either the global 2D UI context or the active tab
    bool onUnhandledEvent(Event& e)
    {
        if (e.type() == EventType::KeyUp) {
            return onUnhandledKeyUp(dynamic_cast<const KeyEvent&>(e));
        }
        return false;
    }

    UID addTab(std::unique_ptr<Tab> tab)
    {
        return impl_add_tab(std::move(tab));
    }

    void open(const std::filesystem::path& p)
    {
        // Defer opening the file until the main event loop is set up
        // otherwise, the resulting `LoadingTab`, `ModelEditorTab` etc.
        // might be initialized before anything else (e.g. before the
        // ui context).
        App::post_event<OpenFileEvent>(owner(), p);
    }

    void on_mount()
    {
        if (not std::exchange(m_HasBeenMountedBefore, true)) {
            // on first mount, place the splash tab at the front of the tabs collection
            addTab(std::make_unique<SplashTab>(&owner()));

            // if the application configuration has requested that a specific tab should be opened,
            // then try looking it up and open it
            if (auto tab = LoadConfigurationDefinedTabIfNecessary(App::settings(), *App::singleton<TabRegistry>(), owner())) {
                addTab(std::move(tab));
            }

            // focus on the rightmost tab
            if (not m_Tabs.empty()) {
                m_RequestedTab = m_Tabs.back()->id();
            }
        }
    }

    void on_unmount()
    {
        // unmount the active tab before unmounting this (host) screen
        if (Tab* active = getActiveTab()) {
            try {
                active->on_unmount();
            }
            catch (const std::exception& ex) {
                // - the tab is faulty in some way
                // - soak up the exception to prevent the whole application from terminating
                // - and emit the error to the log, because we have to assume that this
                //   screen is about to die (it's being unmounted)
                log_error("MainUIScreen::on_unmount: unmounting active tab threw an exception: %s", ex.what());
            }

            m_ActiveTabID = UID::empty();
        }
    }

    bool on_event(Event& e)
    {
        if (e.type() == EventType::KeyDown or
            e.type() == EventType::KeyUp or
            e.type() == EventType::MouseButtonUp or
            e.type() == EventType::MouseMove or
            e.type() == EventType::MouseWheel) {

            // if the user just potentially changed something via a mouse/keyboard
            // interaction then the screen should be aggressively redrawn to reduce
            // and input delays

            App::upd().request_redraw();
        }

        bool handled = false;

        if (e.type() == EventType::KeyUp and dynamic_cast<const KeyEvent&>(e).combination() == (KeyModifier::Ctrl | Key::P)) {
            // `Ctrl+P`: "take a screenshot"
            m_MaybeScreenshotRequest = App::upd().request_screenshot_of_main_window();
            handled = true;
        }
        else if (m_UiContext.on_event(e)) {
            // if the 2D UI captured the event, then assume that the event will be "handled"
            // during `Tab::onDraw` (immediate-mode UI)
            App::upd().request_redraw();
            handled = true;
        }
        else if (e.type() == EventType::Quit) {
            // if it's an application-level QUIT request, then it should be pumped into each
            // tab, while checking whether a tab wants to "block" the request (e.g. because it
            // wants to show a "do you want to save changes?" popup to the user

            bool atLeastOneTabHandledQuit = false;
            for (int i = 0; i < static_cast<int>(m_Tabs.size()); ++i) {
                try {
                    atLeastOneTabHandledQuit = m_Tabs[i]->on_event(e) || atLeastOneTabHandledQuit;
                }
                catch (const std::exception& ex) {
                    log_error("MainUIScreen::on_event: exception thrown by tab: %s", ex.what());

                    // - the tab is faulty in some way
                    // - soak up the exception to prevent the whole application from terminating
                    // - then create a new tab containing the error message, so the user can see the error
                    const UID id = addTab(std::make_unique<ErrorTab>(owner(), ex));
                    impl_select_tab(id);
                    impl_close_tab(m_Tabs[i]->id());
                }
            }

            if (not atLeastOneTabHandledQuit) {
                // if no tab handled the quit event, treat it as-if the user
                // has tried to close all tabs

                for (const auto& tab : m_Tabs) {
                    impl_close_tab(tab->id());
                }
                m_QuitRequested = true;
            }

            // handle any deletion-related side-effects (e.g. showing save prompt)
            handleDeletedTabs();

            if (not atLeastOneTabHandledQuit and not m_MaybeInProgressSaveDialog) {
                // - if no tab handled a quit event
                // - and the UI isn't currently showing a save prompt
                // - then it's safe to outright quit the application from this screen
                App::upd().request_quit();
            }

            handled = true;
        }
        else if (auto* addTabEv = dynamic_cast<OpenTabEvent*>(&e)) {
            if (addTabEv->has_tab()) {
                impl_select_tab(impl_add_tab(addTabEv->take_tab()));
                handled = true;
            }
        }
        else if (auto* closeTabEv = dynamic_cast<CloseTabEvent*>(&e)) {
            impl_close_tab(closeTabEv->tabid_to_close());
            handled = true;
        }
        else if (auto* openFileEv = dynamic_cast<OpenFileEvent*>(&e)) {
            impl_select_tab(impl_add_tab(std::make_unique<LoadingTab>(&owner(), openFileEv->path())));
            handled = true;
        }
        else if (dynamic_cast<ResetUIContextEvent*>(&e)) {
            impl_reset_imgui();
        }
        else if (Tab* active = getActiveTab()) {
            // if there's an active tab, pump the event into the active tab and check
            // whether the tab handled the event

            bool activeTabHandledEvent = false;
            try {
                activeTabHandledEvent = active->on_event(e);
            }
            catch (const std::exception& ex) {
                log_error("MainUIScreen::on_event: exception thrown by tab: %s", ex.what());

                // - the tab is faulty in some way
                // - soak up the exception to prevent the whole application from terminating
                // - then create a new tab containing the error message, so the user can see the error
                UID id = addTab(std::make_unique<ErrorTab>(owner(), ex));
                impl_select_tab(id);
                impl_close_tab(m_ActiveTabID);
            }

            // the event may have triggered tab deletions
            handleDeletedTabs();

            if (activeTabHandledEvent) {
                // If the user dragged a file into an open tab, and the tab accepted the
                // event (e.g. because it opened/imported the file), then the directory
                // of the dropped file should become the next directory that the user sees
                // if they subsequently open a file dialog.
                //
                // The reason that users find this useful is because they might've just
                // dragged a file into the UI to open something and, subsequently, want
                // to load associated data (#918).
                if (auto* dropEv = dynamic_cast<DropFileEvent*>(&e)) {
                    const auto parentDirectory = dropEv->path().parent_path();
                    if (not parentDirectory.empty()) {
                        App::upd().set_prompt_initial_directory_to_show_fallback(parentDirectory);
                    }
                }

                App::upd().request_redraw();
                handled = true;
            }
        }

        return handled or onUnhandledEvent(e);
    }

    void on_tick()
    {
        // tick all the tabs, because they may internally be polling something (e.g.
        // updating something as a simulation runs)
        for (size_t i = 0; i < m_Tabs.size(); ++i) {
            try {
                m_Tabs[i]->on_tick();
            }
            catch (const std::exception& ex) {

                log_error("MainUIScreen::on_tick: tab thrown an exception: %s", ex.what());

                // - the tab is faulty in some way
                // - soak up the exception to prevent the whole application from terminating
                // - then create a new tab containing the error message, so the user can see the error
                const UID id = addTab(std::make_unique<ErrorTab>(owner(), ex));
                impl_select_tab(id);
                impl_close_tab(m_Tabs[i]->id());
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
            App::upd().clear_main_window();
        }

        m_UiContext.on_start_new_frame();

        {
            OSC_PERF("MainUIScreen/drawUIContent");
            drawUIContent();
        }

        if (m_UiWasAggressivelyReset) {
            if (m_RequestedTab == UID::empty()) {
                m_RequestedTab = m_ActiveTabID;
            }
            m_ActiveTabID = UID::empty();

            m_UiContext.reset();
            App::upd().request_redraw();
            m_UiWasAggressivelyReset = false;

            return;
        }

        {
            OSC_PERF("MainUIScreen/render()");
            m_UiContext.render();
        }
    }

    void drawTabSpecificMenu()
    {
        OSC_PERF("MainUIScreen/drawTabSpecificMenu");

        if (ui::begin_main_window_top_bar("##TabSpecificMenuBar")) {
            if (ui::begin_menu_bar()) {
                if (Tab* active = getActiveTab()) {
                    try {
                        active->on_draw_main_menu();
                    }
                    catch (const std::exception& ex) {
                        log_error("MainUIScreen::drawTabSpecificMenu: tab thrown an exception: %s", ex.what());

                        // - the tab is faulty in some way
                        // - soak up the exception to prevent the whole application from terminating
                        // - then create a new tab containing the error message, so the user can see the error
                        const UID id = addTab(std::make_unique<ErrorTab>(owner(), ex));
                        impl_select_tab(id);
                        impl_close_tab(m_ActiveTabID);
                    }

                    if (m_UiWasAggressivelyReset) {
                        return;  // must return here to prevent the ImGui end_panel calls from erroring
                    }
                }
                ui::end_menu_bar();
            }
            ui::end_panel();
            handleDeletedTabs();
        }
    }

    void drawTabBar()
    {
        OSC_PERF("MainUIScreen/drawTabBar");

        ui::push_style_var(ui::StyleVar::FramePadding, ui::get_style_frame_padding() + 2.0f);
        ui::push_style_var(ui::StyleVar::ItemInnerSpacing, Vector2{5.0f, 0.0f});
        ui::push_style_var(ui::StyleVar::TabRounding, 10.0f);
        ui::push_style_var(ui::StyleVar::FrameRounding, 10.0f);
        if (ui::begin_main_window_top_bar("##MainWindowTabBarWrapper")) {
            if (ui::begin_menu_bar()) {
                if (ui::begin_tab_bar("##TabBar")) {
                    for (size_t i = 0; i < m_Tabs.size(); ++i) {
                        ui::TabItemFlags flags = ui::TabItemFlag::NoReorder;

                        if (i == 0) {
                            flags |= ui::TabItemFlag::NoCloseButton;  // splash screen
                        }

                        if (m_Tabs[i]->is_unsaved()) {
                            flags |= ui::TabItemFlag::UnsavedDocument;
                        }

                        if (m_Tabs[i]->id() == m_RequestedTab) {
                            flags |= ui::TabItemFlag::SetSelected;;
                        }

                        if (m_Tabs[i]->id() == m_ActiveTabID and m_Tabs[i]->name() != m_ActiveTabNameLastFrame) {
                            flags |= ui::TabItemFlag::SetSelected;
                            m_ActiveTabNameLastFrame = m_Tabs[i]->name();
                        }

                        ui::push_id(m_Tabs[i].get());
                        bool active = true;

                        if (ui::begin_tab_item(m_Tabs[i]->name(), &active, flags)) {
                            if (m_Tabs[i]->id() != m_ActiveTabID) {
                                if (Tab* activeTab = getActiveTab()) {
                                    activeTab->on_unmount();
                                }
                                m_Tabs[i]->on_mount();
                            }

                            m_ActiveTabID = m_Tabs[i]->id();
                            m_ActiveTabNameLastFrame = m_Tabs[i]->name();

                            if (m_RequestedTab == m_ActiveTabID) {
                                m_RequestedTab = UID::empty();
                            }

                            if (m_UiWasAggressivelyReset) {
                                return;
                            }

                            ui::end_tab_item();
                        }

                        ui::pop_id();
                        if (not active and i != 0)  {  // can't close the splash tab
                            impl_close_tab(m_Tabs[i]->id());
                        }
                    }

                    // adding buttons to tab bars: https://github.com/ocornut/imgui/issues/3291
                    ui::draw_tab_item_button(MSMICONS_PLUS);

                    if (ui::begin_popup_context_menu("popup", ui::PopupFlag::MouseButtonLeft)) {
                        drawAddNewTabMenu();
                        ui::end_popup();
                    }

                    ui::end_tab_bar();
                }
                ui::end_menu_bar();
            }

            ui::end_panel();
            handleDeletedTabs();
        }
        ui::pop_style_var(4);
    }

    void drawUIContent()
    {
        drawTabSpecificMenu();

        if (m_UiWasAggressivelyReset) {
            return;
        }

        drawTabBar();

        if (m_UiWasAggressivelyReset) {
            return;
        }

        // draw the active tab (if any)
        if (Tab* active = getActiveTab()) {
            try {
                OSC_PERF("MainUIScreen/drawActiveTab");
                active->on_draw();
            }
            catch (const std::exception& ex) {
                log_error("MainUIScreen::drawUIConent: tab thrown an exception: %s", ex.what());

                // - the tab is faulty in some way
                // - soak up the exception to prevent the whole application from terminating
                // - then create a new tab containing the error message, so the user can see the error
                // - and indicate that the UI was aggressively reset, because the drawcall may have thrown midway
                //   through rendering the 2D UI
                const UID id = addTab(std::make_unique<ErrorTab>(owner(), ex));
                impl_select_tab(id);
                impl_close_tab(m_ActiveTabID);
                impl_reset_imgui();
            }

            handleDeletedTabs();
        }

        if (m_UiWasAggressivelyReset) {
            return;
        }

        if (m_MaybeInProgressSaveDialog) {
            m_MaybeInProgressSaveDialog->on_draw();
            if (m_MaybeInProgressSaveDialog->is_closed()) {
                m_MaybeInProgressSaveDialog.reset();
            }
        }
    }

    void drawAddNewTabMenu()
    {
        if (ui::draw_menu_item(MSMICONS_EDIT " Editor")) {
            impl_select_tab(addTab(std::make_unique<ModelEditorTab>(&owner())));
        }

        if (ui::draw_menu_item(MSMICONS_CUBE " Mesh Importer")) {
            impl_select_tab(addTab(std::make_unique<MeshImporterTab>(&owner())));
        }

        const std::shared_ptr<const TabRegistry> tabRegistry = App::singleton<TabRegistry>();
        if (not tabRegistry->empty()) {
            if (ui::begin_menu("Experimental Tabs")) {
                for (auto&& tabRegistryEntry : *tabRegistry) {
                    if (ui::draw_menu_item(tabRegistryEntry.name())) {
                        impl_select_tab(addTab(tabRegistryEntry.construct_tab(&owner())));
                    }
                }
                ui::end_menu();
            }
        }
    }

    std::vector<std::unique_ptr<Tab>>::iterator findTabByID(UID id)
    {
        return rgs::find(m_Tabs, id, [](const auto& ptr) { return ptr->id(); });
    }

    Tab* getTabByID(UID id)
    {
        auto it = findTabByID(id);
        return it != m_Tabs.end() ? it->get() : nullptr;
    }

    Tab* getActiveTab()
    {
        return getTabByID(m_ActiveTabID);
    }

    Tab* getRequestedTab()
    {
        return getTabByID(m_RequestedTab);
    }

    UID impl_add_tab(std::unique_ptr<Tab> tab)
    {
        tab->set_parent(&this->owner());
        return m_Tabs.emplace_back(std::move(tab))->id();
    }

    void impl_select_tab(UID id)
    {
        m_RequestedTab = id;
    }

    void impl_close_tab(UID id)
    {
        m_DeletedTabs.insert(id);
    }

    void nukeDeletedTabs()
    {
        int lowestDeletedTab = std::numeric_limits<int>::max();
        for (UID id : m_DeletedTabs) {
            auto it = rgs::find(m_Tabs, id, [](const auto& ptr) { return ptr->id(); });
            if (it != m_Tabs.end()) {
                if (id == m_ActiveTabID) {
                    (*it)->on_unmount();
                    m_ActiveTabID = UID::empty();
                    lowestDeletedTab = min(lowestDeletedTab, static_cast<int>(std::distance(m_Tabs.begin(), it)));
                }
                m_Tabs.erase(it);
            }
        }
        m_DeletedTabs.clear();

        // coerce active tab, if it has become stale due to a deletion
        if (not getRequestedTab() and not getActiveTab() and not m_Tabs.empty()) {
            // focus the tab just to the left of the closed one
            if (1 <= lowestDeletedTab and lowestDeletedTab <= static_cast<int>(m_Tabs.size())) {
                m_RequestedTab = m_Tabs[lowestDeletedTab - 1]->id();
            }
            else {
                m_RequestedTab = m_Tabs.front()->id();
            }
        }
    }

    std::vector<Tab*> collectDeletedTabsWithUnsavedChanges()
    {
        std::vector<Tab*> rv;
        rv.reserve(m_DeletedTabs.size());  // upper limit

        for (UID id : m_DeletedTabs) {
            if (Tab* t = getTabByID(id)) {
                if (t->is_unsaved()) {
                    rv.push_back(t);
                }
            }
        }
        return rv;
    }

    void handleDeletedTabs()
    {
        // tabs aren't immediately deleted, because they may hold onto unsaved changes
        //
        // this top-level screen has to handle the unsaved changes. This is because it would be
        // annoying, from a UX PoV, to have each tab individually prompt the user. It is preferable
        // to have all the "do you want to save changes?" things in one prompt

        if (m_MaybeInProgressSaveDialog) {
            return;  // nothing to process right now: waiting on user decision
        }
        else if (auto tabs = collectDeletedTabsWithUnsavedChanges(); not tabs.empty()) {
            m_MaybeInProgressSaveDialog.emplace(*this, tabs);
            return;  // wait for the user to handle unsaved changes
        }
        else {
            // all changes saved etc. - nuke all the tabs
            nukeDeletedTabs();
        }
    }

    void impl_reset_imgui()
    {
        m_UiWasAggressivelyReset = true;
    }

    void tryHandleScreenshotRequest()
    {
        if (!m_MaybeScreenshotRequest.valid()) {
            return;  // probably empty/errored
        }

        if (m_MaybeScreenshotRequest.valid() and m_MaybeScreenshotRequest.wait_for(0s) == std::future_status::ready) {
            const UID tabID = addTab(std::make_unique<ScreenshotTab>(&owner(), m_MaybeScreenshotRequest.get()));
            impl_select_tab(tabID);
        }
    }

private:
    OSC_OWNER_GETTERS(MainUIScreen);

    // Creates the top-level 2D UI context configuration (fonts, etc.).
    static ui::ContextConfiguration CreateUiContextConfig()
    {
        ui::ContextConfiguration rv;
        rv.set_base_imgui_ini_config_resource("OpenSimCreator/imgui_base_config.ini");
        rv.set_main_font_as_standard_plus_icon_font(
            "OpenSimCreator/fonts/Ruda-Bold.ttf",
            "OpenSimCreator/fonts/msmicons.ttf",
            {MSMICONS_MIN, MSMICONS_MAX}
        );
        return rv;
    }

    // top-level 2D UI context (required for `ui::` calls to work).
    ui::Context m_UiContext{App::upd(), CreateUiContextConfig()};

    // user-visible UI tabs
    std::vector<std::unique_ptr<Tab>> m_Tabs;

    // set of tabs that should be deleted once control returns to this screen
    std::set<UID> m_DeletedTabs;

    // represents the current state of the save dialog (incl. any user-async prompts)
    class InProgressSaveDialog final {
    public:
        explicit InProgressSaveDialog(Impl& impl, std::span<Tab*> tabsWithUnsavedChanges) :
            m_Parent{&impl},
            m_Popup{
                &impl.owner(),
                SaveChangesPopupConfig{
                    "Save Changes?",
                    [this]{ return onUserSelectedSaveChanges(); },
                    [this]{ return onUserSelectedDoNotSaveChanges(); },
                    [this]{ return onUserCancelledOutOfSavePrompt(); },
                    MakeSavePromptString(tabsWithUnsavedChanges)
                }
            },
            m_AsyncPromptQueue{ExtractToReversedVectorOfUIDs(tabsWithUnsavedChanges)}
        {
            m_Popup.open();
        }

        void on_draw()
        {
            if (m_Popup.begin_popup()) {
                m_Popup.on_draw();
                m_Popup.end_popup();
            }

            // Handle async requests
            if (m_Popup.is_open() and m_UserWantsToStartSavingFiles) {
                // 1) Poll+handle any in-progress requests
                if (m_MaybeActiveAsyncSave) {
                    if (const auto result = m_MaybeActiveAsyncSave->try_pop_result()) {
                        switch (*result) {
                        case TabSaveResult::Done:
                            m_AsyncPromptQueue.pop_back();
                            break;
                        case TabSaveResult::Cancelled:
                            m_UserWantsToStartSavingFiles = false;
                            break;
                        }
                        m_MaybeActiveAsyncSave.reset();
                    }
                }

                // 2) launch next request, if we're not currently handling one and there's one waiting
                if (m_UserWantsToStartSavingFiles and not m_MaybeActiveAsyncSave and not m_AsyncPromptQueue.empty()) {
                    if (Tab* nextTab = m_Parent->getTabByID(m_AsyncPromptQueue.back())) {
                        m_MaybeActiveAsyncSave.emplace(*nextTab);
                    }
                    else {
                        m_AsyncPromptQueue.pop_back();  // Can't find tab?
                    }
                }

                // 3) If the queue is empty, transition to the next state (i.e. close the popup)
                if (m_AsyncPromptQueue.empty()) {
                    m_Parent->nukeDeletedTabs();
                    if (m_Parent->m_QuitRequested) {
                        App::upd().request_quit();
                    }
                    m_Popup.close();
                }
            }
        }

        bool is_closed()
        {
            return not m_Popup.is_open();
        }
    private:
        // called by the "save changes?" popup when user opts to save changes
        bool onUserSelectedSaveChanges()
        {
            m_UserWantsToStartSavingFiles = true;
            return false;  // The state transition happens during draw
        }

        // called by the "save changes?" popup when user opts to not save changes
        bool onUserSelectedDoNotSaveChanges()
        {
            m_AsyncPromptQueue.clear();
            m_MaybeActiveAsyncSave.reset();
            m_Parent->nukeDeletedTabs();
            if (m_Parent->m_QuitRequested) {
                App::upd().request_quit();
            }
            return true;
        }

        // called by the "save changes?" popup when user clicks "cancel"
        bool onUserCancelledOutOfSavePrompt()
        {
            m_AsyncPromptQueue.clear();
            m_MaybeActiveAsyncSave.reset();
            m_Parent->m_DeletedTabs.clear();
            m_Parent->m_QuitRequested = false;
            return true;
        }

        Impl* m_Parent = nullptr;
        SaveChangesPopup m_Popup;
        bool m_UserWantsToStartSavingFiles = false;
        std::vector<UID> m_AsyncPromptQueue;
        class ActiveAsyncSaveRequest final {
        public:
            explicit ActiveAsyncSaveRequest(Tab& tab) :
                response{tab.try_save()}
            {}

            std::optional<TabSaveResult> try_pop_result()
            {
                if (not response.valid()) {
                    return TabSaveResult::Cancelled;
                }
                if (response.wait_for(0s) != std::future_status::ready) {
                    return std::nullopt;
                }
                try {
                    return response.get();
                }
                catch (const std::future_error&) {
                    return TabSaveResult::Cancelled;
                }
            }
        private:
            std::future<TabSaveResult> response;
        };
        std::optional<ActiveAsyncSaveRequest> m_MaybeActiveAsyncSave;
    };
    std::optional<InProgressSaveDialog> m_MaybeInProgressSaveDialog;

    // currently-active UI tab
    UID m_ActiveTabID = UID::empty();

    // cached version of active tab name - used to ensure the UI can re-focus a renamed tab
    std::string m_ActiveTabNameLastFrame;

    // a tab that should become active next frame
    UID m_RequestedTab = UID::empty();

    // `true` if `on_mount` has been called on this.
    bool m_HasBeenMountedBefore = false;

    // `true` if the this is midway through trying to quit
    bool m_QuitRequested = false;

    // true if the UI context was aggressively reset by a tab (and, therefore, this screen should reset the UI)
    bool m_UiWasAggressivelyReset = false;

    // `valid` if the user has requested a screenshot (that hasn't yet been handled)
    std::future<Screenshot> m_MaybeScreenshotRequest;
};

osc::MainUIScreen::MainUIScreen() :
    Widget{std::make_unique<Impl>(*this)}
{}
osc::MainUIScreen::~MainUIScreen() noexcept = default;
void osc::MainUIScreen::open(const std::filesystem::path& path) { private_data().open(path); }
void osc::MainUIScreen::impl_on_mount() { private_data().on_mount(); }
void osc::MainUIScreen::impl_on_unmount() { private_data().on_unmount(); }
bool osc::MainUIScreen::impl_on_event(Event& e) { return private_data().on_event(e); }
void osc::MainUIScreen::impl_on_tick() { private_data().on_tick(); }
void osc::MainUIScreen::impl_on_draw() { private_data().onDraw(); }
