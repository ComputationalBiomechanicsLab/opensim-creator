#include "ImGuizmoDemoScreen.hpp"

#include "src/App.hpp"
#include "src/3d/Gl.hpp"
#include "src/3d/Model.hpp"
#include "src/screens/experimental/ExperimentsScreen.hpp"

#include <glm/gtc/type_ptr.hpp>
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>
#include <imgui.h>
#include <ImGuizmo.h>

struct osc::ImGuizmoDemoScreen::Impl final {
    PolarPerspectiveCamera camera = []() {
        PolarPerspectiveCamera rv;
        rv.focusPoint = {0.0f, 0.0f, 0.0f};
        rv.phi = 1.0f;
        rv.theta = 0.0f;
        rv.radius = 5.0f;
        return rv;
    }();

    bool translate = false;
    glm::mat4 cubeMtx{1.0f};
};

osc::ImGuizmoDemoScreen::ImGuizmoDemoScreen() :
    m_Impl{new Impl{}} {
}

osc::ImGuizmoDemoScreen::~ImGuizmoDemoScreen() noexcept = default;

void osc::ImGuizmoDemoScreen::onMount() {
    osc::ImGuiInit();
}

void osc::ImGuizmoDemoScreen::onUnmount() {
    osc::ImGuiShutdown();
}

void osc::ImGuizmoDemoScreen::onEvent(SDL_Event const& e) {
    if (osc::ImGuiOnEvent(e)) {
        return;
    }

    if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE) {
        App::cur().requestTransition<ExperimentsScreen>();
        return;
    }
}

void osc::ImGuizmoDemoScreen::draw() {
    osc::ImGuiNewFrame();

    gl::ClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    gl::Clear(GL_COLOR_BUFFER_BIT);

    glm::mat4 view = m_Impl->camera.getViewMtx();
    glm::vec2 dims = App::cur().dims();
    glm::mat4 projection = m_Impl->camera.getProjMtx(dims.x/dims.y);

    ImGuizmo::BeginFrame();
    ImGuizmo::SetRect(0, 0, dims.x, dims.y);
    glm::mat4 identity{1.0f};
    ImGuizmo::DrawGrid(glm::value_ptr(view), glm::value_ptr(projection), glm::value_ptr(identity), 100.f);
    ImGuizmo::DrawCubes(glm::value_ptr(view), glm::value_ptr(projection), glm::value_ptr(m_Impl->cubeMtx), 1);

    ImGui::Checkbox("translate", &m_Impl->translate);

    ImGuizmo::Manipulate(
        glm::value_ptr(view),
        glm::value_ptr(projection),
        m_Impl->translate ? ImGuizmo::TRANSLATE : ImGuizmo::ROTATE,
        ImGuizmo::LOCAL,
        glm::value_ptr(m_Impl->cubeMtx),
        NULL,
        NULL, //&snap[0],   // snap
        NULL,   // bound sizing?
        NULL);  // bound sizing snap

    osc::ImGuiRender();
}
