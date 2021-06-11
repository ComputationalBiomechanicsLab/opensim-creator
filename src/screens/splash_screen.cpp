#include "splash_screen.hpp"

#include "osc_build_config.hpp"
#include "src/3d/cameras.hpp"
#include "src/3d/3d.hpp"
#include "src/3d/gl.hpp"
#include "src/3d/gl_glm.hpp"
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

static constexpr glm::vec2 menu_dims = {700.0f, 500.0f};

static Drawlist create_drawlist_with_chequered_floor() {
    Drawlist rv;

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
    rv.push_back(mi);

    return rv;
}

static void render_quad(osc::GPU_storage& gs, glm::mat4 const& mvp, gl::Texture_2d& tex) {
    Plain_texture_shader& pts = *gs.shader_pts;
    gl::UseProgram(pts.p);
    gl::Uniform(pts.uMVP, mvp);
    gl::ActiveTexture(GL_TEXTURE0);
    gl::BindTexture(tex);
    gl::Uniform(pts.uSampler0, gl::texture_index<GL_TEXTURE0>());
    gl::BindVertexArray(gs.pts_quad_vao);
    gl::DrawArrays(GL_TRIANGLES, 0, gs.quad_vbo.sizei());
    gl::BindVertexArray();
}

struct Splash_screen::Impl final {
    // main menu (top bar) states
    ui::main_menu::file_tab::State mm_state;

    // main app logo, blitted to top of the screen
    gl::Texture_2d logo = load_image_as_texture(resource("logo.png").string().c_str(), TexFlag_Flip_Pixels_Vertically).texture;

    // CZI attributation logo, blitted to bottom of screen
    gl::Texture_2d cz_logo = load_image_as_texture(resource("chanzuckerberg_logo.png").string().c_str(), TexFlag_Flip_Pixels_Vertically).texture;

    // TUD attributation logo, blitted to bottom of screen
    gl::Texture_2d tud_logo = load_image_as_texture(resource("tud_logo.png").string().c_str(), TexFlag_Flip_Pixels_Vertically).texture;

    // 3D stuff: for the background image
    Render_params params;
    Drawlist drawlist = create_drawlist_with_chequered_floor();
    Render_target render_target;
    Polar_perspective_camera camera;
};

static void tick(Splash_screen::Impl& impl, float dt) {
    impl.camera.theta += dt * 0.015f;
}

