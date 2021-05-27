#include "splash_screen.hpp"

#include "osc_build_config.hpp"
#include "src/3d/cameras.hpp"
#include "src/3d/3d.hpp"
#include "src/3d/gl.hpp"
#include "src/3d/gl_extensions.hpp"
#include "src/3d/texturing.hpp"
#include "src/3d/shaders.hpp"
#include "src/application.hpp"
#include "src/config.hpp"
#include "src/constants.hpp"
#include "src/screens/imgui_demo_screen.hpp"
#include "src/screens/loading_screen.hpp"
#include "src/ui/main_menu.hpp"
#include "src/utils/helpers.hpp"
#include "src/utils/scope_guard.hpp"

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

struct Splash_screen::Impl final {
    ui::main_menu::file_tab::State mm_state;
    gl::Texture_2d logo = osc::load_tex(resource("logo.png").string().c_str(), TexFlag_Flip_Pixels_Vertically);
    gl::Texture_2d cz_logo = osc::load_tex(resource("chanzuckerberg_logo.png").string().c_str(), TexFlag_Flip_Pixels_Vertically);
    gl::Texture_2d tud_logo = osc::load_tex(resource("tud_logo.png").string().c_str(), TexFlag_Flip_Pixels_Vertically);
    Drawlist drawlist;

    Polar_perspective_camera camera;
    glm::vec3 light_dir = {-0.34f, -0.25f, 0.05f};
    glm::vec3 light_rgb = {248.0f / 255.0f, 247.0f / 255.0f, 247.0f / 255.0f};
    glm::vec4 background_rgba = {0.89f, 0.89f, 0.89f, 1.0f};
    glm::vec4 rim_rgba = {1.0f, 0.4f, 0.0f, 0.85f};
    Render_target render_target{1, 1, 1};

    Impl() {
        glm::mat4 model_mtx = []() {
            glm::mat4 rv = glm::identity<glm::mat4>();

            // OpenSim: might contain floors at *exactly* Y = 0.0, so shift the chequered
            // floor down *slightly* to prevent Z fighting from planes rendered from the
            // model itself (the contact planes, etc.)
            rv = glm::translate(rv, {0.0f, -0.001f, 0.0f});
            rv = glm::rotate(rv, osc::pi_f / 2, {-1.0, 0.0, 0.0});
            rv = glm::scale(rv, {100.0f, 100.0f, 1.0f});

            return rv;
        }();

        Mesh_instance mi;
        mi.model_xform = model_mtx;
        mi.normal_xform = normal_matrix(mi.model_xform);
        auto& gpu_storage = Application::current().get_gpu_storage();
        mi.meshidx = gpu_storage.floor_quad_idx;
        mi.texidx = gpu_storage.chequer_idx;
        mi.flags.set_skip_shading();
        drawlist.push_back(mi);
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
            ui::main_menu::action_new_model();
            return true;
        case SDLK_o:
            ui::main_menu::action_open_model();
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

    constexpr glm::vec2 menu_dims = {700.0f, 500.0f};
    glm::vec2 window_dims;
    {
        auto [w, h] = app.window_dimensions();
        window_dims.x = static_cast<float>(w);
        window_dims.y = static_cast<float>(h);
    }

    GPU_storage& gs = Application::current().get_gpu_storage();

    // draw chequered floor background
    {
        impl->render_target.reconfigure(app.window_dimensions().w, app.window_dimensions().h, app.samples());

        Render_params params;
        params.passthrough_hittest_x = -1;
        params.passthrough_hittest_y = -1;
        params.view_matrix = view_matrix(impl->camera);
        params.projection_matrix = projection_matrix(impl->camera, impl->render_target.aspect_ratio());
        params.view_pos = pos(impl->camera);
        params.light_dir = impl->light_dir;
        params.light_rgb = impl->light_rgb;
        params.background_rgba = impl->background_rgba;
        params.rim_rgba = impl->rim_rgba;
        params.flags = RawRendererFlags_Default;
        params.flags &= ~RawRendererFlags_DrawDebugQuads;

        draw_scene(gs, params, impl->drawlist, impl->render_target);

        Plain_texture_shader& pts = *gs.shader_pts;
        gl::UseProgram(pts.p);
        gl::Uniform(pts.uMVP, gl::identity_val);
        gl::ActiveTexture(GL_TEXTURE0);
        gl::BindTexture(impl->render_target.main());
        gl::Uniform(pts.uSampler0, gl::texture_index<GL_TEXTURE0>());
        gl::BindVertexArray(gs.pts_quad_vao);
        gl::DrawArrays(GL_TRIANGLES, 0, gs.quad_vbo.sizei());
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

        Plain_texture_shader& pts = *gs.shader_pts;
        gl::UseProgram(pts.p);
        gl::Uniform(pts.uMVP, mtx);
        gl::ActiveTexture(GL_TEXTURE0);
        gl::BindTexture(impl->logo);
        gl::Uniform(pts.uSampler0, gl::texture_index<GL_TEXTURE0>());
        gl::BindVertexArray(gs.pts_quad_vao);
        gl::Disable(GL_DEPTH_TEST);
        gl::Enable(GL_BLEND);
        gl::DrawArrays(GL_TRIANGLES, 0, gs.quad_vbo.sizei());
        gl::Disable(GL_BLEND);
        gl::Enable(GL_DEPTH_TEST);
        gl::BindVertexArray();
    }

    if (ImGui::BeginMainMenuBar()) {
        ui::main_menu::file_tab::draw(impl->mm_state);
        ui::main_menu::about_tab::draw();
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
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{0.0f, 0.6f, 0.0f, 1.0f});
        if (ImGui::Button("New Model (Ctrl+N)")) {
            ui::main_menu::action_new_model();
        }
        ImGui::PopStyleColor();
        ImGui::SameLine();
        if (ImGui::Button("Open Model (Ctrl+O)")) {
            ui::main_menu::action_open_model();
        }
        ImGui::Dummy(ImVec2{0.0f, 10.0f});

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
                Recent_file const& rf = *it;
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

        Plain_texture_shader& pts = *gs.shader_pts;
        gl::UseProgram(pts.p);
        gl::Uniform(pts.uMVP, mtx);
        gl::ActiveTexture(GL_TEXTURE0);
        gl::BindTexture(impl->tud_logo);
        gl::Uniform(pts.uSampler0, gl::texture_index<GL_TEXTURE0>());
        gl::BindVertexArray(gs.pts_quad_vao);
        gl::Disable(GL_DEPTH_TEST);
        gl::Enable(GL_BLEND);
        gl::DrawArrays(GL_TRIANGLES, 0, gs.quad_vbo.sizei());
        gl::Disable(GL_BLEND);
        gl::Enable(GL_DEPTH_TEST);
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

        Plain_texture_shader& pts = *gs.shader_pts;
        gl::UseProgram(pts.p);
        gl::Uniform(pts.uMVP, mtx);
        gl::ActiveTexture(GL_TEXTURE0);
        gl::BindTexture(impl->cz_logo);
        gl::Uniform(pts.uSampler0, gl::texture_index<GL_TEXTURE0>());
        gl::BindVertexArray(gs.pts_quad_vao);
        gl::Disable(GL_DEPTH_TEST);
        gl::Enable(GL_BLEND);
        gl::DrawArrays(GL_TRIANGLES, 0, gs.quad_vbo.sizei());
        gl::Disable(GL_BLEND);
        gl::Enable(GL_DEPTH_TEST);
        gl::BindVertexArray();
    }
}
