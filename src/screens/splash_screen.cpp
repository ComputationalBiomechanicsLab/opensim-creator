#include "splash_screen.hpp"

#include "osc_build_config.hpp"
#include "src/3d/drawlist.hpp"
#include "src/3d/gl.hpp"
#include "src/3d/gpu_cache.hpp"
#include "src/3d/mesh_generation.hpp"
#include "src/3d/mesh_instance.hpp"
#include "src/3d/polar_camera.hpp"
#include "src/3d/render_target.hpp"
#include "src/3d/renderer.hpp"
#include "src/3d/texturing.hpp"
#include "src/application.hpp"
#include "src/config.hpp"
#include "src/constants.hpp"
#include "src/screens/imgui_demo_screen.hpp"
#include "src/screens/loading_screen.hpp"
#include "src/screens/model_editor_screen.hpp"
#include "src/screens/opengl_test_screen.hpp"
#include "src/utils/scope_guard.hpp"
#include "src/widgets/main_menu_about_tab.hpp"
#include "src/widgets/main_menu_file_tab.hpp"

#include <GL/glew.h>
#include <OpenSim/Simulation/Model/Model.h>
#include <SDL_keyboard.h>
#include <SDL_keycode.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/mat4x3.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <imgui.h>

#include <algorithm>
#include <cstdio>
#include <filesystem>
#include <string>
#include <vector>

namespace fs = std::filesystem;
using namespace osc;

namespace {
    //
    // useful for rendering quads etc.
    struct Plain_texture_shader final {
        gl::Program p = gl::CreateProgramFrom(
            gl::Compile<gl::Vertex_shader>(osc::config::shader_path("plain_texture.vert")),
            gl::Compile<gl::Fragment_shader>(osc::config::shader_path("plain_texture.frag")));

        static constexpr gl::Attribute aPos = gl::AttributeAtLocation(0);
        static constexpr gl::Attribute aTexCoord = gl::AttributeAtLocation(1);

        gl::Uniform_mat4 uMVP = gl::GetUniformLocation(p, "uMVP");
        gl::Uniform_float uTextureScaler = gl::GetUniformLocation(p, "uTextureScaler");
        gl::Uniform_sampler2d uSampler0 = gl::GetUniformLocation(p, "uSampler0");

        template<typename Vbo, typename T = typename Vbo::value_type>
        static gl::Vertex_array create_vao(Vbo& vbo) {
            gl::Vertex_array vao = gl::GenVertexArrays();

            gl::BindVertexArray(vao);
            gl::BindBuffer(vbo);
            gl::VertexAttribPointer(aPos, 3, GL_FLOAT, GL_FALSE, sizeof(T), reinterpret_cast<void*>(offsetof(T, pos)));
            gl::EnableVertexAttribArray(aPos);
            gl::VertexAttribPointer(
                aTexCoord, 2, GL_FLOAT, GL_FALSE, sizeof(T), reinterpret_cast<void*>(offsetof(T, texcoord)));
            gl::EnableVertexAttribArray(aTexCoord);
            gl::BindVertexArray();

            return vao;
        }
    };
}

struct Splash_screen::Impl final {
    Main_menu_file_tab_state mm_state;
    gl::Texture_2d logo =
        osc::load_tex(osc::config::resource_path("logo.png").string().c_str(), TexFlag_Flip_Pixels_Vertically);
    gl::Texture_2d cz_logo = osc::load_tex(
        osc::config::resource_path("chanzuckerberg_logo.png").string().c_str(), TexFlag_Flip_Pixels_Vertically);
    gl::Texture_2d tud_logo =
        osc::load_tex(osc::config::resource_path("tud_logo.png").string().c_str(), TexFlag_Flip_Pixels_Vertically);
    Gpu_cache cache;
    Drawlist drawlist;
    Polar_camera camera;
    glm::vec3 light_pos = {1.5f, 3.0f, 0.0f};
    glm::vec3 light_rgb = {248.0f / 255.0f, 247.0f / 255.0f, 247.0f / 255.0f};
    glm::vec4 background_rgba = {0.89f, 0.89f, 0.89f, 1.0f};
    glm::vec4 rim_rgba = {1.0f, 0.4f, 0.0f, 0.85f};
    Render_target render_target{1, 1, 1};
    Renderer renderer;
    Plain_texture_shader pts;
    gl::Array_bufferT<Textured_vert> quad_vbo{osc::shaded_textured_quad_verts().vert_data};
    gl::Vertex_array quad_vao = Plain_texture_shader::create_vao(quad_vbo);

