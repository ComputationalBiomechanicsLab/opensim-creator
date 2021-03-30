#include "splash_screen.hpp"

#include "osmv_config.hpp"
#include "src/3d/gl.hpp"
#include "src/application.hpp"
#include "src/config.hpp"
#include "src/screens/experimental_merged_screen.hpp"
#include "src/screens/imgui_demo_screen.hpp"
#include "src/screens/loading_screen.hpp"
#include "src/screens/model_editor_screen.hpp"
#include "src/screens/opengl_test_screen.hpp"
#include "src/utils/geometry.hpp"
#include "src/utils/scope_guard.hpp"
#include "src/widgets/main_menu_about_tab.hpp"
#include "src/widgets/main_menu_file_tab.hpp"
#include "src/3d/texturing.hpp"
#include "src/3d/render_target.hpp"
#include "src/3d/gpu_cache.hpp"
#include "src/3d/mesh_instance.hpp"
#include "src/3d/drawlist.hpp"
#include "src/constants.hpp"
#include "src/3d/renderer.hpp"
#include "src/3d/polar_camera.hpp"
#include "src/3d/mesh_generation.hpp"

#include <GL/glew.h>
#include <OpenSim/Simulation/Model/Model.h>
#include <SDL_keyboard.h>
#include <SDL_keycode.h>
#include <imgui.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/mat4x3.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include <algorithm>
#include <cstdio>
#include <filesystem>
#include <string>
#include <vector>

namespace fs = std::filesystem;
using namespace osmv;

namespace {
    //
    // useful for rendering quads etc.
    struct Plain_texture_shader final {
        gl::Program p = gl::CreateProgramFrom(
            gl::Compile<gl::Vertex_shader>(osmv::config::shader_path("plain_texture.vert")),
            gl::Compile<gl::Fragment_shader>(osmv::config::shader_path("plain_texture.frag")));

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
        osmv::load_tex(osmv::config::resource_path("logo.png").string().c_str());
    gl::Texture_2d cz_logo = osmv::load_tex(osmv::config::resource_path("chanzuckerberg_logo.png").string().c_str());
    gl::Texture_2d tud_logo = osmv::load_tex(osmv::config::resource_path("tud_logo.png").string().c_str());
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
    gl::Array_bufferT<Textured_vert> quad_vbo{osmv::shaded_textured_quad_verts().vert_data};
    gl::Vertex_array quad_vao = Plain_texture_shader::create_vao(quad_vbo);

