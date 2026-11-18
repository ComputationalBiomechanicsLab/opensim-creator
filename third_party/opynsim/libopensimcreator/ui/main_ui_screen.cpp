#include "main_ui_screen.h"

#include <libopensimcreator/platform/msmicons.h>
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
#include <filesystem>
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
    std::unique_ptr<Tab> load_configuration_defined_tab_if_necessary(
        const AppSettings& settings, const TabRegistry& tab_registry, Widget& parent)
    {
        if (auto maybe_requested_tab = settings.find_value("initial_tab")) {
            if (std::optional<TabRegistryEntry> maybe_entry =
                    tab_registry.find_by_name(to<std::string>(*maybe_requested_tab))) {
                return maybe_entry->construct_tab(&parent);
            }

            log_warn(
                "{}: cannot find a tab with this name in the tab registry: ignoring",
                to<std::string>(*maybe_requested_tab));
            log_warn("available tabsWithUnsavedChanges are:");
            for (auto&& tab_registry_entry : tab_registry) {
                log_warn("    {}", tab_registry_entry.name());
            }
        }

        return nullptr;
    }

    std::string make_save_prompt_string(std::span<Tab*> tabs_with_unsaved_changes)
    {
        std::stringstream ss;
        if (tabs_with_unsaved_changes.size() > 1) {
            ss << tabs_with_unsaved_changes.size()
               << " tabsWithUnsavedChanges have unsaved changes:\n";
        } else {
            ss << "A tab has unsaved changes:\n";
        }

        for (const Tab* t : tabs_with_unsaved_changes) {
            ss << "\n  - " << t->name();
        }
        ss << "\n\n";

        return std::move(ss).str();
    }

    std::vector<UID> extract_to_reversed_vector_of_ui_ds(std::span<Tab*> tabs)
    {
        std::vector<UID> rv;
        rv.reserve(tabs.size());  // upper limit
        for (auto it = tabs.rbegin(); it != tabs.rend(); ++it) {
            rv.push_back((*it)->id());
        }
        return rv;
    }

    class OpenFileEvent : public Event {
    public:
        explicit OpenFileEvent(std::filesystem::path path) :
            path_{std::move(path)}
        {
            enable_propagation();
        }

        const std::filesystem::path& path() { return path_; }

    private:
        std::filesystem::path path_;
    };
}

class osc::MainUIScreen::Impl final : public WidgetPrivate {
public:
    explicit Impl(MainUIScreen& owner) :
        WidgetPrivate{owner, nullptr}
    {
        set_name("MainUIScreen");
    }

    bool on_unhandled_key_up(const KeyEvent& e)
    {
        if (e.combination() == (ModifierKey::Ctrl | Key::PageUp) or
            e.combination() == (ModifierKey::Ctrl | ModifierKey::Alt | Key::LeftArrow)) {
            // `Ctrl+PageUp` or `Ctrl+Alt+Left`: focus the tab to the left of the currently-active
            // tab
            auto it = find_tab_by_id(active_tab_id_);
            if (it != tabs_.begin() and it != tabs_.end()) {
                --it;  // previous
                impl_select_tab((*it)->id());
            }
            return true;
        }
        if (e.combination() == (ModifierKey::Ctrl | Key::PageDown) or
            e.combination() == (ModifierKey::Ctrl | ModifierKey::Alt | Key::RightArrow)) {
            // `Ctrl+PageDown` or `Ctrl+Alt+Right`: focus the tab to the right of the
            // currently-active tab
            auto it = find_tab_by_id(active_tab_id_);
            if (it != tabs_.end() - 1) {
                ++it;  // next
                impl_select_tab((*it)->id());
            }
            return true;
        }
        if (e.combination() == (ModifierKey::Ctrl | Key::W) and tabs_.size() > 1 and
            active_tab_id_ != tabs_.front()->id()) {
            // `Ctrl+W`: close the current tab - unless it's the splash tab
            impl_close_tab(active_tab_id_);
            return true;
        }
        return false;
    }