    Impl() {
        glm::mat4 model_mtx = []() {
            glm::mat4 rv = glm::identity<glm::mat4>();

            // OpenSim: might contain floors at *exactly* Y = 0.0, so shift the chequered
            // floor down *slightly* to prevent Z fighting from planes rendered from the
            // model itself (the contact planes, etc.)
            rv = glm::translate(rv, {0.0f, -0.001f, 0.0f});
            rv = glm::rotate(rv, osc::pi_f / 2, {-1.0, 0.0, 0.0});
            rv = glm::scale(rv, {100.0f, 100.0f, 0.0f});

            return rv;
        }();

        Rgba32 color{glm::vec4{1.0f, 0.0f, 1.0f, 1.0f}};
        Mesh_instance& mi = drawlist.emplace_back(model_mtx, color, cache.floor_quad, cache.chequered_texture);
        mi.flags.is_shaded = false;
    }
};

// PIMPL forwarding for osc::Splash_screen

osc::Splash_screen::Splash_screen() : impl{new Impl{}} {
}

osc::Splash_screen::~Splash_screen() noexcept {
    delete impl;
}

static bool on_keydown(SDL_KeyboardEvent const& e) {
    SDL_Keycode sym = e.keysym.sym;

    if (e.keysym.mod & KMOD_CTRL) {
        // CTRL

        switch (sym) {
        case SDLK_n:
            main_menu_new();
            return true;
        case SDLK_o:
            main_menu_open();
            return true;
        case SDLK_q:
            Application::current().request_quit_application();
            return true;
        }
    }

    return false;
}

bool osc::Splash_screen::on_event(SDL_Event const& e) {
    if (e.type == SDL_KEYDOWN) {
        return on_keydown(e.key);
    }
    return false;
}

