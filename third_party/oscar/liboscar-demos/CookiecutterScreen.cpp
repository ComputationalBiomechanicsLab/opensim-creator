#include "CookiecutterScreen.h"

#include <liboscar/platform/app.h>
#include <liboscar/platform/widget_private.h>
#include <liboscar/platform/events/event.h>
#include <liboscar/platform/events/event_type.h>
#include <liboscar/ui/oscimgui.h>

#include <memory>

class osc::CookiecutterScreen::Impl final : public WidgetPrivate {
public:
    explicit Impl(CookiecutterScreen& owner) :
        WidgetPrivate{owner, nullptr}
    {
        set_name("CookiecutterScreen");
    }

    void on_mount()
    {
        // called when app receives the screen, but before it starts pumping events
        // into it, ticking it, drawing it, etc.
    }

    void on_unmount()
    {
        // called when the app is going to stop pumping events/ticks/draws into this
        // screen (e.g. because the app is quitting, or transitioning to some other screen)
    }

    bool on_event(Event& e)
    {
        if (e.type() == EventType::Quit) {
            // the app received a quit request from the operating system (e.g. because the
            // user clicked the X, or Alt+F4, etc.)
            App::upd().request_quit();
            return true;
        }
        else if (ui_context_.on_event(e)) {
            return true;  // an element in the 2D UI handled this event
        }
        return false;   // nothing handled the event
    }

    void on_tick()
    {
        // called once per frame, before drawing

        // use this if you need to regularly update something (e.g. an animation, or
        // file polling)
    }

    void on_draw()
    {
        // called once per frame. Code in here should use drawing primitives, `Graphics`,
        // `ui`, etc. to draw things into the screen. The application does not clear the
        // screen buffer between frames (it's assumed that your code does this when it needs
        // to)

        ui_context_.on_start_new_frame();  // prepare the 2D UI for drawing a new frame

        App::upd().clear_main_window();  // set app window bg color

        ui::begin_panel("cookiecutter panel");
        ui::draw_text("hello world");
        ui::draw_checkbox("checkbox_state", &checkbox_state_);
        ui::end_panel();

        ui_context_.render();  // render the 2D UI's drawing to the screen
    }

private:
    ui::Context ui_context_{App::upd()};
    bool checkbox_state_ = false;
};

osc::CookiecutterScreen::CookiecutterScreen() :
    Widget{std::make_unique<Impl>(*this)}
{}
void osc::CookiecutterScreen::impl_on_mount() { private_data().on_mount(); }
void osc::CookiecutterScreen::impl_on_unmount() { private_data().on_unmount(); }
bool osc::CookiecutterScreen::impl_on_event(Event& e) { return private_data().on_event(e); }
void osc::CookiecutterScreen::impl_on_tick() { private_data().on_tick(); }
void osc::CookiecutterScreen::impl_on_draw() { private_data().on_draw(); }