    // called when an event is pumped into this screen but isn't handled by
    // either the global 2D UI context or the active tab
    bool on_unhandled_event(Event& e)
    {
        if (e.type() == EventType::KeyUp) {
            return on_unhandled_key_up(dynamic_cast<const KeyEvent&>(e));
        }
        return false;
    }

    UID add_tab(std::unique_ptr<Tab> tab) { return impl_add_tab(std::move(tab)); }

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
        if (not std::exchange(has_been_mounted_before_, true)) {
            // on first mount, place the splash tab at the front of the tabs collection
            add_tab(std::make_unique<SplashTab>(&owner()));

            // if the application configuration has requested that a specific tab should be opened,
            // then try looking it up and open it
            if (auto tab = load_configuration_defined_tab_if_necessary(
                    App::settings(), *App::singleton<TabRegistry>(), owner())) {
                add_tab(std::move(tab));
            }

            // focus on the rightmost tab
            if (not tabs_.empty()) {
                requested_tab_ = tabs_.back()->id();
            }
        }
    }

    void on_unmount()
    {
        // unmount the active tab before unmounting this (host) screen
        if (Tab* active = get_active_tab()) {
            try {
                active->on_unmount();
            } catch (const std::exception& ex) {
                // - the tab is faulty in some way
                // - soak up the exception to prevent the whole application from terminating
                // - and emit the error to the log, because we have to assume that this
                //   screen is about to die (it's being unmounted)
                log_error(
                    "MainUIScreen::on_unmount: unmounting active tab threw an exception: {}",
                    ex.what());
            }

            active_tab_id_ = UID::empty();
        }
    }

    bool on_event(Event& e)
    {
        if (e.type() == EventType::KeyDown or e.type() == EventType::KeyUp or
            e.type() == EventType::MouseButtonUp or e.type() == EventType::MouseMove or
            e.type() == EventType::MouseWheel) {

            // if the user just potentially changed something via a mouse/keyboard
            // interaction then the screen should be aggressively redrawn to reduce
            // and input delays

            App::upd().request_redraw();
        }

        bool handled = false;

        if (e.type() == EventType::KeyUp and
            dynamic_cast<const KeyEvent&>(e).combination() == (ModifierKey::Ctrl | Key::P)) {
            // `Ctrl+P`: "take a screenshot"
            maybe_screenshot_request_ = App::upd().main_window_request_screenshot();
            handled = true;
        } else if (ui_context_.on_event(e)) {
            // if the 2D UI captured the event, then assume that the event will be "handled"
            // during `Tab::onDraw` (immediate-mode UI)
            App::upd().request_redraw();
            handled = true;
        } else if (e.type() == EventType::Quit) {
            // if it's an application-level QUIT request, then it should be pumped into each
            // tab, while checking whether a tab wants to "block" the request (e.g. because it
            // wants to show a "do you want to save changes?" popup to the user

            bool at_least_one_tab_handled_quit = false;
            for (int i = 0; i < static_cast<int>(tabs_.size()); ++i) {
                try {
                    at_least_one_tab_handled_quit =
                        tabs_[i]->on_event(e) || at_least_one_tab_handled_quit;
                } catch (const std::exception& ex) {
                    log_error("MainUIScreen::on_event: exception thrown by tab: {}", ex.what());

                    // - the tab is faulty in some way
                    // - soak up the exception to prevent the whole application from terminating
                    // - then create a new tab containing the error message, so the user can see the
                    // error
                    const UID id = add_tab(std::make_unique<ErrorTab>(owner(), ex));
                    impl_select_tab(id);
                    impl_close_tab(tabs_[i]->id());
                }
            }

            if (not at_least_one_tab_handled_quit) {
                // if no tab handled the quit event, treat it as-if the user
                // has tried to close all tabs

                for (const auto& tab : tabs_) {
                    impl_close_tab(tab->id());
                }
                quit_requested_ = true;
            }

            // handle any deletion-related side-effects (e.g. showing save prompt)
            handle_deleted_tabs();

            if (not at_least_one_tab_handled_quit and not maybe_in_progress_save_dialog_) {
                // - if no tab handled a quit event
                // - and the UI isn't currently showing a save prompt
                // - then it's safe to outright quit the application from this screen
                App::upd().request_quit();
            }

            handled = true;
        } else if (auto* add_tab_ev = dynamic_cast<OpenTabEvent*>(&e)) {
            if (add_tab_ev->has_tab()) {
                impl_select_tab(impl_add_tab(add_tab_ev->take_tab()));
                handled = true;
            }
        } else if (const auto* close_tab_ev = dynamic_cast<CloseTabEvent*>(&e)) {
            impl_close_tab(close_tab_ev->tabid_to_close());
            handled = true;
        } else if (auto* open_file_ev = dynamic_cast<OpenFileEvent*>(&e)) {
            impl_select_tab(
                impl_add_tab(std::make_unique<LoadingTab>(&owner(), open_file_ev->path())));
            handled = true;
        } else if (dynamic_cast<ResetUIContextEvent*>(&e)) {
            impl_reset_imgui();
        } else if (Tab* active = get_active_tab()) {
            // if there's an active tab, pump the event into the active tab and check
            // whether the tab handled the event

            bool active_tab_handled_event = false;
            try {
                active_tab_handled_event = active->on_event(e);
            } catch (const std::exception& ex) {
                log_error("MainUIScreen::on_event: exception thrown by tab: {}", ex.what());

                // - the tab is faulty in some way
                // - soak up the exception to prevent the whole application from terminating
                // - then create a new tab containing the error message, so the user can see the
                // error
                UID id = add_tab(std::make_unique<ErrorTab>(owner(), ex));
                impl_select_tab(id);
                impl_close_tab(active_tab_id_);
            }

            // the event may have triggered tab deletions
            handle_deleted_tabs();

            if (active_tab_handled_event) {
                // If the user dragged a file into an open tab, and the tab accepted the
                // event (e.g. because it opened/imported the file), then the directory
                // of the dropped file should become the next directory that the user sees
                // if they subsequently open a file dialog.
                //
                // The reason that users find this useful is because they might've just
                // dragged a file into the UI to open something and, subsequently, want
                // to load associated data (#918).
                if (const auto* drop_ev = dynamic_cast<DropFileEvent*>(&e)) {
                    const auto parent_directory = drop_ev->path().parent_path();
                    if (not parent_directory.empty()) {
                        App::upd().set_prompt_initial_directory_to_show_fallback(parent_directory);
                    }
                }

                App::upd().request_redraw();
                handled = true;
            }
        }

        return handled or on_unhandled_event(e);
    }

    void on_tick()
    {
        // tick all the tabs, because they may internally be polling something (e.g.
        // updating something as a simulation runs)
        for (size_t i = 0; i < tabs_.size(); ++i) {
            try {
                tabs_[i]->on_tick();
            } catch (const std::exception& ex) {

                log_error("MainUIScreen::on_tick: tab thrown an exception: {}", ex.what());

                // - the tab is faulty in some way
                // - soak up the exception to prevent the whole application from terminating
                // - then create a new tab containing the error message, so the user can see the
                // error
                const UID id = add_tab(std::make_unique<ErrorTab>(owner(), ex));
                impl_select_tab(id);
                impl_close_tab(tabs_[i]->id());
            }
        }

        // clear the flagged-to-be-deleted tabs
        handle_deleted_tabs();

        // handle any currently-active user screenshot requests
        try_handle_screenshot_request();
    }

    void on_draw()
    {
        OSC_PERF("MainUIScreen/draw");

        {
            OSC_PERF("MainUIScreen/clear_screen");
            App::upd().main_window_clear();
        }

        ui_context_.on_start_new_frame();

        {
            OSC_PERF("MainUIScreen/drawUIContent");
            draw_ui_content();
        }

        if (ui_was_aggressively_reset_) {
            if (requested_tab_ == UID::empty()) {
                requested_tab_ = active_tab_id_;
            }
            active_tab_id_ = UID::empty();

            ui_context_.reset();
            App::upd().request_redraw();
            ui_was_aggressively_reset_ = false;

            return;
        }

        {
            OSC_PERF("MainUIScreen/render()");
            ui_context_.render();
        }
    }

    void draw_tab_specific_menu()
    {
        OSC_PERF("MainUIScreen/drawTabSpecificMenu");

        if (ui::begin_main_window_top_bar("##TabSpecificMenuBar")) {
            if (ui::begin_menu_bar()) {
                if (Tab* active = get_active_tab()) {
                    try {
                        active->on_draw_main_menu();
                    } catch (const std::exception& ex) {
                        log_error(
                            "MainUIScreen::drawTabSpecificMenu: tab thrown an exception: {}",
                            ex.what());

                        // - the tab is faulty in some way
                        // - soak up the exception to prevent the whole application from terminating
                        // - then create a new tab containing the error message, so the user can see
                        // the error
                        const UID id = add_tab(std::make_unique<ErrorTab>(owner(), ex));
                        impl_select_tab(id);
                        impl_close_tab(active_tab_id_);
                    }

                    if (ui_was_aggressively_reset_) {
                        // must return here to prevent the ImGui end_panel calls from erroring
                        return;
                    }
                }
                ui::end_menu_bar();
            }
            ui::end_panel();
            handle_deleted_tabs();
        }
    }

    void draw_tab_bar()
    {
        OSC_PERF("MainUIScreen/drawTabBar");

        ui::push_style_var(ui::StyleVar::FramePadding, ui::get_style_frame_padding() + 2.0f);
        ui::push_style_var(ui::StyleVar::ItemInnerSpacing, Vector2{5.0f, 0.0f});
        ui::push_style_var(ui::StyleVar::TabRounding, 10.0f);
        ui::push_style_var(ui::StyleVar::FrameRounding, 10.0f);
        if (ui::begin_main_window_top_bar("##MainWindowTabBarWrapper")) {
            if (ui::begin_menu_bar()) {
                if (ui::begin_tab_bar("##TabBar")) {
                    for (size_t i = 0; i < tabs_.size(); ++i) {
                        ui::TabItemFlags flags = ui::TabItemFlag::NoReorder;

                        if (i == 0) {
                            flags |= ui::TabItemFlag::NoCloseButton;  // splash screen
                        }

                        if (tabs_[i]->is_unsaved()) {
                            flags |= ui::TabItemFlag::UnsavedDocument;
                        }

                        if (tabs_[i]->id() == requested_tab_) {
                            flags |= ui::TabItemFlag::SetSelected;
                        }

                        if (tabs_[i]->id() == active_tab_id_ and
                            tabs_[i]->name() != active_tab_name_last_frame_) {

                            flags |= ui::TabItemFlag::SetSelected;
                            active_tab_name_last_frame_ = tabs_[i]->name();
                        }

                        ui::push_id(tabs_[i].get());
                        bool active = true;

                        if (ui::begin_tab_item(tabs_[i]->name(), &active, flags)) {
                            if (tabs_[i]->id() != active_tab_id_) {
                                if (Tab* active_tab = get_active_tab()) {
                                    active_tab->on_unmount();
                                }
                                tabs_[i]->on_mount();
                            }

                            active_tab_id_ = tabs_[i]->id();
                            active_tab_name_last_frame_ = tabs_[i]->name();

                            if (requested_tab_ == active_tab_id_) {
                                requested_tab_ = UID::empty();
                            }

                            if (ui_was_aggressively_reset_) {
                                return;
                            }

                            ui::end_tab_item();
                        }

                        ui::pop_id();
                        if (not active and i != 0) {  // can't close the splash tab
                            impl_close_tab(tabs_[i]->id());
                        }
                    }

                    // adding buttons to tab bars: https://github.com/ocornut/imgui/issues/3291
                    ui::draw_tab_item_button(MSMICONS_PLUS);

                    if (ui::begin_popup_context_menu("popup", ui::PopupFlag::MouseButtonLeft)) {
                        draw_add_new_tab_menu();
                        ui::end_popup();
                    }

                    ui::end_tab_bar();
                }
                ui::end_menu_bar();
            }

            ui::end_panel();
            handle_deleted_tabs();
        }
        ui::pop_style_var(4);
    }

    void draw_ui_content()
    {
        draw_tab_specific_menu();

        if (ui_was_aggressively_reset_) {
            return;
        }

        draw_tab_bar();

        if (ui_was_aggressively_reset_) {
            return;
        }

        // draw the active tab (if any)
        if (Tab* active = get_active_tab()) {
            try {
                OSC_PERF("MainUIScreen/drawActiveTab");
                active->on_draw();
            } catch (const std::exception& ex) {
                log_error("MainUIScreen::drawUIConent: tab thrown an exception: {}", ex.what());

                // - the tab is faulty in some way
                // - soak up the exception to prevent the whole application from terminating
                // - then create a new tab containing the error message, so the user can see the
                //   error
                // - and indicate that the UI was aggressively reset, because the drawcall may have
                //   thrown midway through rendering the 2D UI
                const UID id = add_tab(std::make_unique<ErrorTab>(owner(), ex));
                impl_select_tab(id);
                impl_close_tab(active_tab_id_);
                impl_reset_imgui();
            }

            handle_deleted_tabs();
        }

        if (ui_was_aggressively_reset_) {
            return;
        }

        if (maybe_in_progress_save_dialog_) {
            maybe_in_progress_save_dialog_->on_draw();
            if (maybe_in_progress_save_dialog_->is_closed()) {
                maybe_in_progress_save_dialog_.reset();
            }
        }
    }

    void draw_add_new_tab_menu()
    {
        if (ui::draw_menu_item(MSMICONS_EDIT " Editor")) {
            impl_select_tab(add_tab(std::make_unique<ModelEditorTab>(&owner())));
        }

        if (ui::draw_menu_item(MSMICONS_CUBE " Mesh Importer")) {
            impl_select_tab(add_tab(std::make_unique<MeshImporterTab>(&owner())));
        }

        const std::shared_ptr<const TabRegistry> tab_registry = App::singleton<TabRegistry>();
        if (not tab_registry->empty()) {
            if (ui::begin_menu("Experimental Tabs")) {
                for (auto&& tab_registry_entry : *tab_registry) {
                    if (ui::draw_menu_item(tab_registry_entry.name())) {
                        impl_select_tab(add_tab(tab_registry_entry.construct_tab(&owner())));
                    }
                }
                ui::end_menu();
            }
        }
    }

    std::vector<std::unique_ptr<Tab>>::iterator find_tab_by_id(UID id)
    {
        return rgs::find(tabs_, id, [](const auto& ptr) { return ptr->id(); });
    }

    Tab* get_tab_by_id(UID id)
    {
        auto it = find_tab_by_id(id);
        return it != tabs_.end() ? it->get() : nullptr;
    }

    Tab* get_active_tab() { return get_tab_by_id(active_tab_id_); }

    Tab* get_requested_tab() { return get_tab_by_id(requested_tab_); }

    UID impl_add_tab(std::unique_ptr<Tab> tab)
    {
        tab->set_parent(&this->owner());
        return tabs_.emplace_back(std::move(tab))->id();
    }

    void impl_select_tab(UID id) { requested_tab_ = id; }

    void impl_close_tab(UID id) { deleted_tabs_.insert(id); }

    void nuke_deleted_tabs()
    {
        int lowest_deleted_tab = std::numeric_limits<int>::max();
        for (UID id : deleted_tabs_) {
            auto it = rgs::find(tabs_, id, [](const auto& ptr) { return ptr->id(); });
            if (it != tabs_.end()) {
                if (id == active_tab_id_) {
                    (*it)->on_unmount();
                    active_tab_id_ = UID::empty();
                    lowest_deleted_tab =
                        min(lowest_deleted_tab, static_cast<int>(std::distance(tabs_.begin(), it)));
                }
                tabs_.erase(it);
            }
        }
        deleted_tabs_.clear();

        // coerce active tab, if it has become stale due to a deletion
        if (not get_requested_tab() and not get_active_tab() and not tabs_.empty()) {
            // focus the tab just to the left of the closed one
            if (1 <= lowest_deleted_tab and lowest_deleted_tab <= static_cast<int>(tabs_.size())) {
                requested_tab_ = tabs_[lowest_deleted_tab - 1]->id();
            } else {
                requested_tab_ = tabs_.front()->id();
            }
        }
    }

    std::vector<Tab*> collect_deleted_tabs_with_unsaved_changes()
    {
        std::vector<Tab*> rv;
        rv.reserve(deleted_tabs_.size());  // upper limit

        for (UID id : deleted_tabs_) {
            if (Tab* t = get_tab_by_id(id)) {
                if (t->is_unsaved()) {
                    rv.push_back(t);
                }
            }
        }
        return rv;
    }

    void handle_deleted_tabs()
    {
        // tabs aren't immediately deleted, because they may hold onto unsaved changes
        //
        // this top-level screen has to handle the unsaved changes. This is because it would be
        // annoying, from a UX PoV, to have each tab individually prompt the user. It is preferable
        // to have all the "do you want to save changes?" things in one prompt

        if (maybe_in_progress_save_dialog_) {
            return;  // nothing to process right now: waiting on user decision
        } else if (auto tabs = collect_deleted_tabs_with_unsaved_changes(); not tabs.empty()) {
            maybe_in_progress_save_dialog_.emplace(*this, tabs);
            return;  // wait for the user to handle unsaved changes
        } else {
            // all changes saved etc. - nuke all the tabs
            nuke_deleted_tabs();
        }
    }

    void impl_reset_imgui() { ui_was_aggressively_reset_ = true; }

    void try_handle_screenshot_request()
    {
        if (!maybe_screenshot_request_.valid()) {
            return;  // probably empty/errored
        }

        if (maybe_screenshot_request_.valid() and
            maybe_screenshot_request_.wait_for(0s) == std::future_status::ready) {

            const UID tab_id =
                add_tab(std::make_unique<ScreenshotTab>(&owner(), maybe_screenshot_request_.get()));
            impl_select_tab(tab_id);
        }
    }

