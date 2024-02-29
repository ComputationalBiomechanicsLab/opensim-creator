#include "CookiecutterScreen.h"

#include <oscar/oscar.h>
#include <SDL_events.h>

#include <memory>

class osc::CookiecutterScreen::Impl final {
public:

    void onMount()
    {
        // called when app receives the screen, but before it starts pumping events
        // into it, ticking it, drawing it, etc.

        ui::context::Init();  // boot up 2D ui support (ImGui, plotting, etc.)
    }

    void onUnmount()
    {
        // called when the app is going to stop pumping events/ticks/draws into this
        // screen (e.g. because the app is quitting, or transitioning to some other screen)

        ui::context::Shutdown();  // shutdown 2D UI support
    }

    void onEvent(SDL_Event const& e)
    {
        // called when the app receives an event from the operating system

        if (e.type == SDL_QUIT) {
            App::upd().requestQuit();
            return;
        }
        else if (ui::context::OnEvent(e)) {
            return;  // an element in the 2D UI handled this event
        }
    }

    void onTick()
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

        ui::context::NewFrame();  // prepare the 2D UI for drawing a new frame

        App::upd().clearScreen(Color::clear());  // set app window bg color

        ImGui::Begin("cookiecutter panel");
        ui::Text("hello world");
        ImGui::Checkbox("checkbox_state", &m_CheckboxState);
        ImGui::End();

        ui::context::Render();  // render the 2D UI's drawing to the screen
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

void osc::CookiecutterScreen::implOnMount()
{
    m_Impl->onMount();
}

void osc::CookiecutterScreen::implOnUnmount()
{
    m_Impl->onUnmount();
}

void osc::CookiecutterScreen::implOnEvent(SDL_Event const& e)
{
    m_Impl->onEvent(e);
}

void osc::CookiecutterScreen::implOnTick()
{
    m_Impl->onTick();
}

void osc::CookiecutterScreen::implOnDraw()
{
    m_Impl->onDraw();
}
