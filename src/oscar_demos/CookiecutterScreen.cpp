#include "CookiecutterScreen.h"

#include <oscar/oscar.h>
#include <SDL_events.h>

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

    void onEvent(SDL_Event const& e)
    {
        // called when the app receives an event from the operating system

        if (e.type == SDL_QUIT) {
            App::upd().request_quit();
            return;
        }
        else if (ui::context::on_event(e)) {
            return;  // an element in the 2D UI handled this event
        }
    }

    void on_tick()
    {
        // called once per frame, before drawing, with a timedelta from the last call
        // to `tick`

        // use this if you need to regularly update something (e.g. an animation, or
        // file polling)
    }

    void onDraw()
    {
        // called once per frame. Code in here should use drawing primitives, `Graphics`,
        // ImGui, etc. to draw things into the screen. The application does not clear the
        // screen buffer between frames (it's assumed that your code does this when it needs
        // to)

        ui::context::on_start_new_frame();  // prepare the 2D UI for drawing a new frame

        App::upd().clear_screen(Color::clear());  // set app window bg color

        ui::begin_panel("cookiecutter panel");
        ui::draw_text("hello world");
        ui::draw_checkbox("checkbox_state", &m_CheckboxState);
        ui::end_panel();

        ui::context::render();  // render the 2D UI's drawing to the screen
    }

private:
    bool m_CheckboxState = false;
};


// public API (PIMPL)

osc::CookiecutterScreen::CookiecutterScreen() :
    m_Impl{std::make_unique<Impl>()}
{
}

osc::CookiecutterScreen::CookiecutterScreen(CookiecutterScreen&&) noexcept = default;
osc::CookiecutterScreen& osc::CookiecutterScreen::operator=(CookiecutterScreen&&) noexcept = default;
osc::CookiecutterScreen::~CookiecutterScreen() noexcept = default;

void osc::CookiecutterScreen::impl_on_mount()
{
    m_Impl->on_mount();
}

void osc::CookiecutterScreen::impl_on_unmount()
{
    m_Impl->on_unmount();
}

void osc::CookiecutterScreen::impl_on_event(SDL_Event const& e)
{
    m_Impl->onEvent(e);
}

void osc::CookiecutterScreen::impl_on_tick()
{
    m_Impl->on_tick();
}

void osc::CookiecutterScreen::impl_on_draw()
{
    m_Impl->onDraw();
}