private:
    OSC_OWNER_GETTERS(MainUIScreen);

    // Creates the top-level 2D UI context configuration (fonts, etc.).
    static ui::ContextConfiguration create_ui_context_config()
    {
        ui::ContextConfiguration rv;
        rv.set_base_imgui_ini_config_resource("OpenSimCreator/imgui_base_config.ini");
        rv.set_main_font_as_standard_plus_icon_font(
            "OpenSimCreator/fonts/Ruda-Bold.ttf",
            "OpenSimCreator/fonts/msmicons.ttf",
            {MSMICONS_MIN, MSMICONS_MAX});
        return rv;
    }

    // top-level 2D UI context (required for `ui::` calls to work).
    ui::Context ui_context_{App::upd(), create_ui_context_config()};

    // user-visible UI tabs
    std::vector<std::unique_ptr<Tab>> tabs_;

    // set of tabs that should be deleted once control returns to this screen
    std::set<UID> deleted_tabs_;

    // represents the current state of the save dialog (incl. any user-async prompts)
    class InProgressSaveDialog final {
    public:
        explicit InProgressSaveDialog(Impl& impl, std::span<Tab*> tabs_with_unsaved_changes) :
            parent_{&impl},
            popup_{
                &impl.owner(),
                SaveChangesPopupConfig{
                    "Save Changes?",
                    [this] { return on_user_selected_save_changes(); },
                    [this] { return on_user_selected_do_not_save_changes(); },
                    [this] { return on_user_cancelled_out_of_save_prompt(); },
                    make_save_prompt_string(tabs_with_unsaved_changes),
                }},
            async_prompt_queue_{extract_to_reversed_vector_of_ui_ds(tabs_with_unsaved_changes)}
        {
            popup_.open();
        }

        void on_draw()
        {
            if (popup_.begin_popup()) {
                popup_.on_draw();
                popup_.end_popup();
            }

            // Handle async requests
            if (popup_.is_open() and user_wants_to_start_saving_files_) {
                // 1) Poll+handle any in-progress requests
                if (maybe_active_async_save_) {
                    if (const auto result = maybe_active_async_save_->try_pop_result()) {
                        switch (*result) {
                        case TabSaveResult::Done: async_prompt_queue_.pop_back(); break;
                        case TabSaveResult::Cancelled:
                            user_wants_to_start_saving_files_ = false;
                            break;
                        }
                        maybe_active_async_save_.reset();
                    }
                }

                // 2) launch next request, if we're not currently handling one and there's one
                // waiting
                if (user_wants_to_start_saving_files_ and not maybe_active_async_save_ and
                    not async_prompt_queue_.empty()) {

                    if (Tab* next_tab = parent_->get_tab_by_id(async_prompt_queue_.back())) {
                        maybe_active_async_save_.emplace(*next_tab);
                    } else {
                        async_prompt_queue_.pop_back();  // Can't find tab?
                    }
                }

                // 3) If the queue is empty, transition to the next state (i.e. close the popup)
                if (async_prompt_queue_.empty()) {
                    parent_->nuke_deleted_tabs();
                    if (parent_->quit_requested_) {
                        App::upd().request_quit();
                    }
                    popup_.close();
                }
            }
        }

        bool is_closed() { return not popup_.is_open(); }

    private:
        // called by the "save changes?" popup when user opts to save changes
        bool on_user_selected_save_changes()
        {
            user_wants_to_start_saving_files_ = true;
            return false;  // The state transition happens during draw
        }

        // called by the "save changes?" popup when user opts to not save changes
        bool on_user_selected_do_not_save_changes()
        {
            async_prompt_queue_.clear();
            maybe_active_async_save_.reset();
            parent_->nuke_deleted_tabs();
            if (parent_->quit_requested_) {
                App::upd().request_quit();
            }
            return true;
        }

        // called by the "save changes?" popup when user clicks "cancel"
        bool on_user_cancelled_out_of_save_prompt()
        {
            async_prompt_queue_.clear();
            maybe_active_async_save_.reset();
            parent_->deleted_tabs_.clear();
            parent_->quit_requested_ = false;
            return true;
        }

        Impl* parent_ = nullptr;
        SaveChangesPopup popup_;
        bool user_wants_to_start_saving_files_ = false;
        std::vector<UID> async_prompt_queue_;
        class ActiveAsyncSaveRequest final {
        public:
            explicit ActiveAsyncSaveRequest(Tab& tab) :
                response_{tab.try_save()}
            {}

            std::optional<TabSaveResult> try_pop_result()
            {
                if (not response_.valid()) {
                    return TabSaveResult::Cancelled;
                }
                if (response_.wait_for(0s) != std::future_status::ready) {
                    return std::nullopt;
                }
                try {
                    return response_.get();
                } catch (const std::future_error&) {
                    return TabSaveResult::Cancelled;
                }
            }

        private:
            std::future<TabSaveResult> response_;
        };
        std::optional<ActiveAsyncSaveRequest> maybe_active_async_save_;
    };
    std::optional<InProgressSaveDialog> maybe_in_progress_save_dialog_;

    // currently-active UI tab
    UID active_tab_id_ = UID::empty();

    // cached version of active tab name - used to ensure the UI can re-focus a renamed tab
    std::string active_tab_name_last_frame_;

    // a tab that should become active next frame
    UID requested_tab_ = UID::empty();

    // `true` if `on_mount` has been called on this.
    bool has_been_mounted_before_ = false;

    // `true` if the this is midway through trying to quit
    bool quit_requested_ = false;

    // true if the UI context was aggressively reset by a tab (and, therefore, this screen should
    // reset the UI)
    bool ui_was_aggressively_reset_ = false;

    // `valid` if the user has requested a screenshot (that hasn't yet been handled)
    std::future<Screenshot> maybe_screenshot_request_;
};

osc::MainUIScreen::MainUIScreen() :
    Widget{std::make_unique<Impl>(*this)}
{}
osc::MainUIScreen::~MainUIScreen() noexcept = default;
void osc::MainUIScreen::open(const std::filesystem::path& path)
{
    private_data().open(path);
}
void osc::MainUIScreen::impl_on_mount()
{
    private_data().on_mount();
}
void osc::MainUIScreen::impl_on_unmount()
{
    private_data().on_unmount();
}
bool osc::MainUIScreen::impl_on_event(Event& e)
{
    return private_data().on_event(e);
}
void osc::MainUIScreen::impl_on_tick()
{
    private_data().on_tick();
}
void osc::MainUIScreen::impl_on_draw()
{
    private_data().on_draw();
}
