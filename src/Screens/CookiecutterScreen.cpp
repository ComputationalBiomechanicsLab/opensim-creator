#include "CookiecutterScreen.hpp"

#include "src/Platform/App.hpp"

#include <glm/vec4.hpp>
#include <imgui.h>

#include <utility>

class osc::CookiecutterScreen::Impl final {
public:

    void onMount()
    {
        // called when app receives the screen, but before it starts pumping events
        // into it, ticking it, drawing it, etc.

        osc::ImGuiInit();  // boot up ImGui support
    }

    void onUnmount()
    {
        // called when the app is going to stop pumping events/ticks/draws into this
        // screen (e.g. because the app is quitting, or transitioning to some other screen)

        osc::ImGuiShutdown();  // shutdown ImGui support
    }

    void onEvent(SDL_Event const& e)
    {
        // called when the app receives an event from the operating system

        if (e.type == SDL_QUIT)
        {
            App::upd().requestQuit();
            return;
        }
        else if (osc::ImGuiOnEvent(e))
        {
            return;  // ImGui handled this particular event
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
        // called once per frame. Code in here should use drawing primitives, OpenGL, ImGui,
        // etc. to draw things into the screen. The application does not clear the screen
        // buffer between frames (it's assumed that your code does this when it needs to)

        osc::ImGuiNewFrame();  // tell ImGui you're about to start drawing a new frame

        App::upd().clearScreen(glm::vec4{0.0f, 0.0f, 0.0f, 0.0f});  // set app window bg color

        ImGui::Begin("cookiecutter panel");
        ImGui::Text("hello world");
        ImGui::Checkbox("checkbox_state", &m_CheckboxState);
        ImGui::End();

        osc::ImGuiRender();  // tell ImGui to render any ImGui widgets since calling ImGuiNewFrame();
    }

private:
    bool m_CheckboxState = false;
};


// public API (PIMPL)

osc::CookiecutterScreen::CookiecutterScreen() :
    m_Impl{new Impl{}}
{
}

osc::CookiecutterScreen::CookiecutterScreen(CookiecutterScreen&& tmp) noexcept :
    m_Impl{std::exchange(tmp.m_Impl, nullptr)}
{
}

osc::CookiecutterScreen& osc::CookiecutterScreen::operator=(CookiecutterScreen&& tmp) noexcept
{
    std::swap(m_Impl, tmp.m_Impl);
    return *this;
}

osc::CookiecutterScreen::~CookiecutterScreen() noexcept
{
    delete m_Impl;
}

void osc::CookiecutterScreen::onMount()
{
    m_Impl->onMount();
}

void osc::CookiecutterScreen::onUnmount()
{
    m_Impl->onUnmount();
}

void osc::CookiecutterScreen::onEvent(SDL_Event const& e)
{
    m_Impl->onEvent(e);
}

void osc::CookiecutterScreen::onTick()
{
    m_Impl->onTick();
}

void osc::CookiecutterScreen::onDraw()
{
    m_Impl->onDraw();
}
