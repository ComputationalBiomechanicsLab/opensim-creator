#include "UiModelViewerScreen.hpp"

#include "src/3D/Gl.hpp"
#include "src/OpenSimBindings/UiModel.hpp"
#include "src/UI/UiModelViewer.hpp"
#include "src/App.hpp"


#include <imgui.h>

struct osc::UiModelViewerScreen::Impl final {
    std::string modelPath = App::resource("models/RajagopalModel/Rajagopal2015.osim").string();
    UiModel uim{modelPath};
    UiModelViewer viewer;
};

// public API

osc::UiModelViewerScreen::UiModelViewerScreen() :
    m_Impl{new Impl{}} {
}

osc::UiModelViewerScreen::~UiModelViewerScreen() noexcept = default;

void osc::UiModelViewerScreen::onMount() {
    // called when app receives the screen, but before it starts pumping events
    // into it, ticking it, drawing it, etc.

    App::cur().enableDebugMode();
    App::cur().disableVsync();

    osc::ImGuiInit();  // boot up ImGui support
}

void osc::UiModelViewerScreen::onUnmount() {
    // called when the app is going to stop pumping events/ticks/draws into this
    // screen (e.g. because the app is quitting, or transitioning to some other screen)

    osc::ImGuiShutdown();  // shutdown ImGui support
}

void osc::UiModelViewerScreen::onEvent(SDL_Event const& e) {
    // called when the app receives an event from the operating system

    // pump event into ImGui, if using it:
    if (osc::ImGuiOnEvent(e)) {
        return;  // ImGui handled this particular event
    }
}

void osc::UiModelViewerScreen::tick(float) {
    // called once per frame, before drawing, with a timedelta from the last call
    // to `tick`

    // use this if you need to regularly update something (e.g. an animation, or
    // file polling)
}

void osc::UiModelViewerScreen::draw() {
    // called once per frame. Code in here should use drawing primitives, OpenGL, ImGui,
    // etc. to draw things into the screen. The application does not clear the screen
    // buffer between frames (it's assumed that your code does this when it needs to)

    osc::ImGuiNewFrame();  // tell ImGui you're about to start drawing a new frame

    gl::ClearColor(0.0f, 0.0f, 0.0f, 0.0f);  // set app window bg color
    gl::Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);  // clear app window with bg color

    ImGui::Begin("cookiecutter panel");
    ImGui::Text("%.2f", ImGui::GetIO().Framerate);
    ImGui::End();

    ImGui::Begin("viewer", nullptr, ImGuiWindowFlags_MenuBar);
    auto resp = m_Impl->viewer.draw(m_Impl->uim);
    if (resp.hovertestResult) {
        ImGui::BeginTooltip();
        ImGui::Text("hello");
        ImGui::EndTooltip();
    }
    m_Impl->uim.setHovered(resp.hovertestResult);
    ImGui::End();

    osc::ImGuiRender();  // tell ImGui to render any ImGui widgets since calling ImGuiNewFrame();
}
