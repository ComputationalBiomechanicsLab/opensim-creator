#include "imguizmo_demo_screen.hpp"

#include "src/app.hpp"
#include "src/3d/gl.hpp"
#include "src/3d/model.hpp"

#include <glm/gtc/type_ptr.hpp>
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>
#include <imgui.h>
#include <ImGuizmo.h>

struct osc::Imguizmo_demo_screen::Impl final {
    Polar_perspective_camera camera = []() {
        Polar_perspective_camera rv;
        rv.pan = {0.0f, 0.0f, 0.0f};
        rv.phi = 1.0f;
        rv.theta = 0.0f;
        rv.radius = 5.0f;
        return rv;
    }();

    bool translate = false;
    glm::mat4 cube_mtx{1.0f};
};

osc::Imguizmo_demo_screen::Imguizmo_demo_screen() :
    m_Impl{new Impl{}} {
}

osc::Imguizmo_demo_screen::~Imguizmo_demo_screen() noexcept = default;

void osc::Imguizmo_demo_screen::on_mount() {
    osc::ImGuiInit();
}

void osc::Imguizmo_demo_screen::on_unmount() {
    osc::ImGuiShutdown();
}

void osc::Imguizmo_demo_screen::on_event(SDL_Event const& e) {
    if (osc::ImGuiOnEvent(e)) {
        return;
    }

    if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE) {
        // TODO
        //Application::current().request_transition<Experiments_screen>();
        return;
    }
}

void osc::Imguizmo_demo_screen::draw() {
    osc::ImGuiNewFrame();

    gl::ClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    gl::Clear(GL_COLOR_BUFFER_BIT);

    glm::mat4 view = m_Impl->camera.view_matrix();
    glm::vec2 dims = App::cur().dims();
    glm::mat4 projection = m_Impl->camera.projection_matrix(dims.x/dims.y);

    ImGuizmo::BeginFrame();
    ImGuizmo::SetRect(0, 0, dims.x, dims.y);
    glm::mat4 identity{1.0f};
    ImGuizmo::DrawGrid(glm::value_ptr(view), glm::value_ptr(projection), glm::value_ptr(identity), 100.f);
    ImGuizmo::DrawCubes(glm::value_ptr(view), glm::value_ptr(projection), glm::value_ptr(m_Impl->cube_mtx), 1);

    ImGui::Checkbox("translate", &m_Impl->translate);

    ImGuizmo::Manipulate(
        glm::value_ptr(view),
        glm::value_ptr(projection),
        m_Impl->translate ? ImGuizmo::TRANSLATE : ImGuizmo::ROTATE,
        ImGuizmo::LOCAL,
        glm::value_ptr(m_Impl->cube_mtx),
        NULL,
        NULL, //&snap[0],   // snap
        NULL,   // bound sizing?
        NULL);  // bound sizing snap

    osc::ImGuiRender();
}
