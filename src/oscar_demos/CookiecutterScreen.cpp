#include "CookiecutterScreen.h"

#include <oscar/oscar.h>

#include <memory>

class osc::CookiecutterScreen::Impl final {
public:

    void on_mount()
    {
        // called when app receives the screen, but before it starts pumping events
        // into it, ticking it, drawing it, etc.

        ui::context::init();  // boot up 2D ui support (ImGui, plotting, etc.)
    }

    void on_unmount()
    {
        // called when the app is going to stop pumping events/ticks/draws into this
        // screen (e.g. because the app is quitting, or transitioning to some other screen)

        ui::context::shutdown();  // shutdown 2D UI support
    }

    bool on_event(const Event& e)
    {
        if (e.type() == EventType::Quit) {
            // the app received a quit request from the operating system (e.g. because the
            // user clicked the X, or Alt+F4, etc.)
            App::upd().request_quit();
            return true;
        }
        else if (ui::context::on_event(e)) {
            return true;  // an element in the 2D UI handled this event
        }
        return false;   // nothing handled the event
    }

    void on_tick()
    {
        // called once per frame, before drawing, with a timedelta from the last call
        // to `on_tick`

        // use this if you need to regularly update something (e.g. an animation, or
        // file polling)
    }

    void onDraw()
    {
        // called once per frame. Code in here should use drawing primitives, `Graphics`,
        // `ui`, etc. to draw things into the screen. The application does not clear the
        // screen buffer between frames (it's assumed that your code does this when it needs
        // to)

        ui::context::on_start_new_frame();  // prepare the 2D UI for drawing a new frame

        App::upd().clear_screen();  // set app window bg color

        ui::begin_panel("cookiecutter panel");
        ui::draw_text("hello world");
        ui::draw_checkbox("checkbox_state", &checkbox_state_);
        ui::end_panel();

        ui::context::render();  // render the 2D UI's drawing to the screen
    }

private:
    bool checkbox_state_ = false;
};

osc::CookiecutterScreen::CookiecutterScreen() :
    impl_{std::make_unique<Impl>()}
{}

osc::CookiecutterScreen::CookiecutterScreen(CookiecutterScreen&&) noexcept = default;
osc::CookiecutterScreen& osc::CookiecutterScreen::operator=(CookiecutterScreen&&) noexcept = default;
osc::CookiecutterScreen::~CookiecutterScreen() noexcept = default;

void osc::CookiecutterScreen::impl_on_mount()
{
    impl_->on_mount();
}

void osc::CookiecutterScreen::impl_on_unmount()
{
    impl_->on_unmount();
}

bool osc::CookiecutterScreen::impl_on_event(const Event& e)
{
    return impl_->on_event(e);
}

void osc::CookiecutterScreen::impl_on_tick()
{
    impl_->on_tick();
}

void osc::CookiecutterScreen::impl_on_draw()
{
    impl_->onDraw();
}
