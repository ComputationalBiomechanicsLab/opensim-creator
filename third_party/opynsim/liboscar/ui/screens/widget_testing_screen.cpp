#include "widget_testing_screen.h"

#include <liboscar/platform/app.h>
#include <liboscar/platform/widget_private.h>
#include <liboscar/ui/oscimgui.h>

#include <cstddef>
#include <memory>
#include <utility>

class osc::WidgetTestingScreen::Impl final : public WidgetPrivate {
public:
    explicit Impl(WidgetTestingScreen& owner, std::unique_ptr<Widget> widget) :
        WidgetPrivate{owner, nullptr},
        widget_{std::move(widget)}
    {
        widget_->set_parent(this->owner());
        set_name("TabTestingScreen");
    }

    void on_mount()
    {
        widget_->on_mount();
        App::upd().make_main_loop_polling();
    }

    void on_unmount()
    {
        App::upd().make_main_loop_waiting();
        widget_->on_unmount();
    }

    bool on_event(Event& e)
    {
        bool handled = ui_context_.on_event(e);
        handled = widget_->on_event(e) or handled;
        return handled;
    }

    void on_tick()
    {
        widget_->on_tick();
    }

    void on_draw()
    {
        App::upd().clear_main_window();
        ui_context_.on_start_new_frame();
        widget_->on_draw();
        ui_context_.render();

        ++frames_shown_;
        if (frames_shown_ >= min_frames_shown_ and
            App::get().frame_start_time() >= close_time_) {

            App::upd().request_quit();
        }
    }

private:
    ui::Context ui_context_{App::upd()};
    std::unique_ptr<Widget> widget_;
    size_t min_frames_shown_ = 2;
    size_t frames_shown_ = 0;
    AppClock::duration min_open_duration_ = AppSeconds{0};
    AppClock::time_point close_time_ = App::get().frame_start_time() + min_open_duration_;
};

osc::WidgetTestingScreen::WidgetTestingScreen(std::unique_ptr<Widget> widget) :
    Widget{std::make_unique<Impl>(*this, std::move(widget))}
{}
void osc::WidgetTestingScreen::impl_on_mount() { private_data().on_mount(); }
void osc::WidgetTestingScreen::impl_on_unmount() { private_data().on_unmount(); }
bool osc::WidgetTestingScreen::impl_on_event(Event& e) { return private_data().on_event(e); }
void osc::WidgetTestingScreen::impl_on_tick() { private_data().on_tick(); }
void osc::WidgetTestingScreen::impl_on_draw() { private_data().on_draw(); }
