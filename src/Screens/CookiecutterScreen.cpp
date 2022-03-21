#include "CookiecutterScreen.hpp"

#include "src/Platform/App.hpp"

#include <imgui.h>

struct osc::CookiecutterScreen::Impl final {
    bool checkboxState = false;
};

// public API

osc::CookiecutterScreen::CookiecutterScreen() :
    m_Impl{new Impl{}}
{
}

osc::CookiecutterScreen::~CookiecutterScreen() noexcept = default;

void osc::CookiecutterScreen::onMount()
{
    // called when app receives the screen, but before it starts pumping events
    // into it, ticking it, drawing it, etc.

    osc::ImGuiInit();  // boot up ImGui support
}

void osc::CookiecutterScreen::onUnmount()
{
    // called when the app is going to stop pumping events/ticks/draws into this
    // screen (e.g. because the app is quitting, or transitioning to some other screen)

    osc::ImGuiShutdown();  // shutdown ImGui support
}

void osc::CookiecutterScreen::onEvent(SDL_Event const& e)
{
    // called when the app receives an event from the operating system

    // pump event into ImGui, if using it:
    if (osc::ImGuiOnEvent(e))
    {
        return;  // ImGui handled this particular event
    }
}

void osc::CookiecutterScreen::tick(float)
{
    // called once per frame, before drawing, with a timedelta from the last call
    // to `tick`

    // use this if you need to regularly update something (e.g. an animation, or
    // file polling)
}

void osc::CookiecutterScreen::draw()
{
    // called once per frame. Code in here should use drawing primitives, OpenGL, ImGui,
    // etc. to draw things into the screen. The application does not clear the screen
    // buffer between frames (it's assumed that your code does this when it needs to)

    osc::ImGuiNewFrame();  // tell ImGui you're about to start drawing a new frame

    App::cur().clearScreen({0.0f, 0.0f, 0.0f, 0.0f});  // set app window bg color

    ImGui::Begin("cookiecutter panel");
    ImGui::Text("hello world");
    ImGui::Checkbox("checkbox_state", &m_Impl->checkboxState);

    osc::ImGuiRender();  // tell ImGui to render any ImGui widgets since calling ImGuiNewFrame();
}
