#include "experiments_screen.hpp"

#include "src/3d/gl.hpp"
#include "src/3d/gl_glm.hpp"
#include "src/application.hpp"
#include "src/resources.hpp"
#include "src/screens/splash_screen.hpp"
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

namespace {
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

    static constexpr std::array<Basic_vert, 3> triangle = {{
        Basic_vert{{-1.0f, -1.0f, 0.0f}},  // bottom-left
        Basic_vert{{1.0f, -1.0f, 0.0f}},  // bottom-right
        Basic_vert{{0.0f, 1.0f, 0.0f}},  // top-middle
    }};
}

struct Experiments_screen::Impl final {
    std::array<std::string, 1> demos = {"hello triangle"};
    size_t demo_shown = 0;

    struct Hello_triangle_demo {
        Plain_color_shader shader;
        gl::Array_buffer<Basic_vert> vbo{triangle};
        gl::Vertex_array vao = Plain_color_shader::create_vao(vbo);
        float rgb[3]{1.0f, 0.0f, 0.0f};

        void draw() {
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
    } hellotriangle;
};

static bool on_event_opengl_test_screen(osc::Experiments_screen::Impl& impl, SDL_Event const& e) {
    if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE) {
        Application::current().request_screen_transition<osc::Splash_screen>();
        return true;
    }
    return false;
}

static void on_draw_opengl_test_screen(osc::Experiments_screen::Impl& impl) {
    switch (impl.demo_shown) {
    case 0:
        impl.hellotriangle.draw();
        break;
    default:
        OSC_ASSERT(false && "invalid demo index selected: this is a developer error");
    }

    if (ImGui::Begin("main panel")) {
        for (size_t i = 0; i < impl.demos.size(); ++i) {
            ImGui::TextUnformatted(impl.demos[i].c_str());
            if (i != impl.demo_shown) {
                ImGui::SameLine();
                if (ImGui::Button("show")) {
                    impl.demo_shown = i;
                }
            }
        }
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