    Impl() {
        glm::mat4 model_mtx = []() {
            glm::mat4 rv = glm::identity<glm::mat4>();

            // OpenSim: might contain floors at *exactly* Y = 0.0, so shift the chequered
            // floor down *slightly* to prevent Z fighting from planes rendered from the
            // model itself (the contact planes, etc.)
            rv = glm::translate(rv, {0.0f, -0.001f, 0.0f});
            rv = glm::rotate(rv, osmv::pi_f / 2, {-1.0, 0.0, 0.0});
            rv = glm::scale(rv, {100.0f, 100.0f, 0.0f});

            return rv;
        }();

        Rgba32 color{glm::vec4{1.0f, 0.0f, 1.0f, 1.0f}};
        Mesh_instance& mi = drawlist.emplace_back(model_mtx, color, cache.floor_quad, cache.chequered_texture);
        mi.flags.is_shaded = false;
    }
};

// PIMPL forwarding for osmv::Splash_screen

osmv::Splash_screen::Splash_screen() : impl{new Impl{}} {
}

osmv::Splash_screen::~Splash_screen() noexcept {
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

bool osmv::Splash_screen::on_event(SDL_Event const& e) {
    if (e.type == SDL_KEYDOWN) {
        return on_keydown(e.key);
    }
    return false;
}

void osmv::Splash_screen::draw() {
    Application& app = Application::current();

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


    if (ImGui::BeginMainMenuBar()) {
        draw_main_menu_file_tab(impl->mm_state);
        draw_main_menu_about_tab();
        ImGui::EndMainMenuBar();
    }

    // center the menu
    {
        static constexpr int menu_width = 700;
        static constexpr int menu_height = 700;

        auto d = app.window_dimensions();
        float menu_x = static_cast<float>((d.w - menu_width) / 2);
        float menu_y = static_cast<float>((d.h - menu_height) / 2);

        ImGui::SetNextWindowPos(ImVec2(menu_x, menu_y));
        ImGui::SetNextWindowSize(ImVec2(menu_width, -1));
        ImGui::SetNextWindowSizeConstraints(ImVec2(menu_width, menu_height), ImVec2(menu_width, menu_height));
    }

    bool b = true;
    if (ImGui::Begin("Splash screen", &b, ImGuiWindowFlags_NoTitleBar)) {
        {
            void* texture_handle = reinterpret_cast<void*>(static_cast<uintptr_t>(impl->logo.raw_handle()));
            ImGui::Dummy(ImVec2{ImGui::GetContentRegionAvailWidth() / 2.0f - 125.0f / 2.0f, 0.0f});
            ImGui::SameLine();
            ImVec2 image_dimensions{125.0f, 125.0f};
            ImGui::Image(texture_handle, image_dimensions);
        }

        ImGui::Columns(2);

        // left-column: utils etc.
        {
            ImGui::Text("Utilities:");
            ImGui::Dummy(ImVec2{0.0f, 3.0f});

            if (ImGui::Button("ImGui demo")) {
                app.request_screen_transition<osmv::Imgui_demo_screen>();
            }

            if (ImGui::Button("Model editor")) {
                app.request_screen_transition<osmv::Model_editor_screen>();
            }

            if (ImGui::Button("Rendering tests (meta)")) {
                app.request_screen_transition<osmv::Opengl_test_screen>();
            }

            if (ImGui::Button("experimental new merged screen")) {
                auto rajagopal_path = config::resource_path("models", "RajagopalModel", "Rajagopal2015.osim");
                auto model = std::make_unique<OpenSim::Model>(rajagopal_path.string());
                app.request_screen_transition<Experimental_merged_screen>(std::move(model));
            }

            ImGui::Dummy(ImVec2{0.0f, 4.0f});
            if (ImGui::Button("Exit")) {
                app.request_quit_application();
            }

            ImGui::NextColumn();
        }

        // right-column: open, recent files, examples
        {
            // de-dupe imgui IDs because these lists may contain duplicate
            // names
            int id = 0;

            // recent files:
            if (!impl->mm_state.recent_files.empty()) {
                ImGui::Text("Recent files:");
                ImGui::Dummy(ImVec2{0.0f, 3.0f});

                // iterate in reverse: recent files are stored oldest --> newest
                for (auto it = impl->mm_state.recent_files.rbegin(); it != impl->mm_state.recent_files.rend(); ++it) {
                    config::Recent_file const& rf = *it;
                    ImGui::PushID(++id);
                    if (ImGui::Button(rf.path.filename().string().c_str())) {
                        app.request_screen_transition<osmv::Loading_screen>(rf.path);
                    }
                    ImGui::PopID();
                }
            }

            ImGui::Dummy(ImVec2{0.0f, 5.0f});

            // examples:
            if (!impl->mm_state.example_osims.empty()) {
                ImGui::Text("Examples:");
                ImGui::Dummy(ImVec2{0.0f, 3.0f});

                for (fs::path const& ex : impl->mm_state.example_osims) {
                    ImGui::PushID(++id);
                    if (ImGui::Button(ex.filename().string().c_str())) {
                        app.request_screen_transition<osmv::Loading_screen>(ex);
                    }
                    ImGui::PopID();
                }
            }

            ImGui::NextColumn();
        }

        ImGui::Columns();

        // cz logo
        {
            void* handle = reinterpret_cast<void*>(static_cast<uintptr_t>(impl->cz_logo.raw_handle()));
            ImGui::Dummy(ImVec2{ImGui::GetContentRegionAvailWidth() / 2.0f - 128.0f / 2.0f, 0.0f});
            ImGui::SameLine();
            ImVec2 image_dimensions{128.0f, 128.0f};
            ImGui::Image(handle, image_dimensions);
        }

        // tud logo
        {
            void* handle = reinterpret_cast<void*>(static_cast<uintptr_t>(impl->tud_logo.raw_handle()));
            ImGui::Dummy(ImVec2{ImGui::GetContentRegionAvailWidth() / 2.0f - 128.0f / 2.0f, 0.0f});
            ImGui::SameLine();
            ImVec2 image_dimensions{128.0f, 128.0f};
            ImGui::Image(handle, image_dimensions);
        }
    }
    ImGui::End();

    // bottom-right: version info etc.
    {
        auto wd = app.window_dimensions();
        ImVec2 wd_imgui = {static_cast<float>(wd.w), static_cast<float>(wd.h)};

        char const* l1 = "osmv " OSMV_VERSION_STRING " (build " OSMV_BUILD_ID ")";
        ImVec2 l1_dims = ImGui::CalcTextSize(l1);

        ImVec2 l1_pos = {0, wd_imgui.y - l1_dims.y};

        ImGui::GetBackgroundDrawList()->AddText(l1_pos, 0xff000000, l1);
    }
}
