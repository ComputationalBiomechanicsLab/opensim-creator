#include "opengl_test_screen.hpp"

#include "application.hpp"
#include "config.hpp"
#include "splash_screen.hpp"
#include "src/3d/gl.hpp"

#include <imgui.h>

#include <array>
#include <vector>

namespace {
    struct Plain_color_shader final {
        gl::Program p = gl::CreateProgramFrom(
            gl::Compile<gl::Vertex_shader>(osmv::config::shader_path("plain_color.vert")),
            gl::Compile<gl::Fragment_shader>(osmv::config::shader_path("plain_color.frag")));

        static constexpr gl::Attribute aPos = gl::AttributeAtLocation(0);

        gl::Uniform_mat4 uModelMat = gl::GetUniformLocation(p, "uModelMat");
        gl::Uniform_mat4 uViewMat = gl::GetUniformLocation(p, "uViewMat");
        gl::Uniform_mat4 uProjMat = gl::GetUniformLocation(p, "uProjMat");
        gl::Uniform_vec3 uRgb = gl::GetUniformLocation(p, "uRgb");

        template<typename Vbo, typename T = typename Vbo::value_type>
        static gl::Vertex_array create_vao(Vbo& vbo) {
            static_assert(std::is_standard_layout<T>::value, "this is required for offsetof");

            gl::Vertex_array vao = gl::GenVertexArrays();
            gl::BindVertexArray(vao);
            gl::BindBuffer(vbo);
            gl::VertexAttribPointer(aPos, 3, GL_FLOAT, GL_FALSE, sizeof(T), reinterpret_cast<void*>(offsetof(T, pos)));
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

namespace osmv {
    struct Opengl_test_screen_impl final {
        std::vector<std::string> demos = {"hello triangle"};
        size_t demo_shown = 0;

        struct Hello_triangle_impl {
            Plain_color_shader shader;
            gl::Array_bufferT<Basic_vert> vbo{triangle};
            gl::Vertex_array vao = Plain_color_shader::create_vao(vbo);
            float rgb[3]{1.0f, 0.0f, 0.0f};
        } hellotriangle;
    };
}

static void draw_demo0_hellotriangle(osmv::Opengl_test_screen_impl& impl) {
    auto& hti = impl.hellotriangle;
    Plain_color_shader& shader = hti.shader;

    if (ImGui::Begin("editor")) {
        ImGui::ColorEdit3("rgb", hti.rgb);
        ImGui::End();
    }

    gl::UseProgram(shader.p);
    gl::Uniform(shader.uModelMat, gl::identity_val);
    gl::Uniform(shader.uViewMat, gl::identity_val);
    gl::Uniform(shader.uProjMat, gl::identity_val);
    gl::Uniform(shader.uRgb, hti.rgb);
    gl::BindVertexArray(hti.vao);
    gl::DrawArrays(GL_TRIANGLES, 0, hti.vbo.sizei());
    gl::BindVertexArray();
}

osmv::Opengl_test_screen::Opengl_test_screen() : impl{new Opengl_test_screen_impl{}} {
}

osmv::Opengl_test_screen::~Opengl_test_screen() noexcept {
    delete impl;
}

bool osmv::Opengl_test_screen::on_event(SDL_Event const& e) {
    if (e.type == SDL_KEYDOWN and e.key.keysym.sym == SDLK_ESCAPE) {
        app().request_screen_transition<osmv::Splash_screen>();
        return true;
    }
    return false;
}

void osmv::Opengl_test_screen::draw() {
    switch (impl->demo_shown) {
    case 0:
        draw_demo0_hellotriangle(*impl);
        break;
    default:
        assert(false && "invalid demo index selected: this shouldn't happen");
    }

    if (ImGui::Begin("main panel")) {
        for (size_t i = 0; i < impl->demos.size(); ++i) {
            ImGui::Text("%s", impl->demos[i].c_str());
            if (i != impl->demo_shown) {
                ImGui::SameLine();
                if (ImGui::Button("show")) {
                    impl->demo_shown = i;
                }
            }
        }
    }
    ImGui::End();
}
