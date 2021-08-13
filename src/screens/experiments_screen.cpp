#include "experiments_screen.hpp"

#include "src/app.hpp"
#include "src/3d/3d.hpp"
#include "src/3d/gl.hpp"
#include "src/3d/gl_glm.hpp"
#include "src/utils/helpers.hpp"


//#include "src/screens/splash_screen.hpp"
//#include "src/screens/meshes_to_model_wizard_screen.hpp"

#include <GL/glew.h>
#include <SDL_keyboard.h>
#include <SDL_keycode.h>
#include <glm/vec3.hpp>
#include <imgui.h>
#include <ImGuizmo.h>

#include <array>
#include <cassert>
#include <cstddef>
#include <stdexcept>
#include <string>
#include <type_traits>

using namespace osc;

// private impl details
namespace {

    std::string slurp(char const* resource) {
        return slurp_into_string(App::resource(resource));
    }

    struct Basic_vert {
        glm::vec3 pos;
    };

    constexpr std::array<Basic_vert, 3> g_TriangleVerts = {{
        Basic_vert{{-1.0f, -1.0f, 0.0f}},  // bottom-left
        Basic_vert{{1.0f, -1.0f, 0.0f}},  // bottom-right
        Basic_vert{{0.0f, 1.0f, 0.0f}},  // top-middle
    }};

    // basic shader that colors primitives with no shading
    struct Plain_color_shader final {
        gl::Program p = gl::CreateProgramFrom(
            gl::CompileFromSource<gl::Vertex_shader>(slurp("shaders/plain_color.vert")),
            gl::CompileFromSource<gl::Fragment_shader>(slurp("shaders/plain_color.frag")));

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

    // a screen that draws the typical "hello triangle" OpenGL example
    struct Triangle_test_screen final : public Screen {
        Plain_color_shader shader;
        gl::Array_buffer<Basic_vert> vbo{g_TriangleVerts};
        gl::Vertex_array vao = Plain_color_shader::create_vao(vbo);
        float rgb[3]{1.0f, 0.0f, 0.0f};

        void on_mount() override {
            osc::ImGuiInit();
        }

        void on_unmount() override {
            osc::ImGuiShutdown();
        }

        void on_event(SDL_Event const& e) override {
            if (osc::ImGuiOnEvent(e)) {
                return;
            }

            if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE) {
                // TODO
                //Application::current().request_transition<Experiments_screen>();
                return;
            }
        }

        void draw() override {
            gl::Viewport(0, 0, App::cur().idims().x, App::cur().idims().y);
            gl::ClearColor(0.0f, 0.0f, 0.0f, 0.0f);
            gl::Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            osc::ImGuiNewFrame();

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

            osc::ImGuiRender();
        }
    };

    // a screen that draws the ImGuizmo test screen
    struct ImGuizmo_test_screen final : public Screen {

        Polar_perspective_camera camera = []() {
            Polar_perspective_camera rv;
            rv.pan = {0.0f, 0.0f, 0.0f};
            rv.phi = 1.0f;
            rv.theta = 0.0f;
            return rv;
        }();

        bool translate = false;
        glm::mat4 cube_mtx{1.0f};

        void on_mount() override {
            osc::ImGuiInit();
        }

        void on_unmount() override {
            osc::ImGuiShutdown();
        }

        void on_event(SDL_Event const& e) override {
            if (osc::ImGuiOnEvent(e)) {
                return;
            }

            if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE) {
                // TODO
                //Application::current().request_transition<Experiments_screen>();
                return;
            }
        }

        void draw() override {
            osc::ImGuiNewFrame();

            gl::ClearColor(0.0f, 0.0f, 0.0f, 0.0f);
            gl::Clear(GL_COLOR_BUFFER_BIT);

            glm::mat4 view = camera.view_matrix();
            glm::vec2 dims = App::cur().dims();
            glm::mat4 projection = camera.projection_matrix(dims.x/dims.y);

            ImGuizmo::BeginFrame();
            ImGuizmo::SetRect(0, 0, dims.x, dims.y);
            glm::mat4 identity{1.0f};
            ImGuizmo::DrawGrid(glm::value_ptr(view), glm::value_ptr(projection), glm::value_ptr(identity), 100.f);
            ImGuizmo::DrawCubes(glm::value_ptr(view), glm::value_ptr(projection), glm::value_ptr(cube_mtx), 1);

            ImGui::Checkbox("translate", &translate);

            ImGuizmo::Manipulate(
                glm::value_ptr(view),
                glm::value_ptr(projection),
                translate ? ImGuizmo::TRANSLATE : ImGuizmo::ROTATE,
                ImGuizmo::LOCAL,
                glm::value_ptr(cube_mtx),
                NULL,
                NULL, //&snap[0],   // snap
                NULL,   // bound sizing?
                NULL);  // bound sizing snap

            osc::ImGuiRender();
        }
    };
}

// experiments screen impl
struct Experiments_screen::Impl final {
};

// public API

osc::Experiments_screen::Experiments_screen() : impl{new Impl{}} {
}

osc::Experiments_screen::~Experiments_screen() noexcept = default;

void osc::Experiments_screen::on_mount() {
    osc::ImGuiInit();
}

void osc::Experiments_screen::on_unmount() {
    osc::ImGuiShutdown();
}

void osc::Experiments_screen::on_event(SDL_Event const& e) {
    if (osc::ImGuiOnEvent(e)) {
        return;
    }

    if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE) {
        // TODO
        //Application::current().request_transition<osc::Splash_screen>();
        return;
    }
}

void Experiments_screen::draw() {
    gl::ClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    gl::Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    osc::ImGuiNewFrame();

    glm::vec2 dims = App::cur().dims();
    glm::vec2 menu_dims = {700.0f, 500.0f};

    // center the menu
    {
        glm::vec2 menu_pos = (dims - menu_dims) / 2.0f;
        ImGui::SetNextWindowPos(menu_pos);
        ImGui::SetNextWindowSize(ImVec2(menu_dims.x, -1));
        ImGui::SetNextWindowSizeConstraints(menu_dims, menu_dims);
    }

    ImGui::Begin("select experiment");

    if (ImGui::Button("OpenGl: hello triangle")) {
        App::cur().request_transition<Triangle_test_screen>();
    }

    if (ImGui::Button("OpenSim: Meshes to model wizard")) {
        // TODO
        // Application::current().request_transition<Meshes_to_model_wizard_screen>();
    }

    if (ImGui::Button("ImGuizmo test screen")) {
        App::cur().request_transition<ImGuizmo_test_screen>();
    }

    ImGui::End();

    osc::ImGuiRender();
}
