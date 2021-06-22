#include "experiments_screen.hpp"

#include "src/3d/gl.hpp"
#include "src/3d/gl_glm.hpp"
#include "src/application.hpp"
#include "src/resources.hpp"
#include "src/screens/splash_screen.hpp"
#include "src/screens/meshes_to_model_wizard_screen.hpp"
#include "src/utils/helpers.hpp"

#include <GL/glew.h>
#include <SDL_keyboard.h>
#include <SDL_keycode.h>
#include <glm/vec3.hpp>
#include <imgui.h>

#include <array>
#include <cassert>
#include <cstddef>
#include <stdexcept>
#include <string>
#include <type_traits>

using namespace osc;

struct Plain_color_shader final {
    gl::Program p = gl::CreateProgramFrom(
        gl::CompileFromSource<gl::Vertex_shader>(slurp_resource("shaders/plain_color.vert")),
        gl::CompileFromSource<gl::Fragment_shader>(slurp_resource("shaders/plain_color.frag")));

    static constexpr gl::Attribute_vec3 aPos{0};

    gl::Uniform_mat4 uModelMat{p, "uModelMat"};
    gl::Uniform_mat4 uViewMat{p, "uViewMat"};
    gl::Uniform_mat4 uProjMat{p, "uProjMat"};
    gl::Uniform_vec3 uRgb{p, "uRgb"};

    template<typename Vbo, typename T = typename Vbo::value_type>
    static gl::Vertex_array create_vao(Vbo& vbo) {
        static_assert(std::is_standard_layout<T>::value, "this is required for offsetof");

        gl::Vertex_array vao;
        gl::BindVertexArray(vao);
        gl::BindBuffer(vbo);
        gl::VertexAttribPointer(aPos, false, sizeof(T), offsetof(T, pos));
        gl::EnableVertexAttribArray(aPos);
        gl::BindVertexArray();
        return vao;
    }
};

struct Basic_vert {
    glm::vec3 pos;
};

static constexpr std::array<Basic_vert, 3> g_TriangleVerts = {{
    Basic_vert{{-1.0f, -1.0f, 0.0f}},  // bottom-left
    Basic_vert{{1.0f, -1.0f, 0.0f}},  // bottom-right
    Basic_vert{{0.0f, 1.0f, 0.0f}},  // top-middle
}};

// screen that shows a triangle
struct Triangle_test_screen final : public Screen {
    Plain_color_shader shader;
    gl::Array_buffer<Basic_vert> vbo{g_TriangleVerts};
    gl::Vertex_array vao = Plain_color_shader::create_vao(vbo);
    float rgb[3]{1.0f, 0.0f, 0.0f};

    bool on_event(SDL_Event const& e) override {
        if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE) {
            Application::current().request_screen_transition<Experiments_screen>();
            return true;
        }
        return false;
    }

    void draw() override {
        if (ImGui::Begin("editor")) {
            ImGui::ColorEdit3("rgb", rgb);
            ImGui::End();
        }

        gl::UseProgram(shader.p);
        gl::Uniform(shader.uModelMat, gl::identity_val);
        gl::Uniform(shader.uViewMat, gl::identity_val);
        gl::Uniform(shader.uProjMat, gl::identity_val);
        gl::Uniform(shader.uRgb, rgb);
        gl::BindVertexArray(vao);
        gl::DrawArrays(GL_TRIANGLES, 0, vbo.sizei());
        gl::BindVertexArray();
    }
};

struct Experiments_screen::Impl final {
};

static bool on_event_opengl_test_screen(osc::Experiments_screen::Impl&, SDL_Event const& e) {
    if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE) {
        Application::current().request_screen_transition<osc::Splash_screen>();
        return true;
    }
    return false;
}

static void on_draw_opengl_test_screen(osc::Experiments_screen::Impl&) {
    auto [w, h] = Application::current().window_dimensionsf();
    glm::vec2 window_dims{w, h};
    glm::vec2 menu_dims = {700.0f, 500.0f};

    // center the menu
    {
        glm::vec2 menu_pos = (window_dims - menu_dims) / 2.0f;
        ImGui::SetNextWindowPos(menu_pos);
        ImGui::SetNextWindowSize(ImVec2(menu_dims.x, -1));
        ImGui::SetNextWindowSizeConstraints(menu_dims, menu_dims);
    }

    ImGui::Begin("select experiment");

    if (ImGui::Button("OpenGl: hello triangle")){
        Application::current().request_screen_transition<Triangle_test_screen>();
    }

    if (ImGui::Button("OpenSim: Meshes to model wizard")) {
        Application::current().request_screen_transition<Meshes_to_model_wizard_screen>();
    }

    ImGui::End();
}

// public API

Experiments_screen::Experiments_screen() : impl{new Impl{}} {
}
Experiments_screen::~Experiments_screen() noexcept = default;

bool Experiments_screen::on_event(SDL_Event const& e) {
    return ::on_event_opengl_test_screen(*impl, e);
}
void Experiments_screen::draw() {
    ::on_draw_opengl_test_screen(*impl);
}