void osc::Splash_screen::draw() {
    Application& app = Application::current();

    constexpr glm::vec2 menu_dims = {700.0f, 700.0f};
    glm::vec2 window_dims;
    {
        auto [w, h] = app.window_dimensions();
        window_dims.x = static_cast<float>(w);
        window_dims.y = static_cast<float>(h);
    }

    // draw chequered floor background
    {
        impl->render_target.reconfigure(app.window_dimensions().w, app.window_dimensions().h, app.samples());

        Raw_drawcall_params params;
        params.passthrough_hittest_x = -1;
        params.passthrough_hittest_y = -1;
        params.view_matrix = impl->camera.view_matrix();
        params.projection_matrix = impl->camera.projection_matrix(impl->render_target.aspect_ratio());
        params.view_pos = impl->camera.pos();
        params.light_pos = impl->light_pos;
        params.light_rgb = impl->light_rgb;
        params.background_rgba = impl->background_rgba;
        params.rim_rgba = impl->rim_rgba;
        params.flags = RawRendererFlags_Default;
        params.flags &= ~RawRendererFlags_DrawDebugQuads;

        (void)impl->renderer.draw(impl->cache.storage, params, impl->drawlist, impl->render_target);

        gl::UseProgram(impl->pts.p);
        gl::Uniform(impl->pts.uMVP, gl::identity_val);
        gl::ActiveTexture(GL_TEXTURE0);
        gl::BindTexture(impl->render_target.main());
        gl::Uniform(impl->pts.uSampler0, gl::texture_index<GL_TEXTURE0>());
        gl::BindVertexArray(impl->quad_vao);
        gl::DrawArrays(GL_TRIANGLES, 0, impl->quad_vbo.sizei());
        gl::BindVertexArray();
    }

    // draw logo just above the menu
    {
        constexpr glm::vec2 logo_dims = {125.0f, 125.0f};
        glm::vec2 scale = logo_dims / window_dims;

        glm::mat4 mtx = glm::scale(
            glm::translate(
                glm::identity<glm::mat4>(), glm::vec3{0.0f, (menu_dims.y + logo_dims.y) / window_dims.y, 0.0f}),
            glm::vec3{scale.x, scale.y, 1.0f});

        gl::UseProgram(impl->pts.p);
        gl::Uniform(impl->pts.uMVP, mtx);
        gl::ActiveTexture(GL_TEXTURE0);
        gl::BindTexture(impl->logo);
        gl::Uniform(impl->pts.uSampler0, gl::texture_index<GL_TEXTURE0>());
        gl::BindVertexArray(impl->quad_vao);
        glDisable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
        gl::DrawArrays(GL_TRIANGLES, 0, impl->quad_vbo.sizei());
        glDisable(GL_BLEND);
        glEnable(GL_DEPTH_TEST);
        gl::BindVertexArray();
    }

    if (ImGui::BeginMainMenuBar()) {
        draw_main_menu_file_tab(impl->mm_state);
        draw_main_menu_about_tab();
        ImGui::EndMainMenuBar();
    }

    // center the menu
    {
        auto d = app.window_dimensions();
        float menu_x = static_cast<float>((d.w - menu_dims.x) / 2);
        float menu_y = static_cast<float>((d.h - menu_dims.y) / 2);

        ImGui::SetNextWindowPos(ImVec2(menu_x, menu_y));
        ImGui::SetNextWindowSize(ImVec2(menu_dims.x, -1));
        ImGui::SetNextWindowSizeConstraints(menu_dims, menu_dims);
    }

    bool b = true;
    if (ImGui::Begin("Splash screen", &b, ImGuiWindowFlags_NoTitleBar)) {
        // de-dupe imgui IDs because these lists may contain duplicate
        // names
        int id = 0;

        ImGui::Columns(2);

        // left column: recent files
        if (!impl->mm_state.recent_files.empty()) {
            ImGui::Text("Recent files:");
            ImGui::Dummy(ImVec2{0.0f, 3.0f});

            // iterate in reverse: recent files are stored oldest --> newest
            for (auto it = impl->mm_state.recent_files.rbegin(); it != impl->mm_state.recent_files.rend(); ++it) {
                config::Recent_file const& rf = *it;
                ImGui::PushID(++id);
                if (ImGui::Button(rf.path.filename().string().c_str())) {
                    app.request_screen_transition<osc::Loading_screen>(rf.path);
                }
                ImGui::PopID();
            }
        }
        ImGui::NextColumn();

        // right column: example model files
        if (!impl->mm_state.example_osims.empty()) {
            ImGui::Text("Examples:");
            ImGui::Dummy(ImVec2{0.0f, 3.0f});

            for (fs::path const& ex : impl->mm_state.example_osims) {
                ImGui::PushID(++id);
                if (ImGui::Button(ex.filename().string().c_str())) {
                    app.request_screen_transition<osc::Loading_screen>(ex);
                }
                ImGui::PopID();
            }
        }
        ImGui::NextColumn();

        ImGui::Columns();
    }
    ImGui::End();

    // draw logo just above the menu
    {
        constexpr glm::vec2 logo_dims = {128.0f, 128.0f};
        glm::vec2 scale = logo_dims / window_dims;

        glm::mat4 mtx = glm::scale(
            glm::translate(
                glm::identity<glm::mat4>(),
                glm::vec3{-logo_dims.x / window_dims.x, -(menu_dims.y + logo_dims.y + 10.0f) / window_dims.y, 0.0f}),
            glm::vec3{scale.x, scale.y, 1.0f});

        gl::UseProgram(impl->pts.p);
        gl::Uniform(impl->pts.uMVP, mtx);
        gl::ActiveTexture(GL_TEXTURE0);
        gl::BindTexture(impl->tud_logo);
        gl::Uniform(impl->pts.uSampler0, gl::texture_index<GL_TEXTURE0>());
        gl::BindVertexArray(impl->quad_vao);
        glDisable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
        gl::DrawArrays(GL_TRIANGLES, 0, impl->quad_vbo.sizei());
        glDisable(GL_BLEND);
        glEnable(GL_DEPTH_TEST);
        gl::BindVertexArray();
    }

    // draw logo just above the menu
    {
        constexpr glm::vec2 logo_dims = {128.0f, 128.0f};
        glm::vec2 scale = logo_dims / window_dims;

        glm::mat4 mtx = glm::scale(
            glm::translate(
                glm::identity<glm::mat4>(),
                glm::vec3{logo_dims.x / window_dims.x, -(menu_dims.y + logo_dims.y + 10.0f) / window_dims.y, 0.0f}),
            glm::vec3{scale.x, scale.y, 1.0f});

        gl::UseProgram(impl->pts.p);
        gl::Uniform(impl->pts.uMVP, mtx);
        gl::ActiveTexture(GL_TEXTURE0);
        gl::BindTexture(impl->cz_logo);
        gl::Uniform(impl->pts.uSampler0, gl::texture_index<GL_TEXTURE0>());
        gl::BindVertexArray(impl->quad_vao);
        glDisable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
        gl::DrawArrays(GL_TRIANGLES, 0, impl->quad_vbo.sizei());
        glDisable(GL_BLEND);
        glEnable(GL_DEPTH_TEST);
        gl::BindVertexArray();
    }
}
