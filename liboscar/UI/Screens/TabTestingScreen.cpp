#include "TabTestingScreen.h"

#include <liboscar/Platform/App.h>
#include <liboscar/Platform/ScreenPrivate.h>
#include <liboscar/UI/Tabs/Tab.h>
#include <liboscar/UI/Tabs/TabRegistryEntry.h>
#include <liboscar/UI/oscimgui.h>

#include <cstddef>
#include <memory>
#include <utility>

class osc::TabTestingScreen::Impl final : public ScreenPrivate {
public:
    explicit Impl(TabTestingScreen& owner, TabRegistryEntry registry_entry) :
        ScreenPrivate{owner, nullptr, "TabTestingScreen"},
        registry_entry_{std::move(registry_entry)}
    {}

    void on_mount()
    {
        current_tab_ = registry_entry_.construct_tab(&owner());
        current_tab_->on_mount();
        App::upd().make_main_loop_polling();
    }

    void on_unmount()
    {
        App::upd().make_main_loop_waiting();
        current_tab_->on_unmount();
        current_tab_.reset();
    }

    bool on_event(Event& e)
    {
        bool handled = ui_context_.on_event(e);
        handled = current_tab_->on_event(e) or handled;
        return handled;
    }

    void on_tick()
    {
        current_tab_->on_tick();
    }

    void on_draw()
    {
        App::upd().clear_screen();
        ui_context_.on_start_new_frame();
        current_tab_->on_draw();
        ui_context_.render();

        ++frames_shown_;
        if (frames_shown_ >= min_frames_shown_ and
            App::get().frame_start_time() >= close_time_) {

            App::upd().request_quit();
        }
    }

private:
    ui::Context ui_context_{App::upd()};
    TabRegistryEntry registry_entry_;
    std::unique_ptr<Tab> current_tab_;
    size_t min_frames_shown_ = 2;
    size_t frames_shown_ = 0;
    AppClock::duration min_open_duration_ = AppSeconds{0};
    AppClock::time_point close_time_ = App::get().frame_start_time() + min_open_duration_;
};

osc::TabTestingScreen::TabTestingScreen(const TabRegistryEntry& registry_entry) :
    Screen{std::make_unique<Impl>(*this, registry_entry)}
{}
void osc::TabTestingScreen::impl_on_mount() { private_data().on_mount(); }
void osc::TabTestingScreen::impl_on_unmount() { private_data().on_unmount(); }
bool osc::TabTestingScreen::impl_on_event(Event& e) { return private_data().on_event(e); }
void osc::TabTestingScreen::impl_on_tick() { private_data().on_tick(); }
void osc::TabTestingScreen::impl_on_draw() { private_data().on_draw(); }
