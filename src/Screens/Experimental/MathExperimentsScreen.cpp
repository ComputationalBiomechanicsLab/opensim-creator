#include "MathExperimentsScreen.hpp"

#include "src/3D/Gl.hpp"
#include "src/3D/Model.hpp"
#include "src/App.hpp"

#include <imgui.h>
#include <glm/vec2.hpp>
#include <glm/gtc/matrix_transform.hpp>

struct osc::MathExperimentsScreen::Impl final {
    Transform boxTransform = Transform::atPosition({75.0f, 75.0f, 0.0f});
};

// public API

osc::MathExperimentsScreen::MathExperimentsScreen() :
    m_Impl{new Impl{}} {
}

osc::MathExperimentsScreen::~MathExperimentsScreen() noexcept = default;

void osc::MathExperimentsScreen::onMount() {
    // called when app receives the screen, but before it starts pumping events
    // into it, ticking it, drawing it, etc.

    osc::ImGuiInit();  // boot up ImGui support
}

void osc::MathExperimentsScreen::onUnmount() {
    // called when the app is going to stop pumping events/ticks/draws into this
    // screen (e.g. because the app is quitting, or transitioning to some other screen)

    osc::ImGuiShutdown();  // shutdown ImGui support
}

void osc::MathExperimentsScreen::onEvent(SDL_Event const& e) {
    // called when the app receives an event from the operating system

    // pump event into ImGui, if using it:
    if (osc::ImGuiOnEvent(e)) {
        return;  // ImGui handled this particular event
    }
}

void osc::MathExperimentsScreen::tick(float) {
    // called once per frame, before drawing, with a timedelta from the last call
    // to `tick`

    // use this if you need to regularly update something (e.g. an animation, or
    // file polling)
}

void osc::MathExperimentsScreen::draw() {
    // called once per frame. Code in here should use drawing primitives, OpenGL, ImGui,
    // etc. to draw things into the screen. The application does not clear the screen
    // buffer between frames (it's assumed that your code does this when it needs to)

    osc::ImGuiNewFrame();  // tell ImGui you're about to start drawing a new frame

    gl::ClearColor(1.0f, 1.0f, 1.0f, 1.0f);  // set app window bg color
    gl::Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);  // clear app window with bg color

    glm::vec2 screenCenter = ImGui::GetMainViewport()->GetCenter();
    glm::vec2 mousePos = ImGui::GetIO().MousePos;
    glm::vec2 mainVec = mousePos - screenCenter;

    ImDrawList* dl = ImGui::GetForegroundDrawList();

    // draw vector
    dl->AddLine(screenCenter, mousePos, 0xff000000, 1.0f);

    // draw x component
    {
        glm::vec2 xBegin = screenCenter;
        glm::vec2 xEnd = {mousePos.x, screenCenter.y};
        ImVec2 xMid = ImVec2{xBegin.x + (xEnd.x - xBegin.x)/2.0f, xBegin.y};

        char buf[256];
        std::snprintf(buf, sizeof(buf), "%.3f", xEnd.x - xBegin.x);

        dl->AddLine(xBegin, xEnd, 0xffaaaaaa, 1.0f);
        dl->AddText(xMid, 0xff000000, buf);
    }

    // draw y component
    {
        ImVec2 yBegin = screenCenter;
        ImVec2 yEnd = {screenCenter.x, mousePos.y};
        ImVec2 yMid = ImVec2{yBegin.x, yBegin.y + (yEnd.y - yBegin.y)/2.0f};

        char buf[256];
        std::snprintf(buf, sizeof(buf), "%.3f", yEnd.y - yBegin.y);

        dl->AddLine(yBegin, yEnd, 0xffaaaaaa, 1.0f);
        dl->AddText(yMid, 0xff000000, buf);
    }

    // draw other vector
    auto proj = [](glm::vec2 const& a, glm::vec2 const& b) {
        return glm::dot(a, b)/glm::dot(b, b) * b;
    };
    {
        glm::vec2 otherVec = {0.0, -50.0f};
        dl->AddLine(screenCenter, screenCenter + otherVec, 0xff00ff00, 2.0f);

        glm::vec2 projvec = proj(otherVec, mainVec);
        dl->AddLine(screenCenter, glm::vec2{screenCenter} + projvec, 0xffff0000, 2.0f);
    }

    ImGui::Begin("cookiecutter panel");
    ImGui::Text("screen center = %.2f, %.2f", screenCenter.x, screenCenter.y);
    ImGui::Text("mainvec = %.2f, %.2f", mainVec.x, mainVec.y);
    glm::vec2 relVec = toMat4(m_Impl->boxTransform) * glm::vec4{mousePos, 0.0f, 1.0f};
    ImGui::Text("relvec (mtx) = %.2f, %.2f", relVec.x, relVec.y);
    glm::vec2 relVecF = transformPoint(m_Impl->boxTransform, glm::vec3{mousePos, 0.0f});
    ImGui::Text("relvec (func) = %.2f, %.2f", relVecF.x, relVecF.y);

    osc::ImGuiRender();  // tell ImGui to render any ImGui widgets since calling ImGuiNewFrame();
}
