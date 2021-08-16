#include "cookiecutter_screen.hpp"

#include "src/app.hpp"
#include "src/3d/gl.hpp"

#include <imgui.h>

struct osc::cookiecutter_screen::Impl final {
    bool checkbox_state = false;
};

// public API

osc::cookiecutter_screen::cookiecutter_screen() :
    m_Impl{new Impl{}} {
}

osc::cookiecutter_screen::~cookiecutter_screen() noexcept = default;

void osc::cookiecutter_screen::on_mount() {
    // called when app receives the screen, but before it starts pumping events
    // into it, ticking it, drawing it, etc.

    osc::ImGuiInit();  // boot up ImGui support
}

void osc::cookiecutter_screen::on_unmount() {
    // called when the app is going to stop pumping events/ticks/draws into this
    // screen (e.g. because the app is quitting, or transitioning to some other screen)

    osc::ImGuiShutdown();  // shutdown ImGui support
}

void osc::cookiecutter_screen::on_event(SDL_Event const& e) {
    // called when the app receives an event from the operating system

    // pump event into ImGui, if using it:
    if (osc::ImGuiOnEvent(e)) {
        return;  // ImGui handled this particular event
    }
}

void osc::cookiecutter_screen::tick(float dt_secs) {
    // called once per frame, before drawing, with a timedelta from the last call
    // to `tick`

    // use this if you need to regularly update something (e.g. an animation, or
    // file polling)
}

void osc::cookiecutter_screen::draw() {
    // called once per frame. Code in here should use drawing primitives, OpenGL, ImGui,
    // etc. to draw things into the screen. The application does not clear the screen
    // buffer between frames (it's assumed that your code does this when it needs to)

    osc::ImGuiNewFrame();  // tell ImGui you're about to start drawing a new frame

    gl::ClearColor(0.0f, 0.0f, 0.0f, 0.0f);  // set app window bg color
    gl::Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);  // clear app window with bg color

    ImGui::Begin("cookiecutter panel");
    ImGui::Text("hello world");
    ImGui::Checkbox("checkbox_state", &m_Impl->checkbox_state);

    osc::ImGuiRender();  // tell ImGui to render any ImGui widgets since calling ImGuiNewFrame();
}