static void draw(Splash_screen::Impl& impl) {
    Application& app = Application::current();
    GPU_storage& gs = app.get_gpu_storage();

    auto [w, h] = app.window_dimensionsf();
    glm::vec2 window_dims{w, h};

    gl::Enable(GL_BLEND);

    // draw chequered floor background
    {
        auto [wi, hi] = app.window_dimensionsi();
        impl.render_target.reconfigure(wi, hi, app.samples());

        impl.params.view_matrix = view_matrix(impl.camera);
        impl.params.projection_matrix = projection_matrix(impl.camera, impl.render_target.aspect_ratio());
        impl.params.view_pos = pos(impl.camera);

        draw_scene(gs, impl.params, impl.drawlist, impl.render_target);
        render_quad(gs, glm::mat4{1.0f}, impl.render_target.main());
    }

    // draw logo just above the menu
    {
        glm::vec2 desired_logo_dims = {128.0f, 128.0f};
        glm::vec2 scale = desired_logo_dims / window_dims;
        float y_above_menu = (menu_dims.y + desired_logo_dims.y + 64.0f) / window_dims.y;

        glm::mat4 translate_xform = glm::translate(glm::mat4{1.0f}, {0.0f, y_above_menu, 0.0f});
        glm::mat4 scale_xform = glm::scale(glm::mat4{1.0f}, {scale.x, scale.y, 1.0f});

        glm::mat4 model = translate_xform * scale_xform;

        gl::Disable(GL_DEPTH_TEST);
        gl::Enable(GL_BLEND);
        render_quad(gs, model, impl.logo);
    }

    if (ImGui::BeginMainMenuBar()) {
        ui::main_menu::file_tab::draw(impl.mm_state);
        ui::main_menu::about_tab::draw();
        ImGui::EndMainMenuBar();
    }

    // center the menu
    {
        glm::vec2 menu_pos = (window_dims - menu_dims) / 2.0f;
        ImGui::SetNextWindowPos(menu_pos);
        ImGui::SetNextWindowSize(ImVec2(menu_dims.x, -1));
        ImGui::SetNextWindowSizeConstraints(menu_dims, menu_dims);
    }

    bool b = true;
    if (ImGui::Begin("Splash screen", &b, ImGuiWindowFlags_NoTitleBar)) {

        // `new` button
        {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{0.0f, 0.6f, 0.0f, 1.0f});
            if (ImGui::Button(ICON_FA_FILE_ALT " New Model (Ctrl+N)")) {
                ui::main_menu::action_new_model();
            }
            ImGui::PopStyleColor();
        }

        ImGui::SameLine();

        // `open` button
        {
            if (ImGui::Button(ICON_FA_FOLDER_OPEN " Open Model (Ctrl+O)")) {
                ui::main_menu::action_open_model();
            }
        }

        ImGui::Dummy(ImVec2{0.0f, 10.0f});

        // de-dupe imgui IDs because these lists may contain duplicate
        // names
        int id = 0;

        ImGui::Columns(2);

        // left column: recent files
        if (!impl.mm_state.recent_files.empty()) {
            ImGui::TextUnformatted("Recent files:");
            ImGui::Dummy(ImVec2{0.0f, 3.0f});

            // iterate in reverse: recent files are stored oldest --> newest
            for (auto it = impl.mm_state.recent_files.rbegin(); it != impl.mm_state.recent_files.rend(); ++it) {
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
        if (!impl.mm_state.example_osims.empty()) {
            ImGui::TextUnformatted("Examples:");
            ImGui::Dummy(ImVec2{0.0f, 3.0f});

            for (fs::path const& ex : impl.mm_state.example_osims) {
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

    // draw TUD logo below menu, slightly to the left
    {
        glm::vec2 desired_logo_dims = {128.0f, 128.0f};
        glm::vec2 scale = desired_logo_dims / window_dims;
        float x_leftwards_by_logo_width = -desired_logo_dims.x / window_dims.x;
        float y_below_menu = -(menu_dims.y + desired_logo_dims.y) / window_dims.y;

        glm::mat4 translate_xform = glm::translate(glm::mat4{1.0f}, {x_leftwards_by_logo_width, y_below_menu, 0.0f});
        glm::mat4 scale_xform = glm::scale(glm::mat4{1.0f}, {scale.x, scale.y, 1.0f});

        glm::mat4 model = translate_xform * scale_xform;

        gl::Enable(GL_BLEND);
        gl::Disable(GL_DEPTH_TEST);
        render_quad(gs, model, impl.tud_logo);
    }

    // draw CZI logo below menu, slightly to the right
    {
        glm::vec2 desired_logo_dims = {128.0f, 128.0f};
        glm::vec2 scale = desired_logo_dims / window_dims;
        float x_rightwards_by_logo_width = desired_logo_dims.x / window_dims.x;
        float y_below_menu = -(menu_dims.y + desired_logo_dims.y) / window_dims.y;

        glm::mat4 translate_xform = glm::translate(glm::mat4{1.0f}, {x_rightwards_by_logo_width, y_below_menu, 0.0f});
        glm::mat4 scale_xform = glm::scale(glm::mat4{1.0f}, {scale.x, scale.y, 1.0f});

        glm::mat4 model = translate_xform * scale_xform;

        gl::Disable(GL_DEPTH_TEST);
        gl::Enable(GL_BLEND);
        render_quad(gs, model, impl.cz_logo);
    }
}

// PIMPL forwarding for osc::Splash_screen

osc::Splash_screen::Splash_screen() : impl{new Impl{}} {
}

osc::Splash_screen::~Splash_screen() noexcept {
    delete impl;
}

void osc::Splash_screen::tick(float dt) {
    ::tick(*impl, dt);
}

void osc::Splash_screen::draw() {
    ::draw(*impl);
}
